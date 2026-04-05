#ifndef HSQML_CLASS_H
#define HSQML_CLASS_H

#include <QtCore/QObject>
#include <QtCore/QAtomicInt>
#include <QtCore/QMetaType>
#include <QtCore/QScopedArrayPointer>

#include "hsqml.h"

class HsQMLClass
{
public:
    HsQMLClass(
        unsigned int*, unsigned int*, char*,
        HsStablePtr, HsQMLUniformFunc*, HsQMLUniformFunc*);
    ~HsQMLClass();
    const char* name();
    HsStablePtr hsTypeRep();
    int methodCount();
    int propertyCount();
    const HsQMLUniformFunc* methods();
    const HsQMLUniformFunc* properties();
    const QMetaObject* metaObj();
    void destroy();
    enum RefSrc {Handle, ObjProxy};
    void ref(RefSrc);
    void deref(RefSrc);

private:
    QAtomicInt mRefCount;
    unsigned int* mMetaData;
    QScopedArrayPointer<char> mMetaStrData;
    HsStablePtr mHsTypeRep;
    int mMethodCount;
    int mPropertyCount;
    HsQMLUniformFunc* mMethods;
    HsQMLUniformFunc* mProperties;
#if QT_VERSION >= 0x060000
    QMetaObject* mMetaObjectRaw;
#endif
    QMetaObject mMetaObject;
};

#endif /*HSQML_CLASS_H*/
