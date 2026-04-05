#ifndef HSQML_MANAGER_H
#define HSQML_MANAGER_H

#include <QtCore/QAtomicPointer>
#include <QtCore/QAtomicInt>
#include <QtCore/QByteArray>
#include <QtCore/QMutex>
#if QT_VERSION >= 0x060000
#include <QtCore/QRecursiveMutex>
#endif
#include <QtCore/QSet>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QVariant>
#include <QtCore/QVector>
#include <QtWidgets/QApplication>
#include <QtGui/QIcon>

#include "hsqml.h"

#define HSQML_LOG(ll, msg) if (gManager->checkLogLevel(ll)) gManager->log(msg)

class HsQMLManagerApp;
class HsQMLClass;
class HsQMLEngine;

#if QT_VERSION >= 0x060000
typedef QRecursiveMutex HsQMLRecursiveMutex;
#else
typedef QMutex HsQMLRecursiveMutex;
#endif

class HsQMLManager
{
public:
    enum CounterId {
        ClassCount,
        ObjectCount,
        QObjectCount,
        VariantCount,
        EngineCount,
        ClassSerial,
        ObjectSerial,
        EngineSerial,
        TotalCounters
    }; 

    HsQMLManager(
        void (*)(HsFunPtr),
        void (*)(HsStablePtr));
    void setLogLevel(int);
    bool checkLogLevel(int);
    void log(const QString&);
    int updateCounter(CounterId, int);
    void freeFun(HsFunPtr);
    void freeStable(HsStablePtr);
    bool setArgs(const QStringList&);
    QVector<char*>& argsPtrs();
    bool setFlag(HsQMLGlobalFlag, bool);
    bool getFlag(HsQMLGlobalFlag);
    void registerObject(const QObject*);
    void unregisterObject(const QObject*);
#if QT_VERSION < 0x060000
    void hookedConstruct(QVariant::Private*, const void*);
    void hookedClear(QVariant::Private*);
#endif
    bool isEventThread();
    typedef HsQMLEventLoopStatus EventLoopStatus;
    EventLoopStatus runEventLoop(
        HsQMLTrivialCb, HsQMLTrivialCb, HsQMLTrivialCb);
    EventLoopStatus requireEventLoop();
    void releaseEventLoop();
    void notifyJobs();
    void setActiveEngine(HsQMLEngine*);
    HsQMLEngine* activeEngine();
    void postAppEvent(QEvent*);
    void zombifyClass(HsQMLClass*);
    EventLoopStatus shutdown();
    void setWindowIcon(const QString& iconPath);

private:
    friend class HsQMLManagerApp;
    Q_DISABLE_COPY(HsQMLManager)

    int mLogLevel;
    QMutex mLogLock;
    QAtomicInt mCounters[TotalCounters];
    bool mAtExit;
    void (*mFreeFun)(HsFunPtr);
    void (*mFreeStable)(HsStablePtr);
    QVector<QByteArray> mArgs;
    QVector<char*> mArgsPtrs;
    QSet<const QObject*> mObjectSet;
    QVector<HsQMLClass*> mZombieClasses;
#if QT_VERSION < 0x060000
    const QVariant::Handler* mOriginalHandler;
#endif
    HsQMLManagerApp* mApp;
    HsQMLRecursiveMutex mLock;
    bool mRunning;
    int mRunCount;
    bool mShutdown;
    void* mStackBase;
    HsQMLTrivialCb mStartCb;
    HsQMLTrivialCb mJobsCb;
    HsQMLTrivialCb mYieldCb;
    HsQMLEngine* mActiveEngine;
    bool mQmlDebugEnabled;
};

class HsQMLManagerApp : public QObject
{
    Q_OBJECT

public:
    HsQMLManagerApp();
    virtual ~HsQMLManagerApp();
    virtual void customEvent(QEvent*);
    virtual void timerEvent(QTimerEvent*);
    virtual void setWindowIcon(QIcon*);
    int exec();

    enum CustomEventIndicies {
        StartedLoopEventIndex,
        StopLoopEventIndex,
        PendingJobsEventIndex,
        RemoveGCLockEventIndex,
        CreateEngineEventIndex,
    };

    static const QEvent::Type StartedLoopEvent =
        static_cast<QEvent::Type>(QEvent::User+StartedLoopEventIndex);
    static const QEvent::Type StopLoopEvent =
        static_cast<QEvent::Type>(QEvent::User+StopLoopEventIndex);
    static const QEvent::Type PendingJobsEvent =
        static_cast<QEvent::Type>(QEvent::User+PendingJobsEventIndex);
    static const QEvent::Type RemoveGCLockEvent =
        static_cast<QEvent::Type>(QEvent::User+RemoveGCLockEventIndex);
    static const QEvent::Type CreateEngineEvent =
        static_cast<QEvent::Type>(QEvent::User+CreateEngineEventIndex);

private:
    Q_DISABLE_COPY(HsQMLManagerApp)

#if QT_VERSION < 0x060000
    QVariant::Handler mHookedHandler;
#endif
    int mArgC;
    QApplication mApp;
};

class ManagerPointer : public QAtomicPointer<HsQMLManager>
{
public:
    HsQMLManager* operator->() const
    {
        return loadRelaxed();
    }

    operator HsQMLManager*() const
    {
        return loadRelaxed();
    }
};

extern ManagerPointer gManager;

#endif /*HSQML_MANAGER_H*/
