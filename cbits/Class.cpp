#include <cstdlib>
#include <cstring>
#include <HsFFI.h>
#include <QtCore/QMetaObject>
#include <QtCore/QMetaType>
#include <QtCore/QString>
#ifdef HSQML_QT6
#include <QtCore/private/qmetaobjectbuilder_p.h>
#endif

#include "hsqml.h"
#include "Class.h"
#include "Manager.h"

enum MDFields {
    MD_METHOD_COUNT   = 4,
    MD_METHOD_INDEX   = 5,
    MD_PROPERTY_COUNT = 6,
    MD_PROPERTY_INDEX = 7,
};

static const char* cRefSrcNames[] = {"Hndl", "Proxy"};

static QByteArray hsqmlMetaString(
    const unsigned int* metaStrInfo,
    const char* metaStrChar,
    unsigned int index)
{
    unsigned int start = index > 0 ? metaStrInfo[index] : 0;
    unsigned int end = metaStrInfo[index + 1];
    return QByteArray(metaStrChar + start, end - start - 1);
}

#ifdef HSQML_QT6
static QMetaMethodBuilder hsqmlAddMethod(
    QMetaObjectBuilder& builder,
    const unsigned int* metaData,
    const unsigned int* metaStrInfo,
    const char* metaStrChar,
    unsigned int entry)
{
    const unsigned int paramCount = metaData[entry + 1];
    const unsigned int typeOffset = metaData[entry + 2];
    const unsigned int flags = metaData[entry + 4];
    const QByteArray methodName = hsqmlMetaString(metaStrInfo, metaStrChar, metaData[entry]);

    QList<QByteArray> paramTypes;
    QList<QByteArray> paramNames;
    paramTypes.reserve(paramCount);
    paramNames.reserve(paramCount);
    for (unsigned int i = 0; i < paramCount; ++i) {
        paramTypes.append(QMetaType(static_cast<int>(metaData[typeOffset + i + 1])).name());
        paramNames.append(hsqmlMetaString(
            metaStrInfo, metaStrChar, metaData[typeOffset + paramCount + i + 1]));
    }

    QByteArray signature = methodName + "(";
    for (int i = 0; i < paramTypes.size(); ++i) {
        if (i) {
            signature += ",";
        }
        signature += paramTypes[i];
    }
    signature += ")";

    QMetaMethodBuilder method = (flags & 0x0c) == 0x04
        ? builder.addSignal(signature)
        : builder.addMethod(signature, QMetaType(static_cast<int>(metaData[typeOffset])).name());
    method.setParameterNames(paramNames);

    switch (flags & 0x03) {
    case 0x00:
        method.setAccess(QMetaMethod::Private);
        break;
    case 0x01:
        method.setAccess(QMetaMethod::Protected);
        break;
    default:
        method.setAccess(QMetaMethod::Public);
        break;
    }

    return method;
}
#endif

HsQMLClass::HsQMLClass(
    unsigned int*  metaData,
    unsigned int*  metaStrInfo,
    char*          metaStrChar,
    HsStablePtr    hsTypeRep,
    HsQMLUniformFunc* methods,
    HsQMLUniformFunc* properties)
    : mRefCount(0)
    , mMetaData(metaData)
    , mHsTypeRep(hsTypeRep)
    , mMethodCount(metaData[MD_METHOD_COUNT])
    , mPropertyCount(metaData[MD_PROPERTY_COUNT])
    , mMethods(methods)
    , mProperties(properties)
#ifdef HSQML_QT6
    , mMetaObjectRaw(NULL)
#endif
{
#ifdef HSQML_QT6
    QMetaObjectBuilder builder;
    builder.setClassName(hsqmlMetaString(metaStrInfo, metaStrChar, 0));
    builder.setSuperClass(&QObject::staticMetaObject);

    const unsigned int methodIndex = metaData[MD_METHOD_INDEX];
    QList<QMetaMethodBuilder> signalBuilders;
    signalBuilders.reserve(mMethodCount);
    for (int i = 0; i < mMethodCount; ++i) {
        const unsigned int entry = methodIndex + (5 * i);
        QMetaMethodBuilder method =
            hsqmlAddMethod(builder, metaData, metaStrInfo, metaStrChar, entry);
        if ((metaData[entry + 4] & 0x0c) == 0x04) {
            signalBuilders.append(method);
        }
    }

    const unsigned int propertyIndex = metaData[MD_PROPERTY_INDEX];
    const unsigned int propertyNotifyIndex = propertyIndex + (3 * mPropertyCount);
    for (int i = 0; i < mPropertyCount; ++i) {
        const unsigned int entry = propertyIndex + (3 * i);
        const unsigned int flags = metaData[entry + 2];
        const QByteArray propName = hsqmlMetaString(metaStrInfo, metaStrChar, metaData[entry]);
        const QMetaType propType = QMetaType(static_cast<int>(metaData[entry + 1]));
        QMetaPropertyBuilder prop = builder.addProperty(propName, propType.name(), propType);
        prop.setReadable(flags & 0x00000001);
        prop.setWritable(flags & 0x00000002);
        prop.setResettable(flags & 0x00000004);
        prop.setEnumOrFlag(flags & 0x00000008);
        prop.setScriptable(flags & 0x00004000);
        prop.setConstant(flags & 0x00000400);
        prop.setFinal(flags & 0x00000800);
        if (flags & 0x00400000) {
            const unsigned int notifyIndex = metaData[propertyNotifyIndex + i];
            if (notifyIndex < static_cast<unsigned int>(signalBuilders.size())) {
                prop.setNotifySignal(signalBuilders[notifyIndex]);
            }
        }
    }

    mMetaObjectRaw = builder.toMetaObject();
    mMetaObject = *mMetaObjectRaw;
#else
    // Create string data
    unsigned int strCount = metaStrInfo[0];
    unsigned int strLength = metaStrInfo[strCount];
    size_t arrayOff = strCount * sizeof(QByteArrayData);
    size_t arraySize = arrayOff + strLength;
    mMetaStrData.reset(new char[arraySize]);
    for (unsigned int i = 0; i < strCount; i++) {
        int start = i > 0 ? metaStrInfo[i] : 0;
        int size = metaStrInfo[i + 1] - start;
        int offset = arrayOff - (i * sizeof(QByteArrayData)) + start;
        QByteArrayData data = {
            Q_REFCOUNT_INITIALIZE_STATIC, size - 1, 0, 0, offset};
        std::memcpy(&mMetaStrData[i * sizeof(QByteArrayData)],
            &data, sizeof(QByteArrayData));
    }
    std::memcpy(&mMetaStrData[arrayOff], metaStrChar, strLength);

    // Create meta-object
    QMetaObject metaObj = {
          &QObject::staticMetaObject,
          reinterpret_cast<QByteArrayData*>(mMetaStrData.data()),
          mMetaData,
          0,
          0};
    mMetaObject = metaObj;
#endif

    // Add reference
    ref(Handle);

    gManager->updateCounter(HsQMLManager::ClassCount, 1);
}

HsQMLClass::~HsQMLClass()
{}

const char* HsQMLClass::name()
{
    return mMetaObject.className();
} 

HsStablePtr HsQMLClass::hsTypeRep()
{
    return mHsTypeRep;
}

int HsQMLClass::methodCount()
{
    return mMethodCount;
}

int HsQMLClass::propertyCount()
{
    return mPropertyCount;
}

const HsQMLUniformFunc* HsQMLClass::methods()
{
    return mMethods;
}

const HsQMLUniformFunc* HsQMLClass::properties()
{
    return mProperties;
}

const QMetaObject* HsQMLClass::metaObj()
{
    return &mMetaObject;
}

void HsQMLClass::ref(RefSrc src)
{
    int count = mRefCount.fetchAndAddOrdered(1);

    HSQML_LOG(count == 0 ? 1 : 2,
        QString::asprintf("%s Class, name=%s, src=%s, count=%d.",
        count ? "Ref" : "New", name(), cRefSrcNames[src], count+1));
}

void HsQMLClass::deref(RefSrc src)
{
    int count = mRefCount.fetchAndAddOrdered(-1);

    HSQML_LOG(count == 1 ? 1 : 2,
        QString::asprintf("%s Class, name=%s, src=%s, count=%d.",
        count > 1 ? "Deref" : "Delete", name(), cRefSrcNames[src], count));

    if (count == 1) {
        destroy();
    }
}

void HsQMLClass::destroy()
{
    for (int i=0; i<mMethodCount; i++) {
        gManager->freeFun((HsFunPtr)mMethods[i]);
        mMethods[i] = NULL;
    }
    for (unsigned int i=0; i<2*mPropertyCount; i++) {
        if (mProperties[i]) {
            gManager->freeFun((HsFunPtr)mProperties[i]);
            mProperties[i] = NULL;
        }
    }
    gManager->freeStable(mHsTypeRep);
    mHsTypeRep = NULL;
    std::free(mMetaData);
    mMetaData = NULL;
    std::free(mMethods);
    mMethods = NULL;
    std::free(mProperties);
    mProperties = NULL;
#ifdef HSQML_QT6
    // QMetaObjectBuilder::toMetaObject() returns caller-owned heap storage.
    std::free(static_cast<void*>(mMetaObjectRaw));
    mMetaObjectRaw = NULL;
#endif

    gManager->updateCounter(HsQMLManager::ClassCount, -1);

    // Qt internally retains pointers to QMetaObjects it has encountered
    // without any mechanism for unregistering them. Hence, classes can't be
    // deleted prior to shutdown.
    gManager->zombifyClass(this);
}

extern "C" int hsqml_get_next_class_id()
{
    return gManager->updateCounter(HsQMLManager::ClassSerial, 1);
}

extern "C" HsQMLClassHandle* hsqml_create_class(
    unsigned int*  metaData,
    unsigned int*  metaStrInfo,
    char*          metaStrChar,
    HsStablePtr    hsTypeRep,
    HsQMLUniformFunc* methods,
    HsQMLUniformFunc* properties)
{
    HsQMLClass* klass = new HsQMLClass(
        metaData, metaStrInfo, metaStrChar, hsTypeRep, methods, properties);
    return (HsQMLClassHandle*)klass;
}

extern "C" void hsqml_finalise_class_handle(
    HsQMLClassHandle* hndl)
{
    HsQMLClass* klass = (HsQMLClass*)hndl;
    klass->deref(HsQMLClass::Handle);
}
