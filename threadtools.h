//----------------------------------------------------------------------------
#ifndef THREADTOOLS_H
#define THREADTOOLS_H
//----------------------------------------------------------------------------
#include <pthread.h>
#include <memory>
#include "tools.h"
//----------------------------------------------------------------------------

/// Mutex-Klasse (unbenant) für einen pthread-mutex. Wie bei Windows-Standard werden
/// rekursive Mutexe benutzt.
class TMutex
{
private:
  /// pthreads unbenannter Mutex -> CriticalSection in Windows
  pthread_mutex_t mMutex;
  pthread_mutexattr_t mta;
public:
  TMutex()
  {
    int st;
    st=pthread_mutexattr_init(&mta);
    if (st) throw TOSErr("pthread_mutexattr_init failed.",st);
    pthread_mutexattr_settype(&mta,PTHREAD_MUTEX_RECURSIVE_NP);
    st=pthread_mutex_init( &mMutex,&mta);
    if (st) throw TOSErr("pthread_mutex_init failed.",st);
  };
  ~TMutex()
  {
    pthread_mutex_destroy(&mMutex);
    pthread_mutexattr_destroy(&mta);
  };
  void Lock()
  {
    int st;
    st=pthread_mutex_lock(&mMutex);
    if (st) throw TOSErr("pthread_mutex_lock failed.",st);
  };
  /// Versuche Lock zu bekommen
  /// \return true: Lock bekommen, false: Lock nicht bekommen
  bool TryLock()
  {
    int st;
    st=pthread_mutex_trylock(&mMutex);
    if (st==EBUSY) return false;
    else if (st!=0) throw TOSErr("pthread_mutex_unlock failed.",st);
    else return true;
  };
  void Unlock()
  {
    int st;
    st=pthread_mutex_unlock(&mMutex);
    if (st) throw TOSErr("pthread_mutex_unlock failed.",st);
  };
  pthread_mutex_t * Handle()
  {
    return & mMutex;
  };
};

/// Guardklasse fuer einen pthread-Mutex
class TCritGuard
{
protected:
  /// Pointer auf pthread-Mutex;
  TMutex * mCritSec;
public:
  /// Versucht im Konstruktor den Mutex zu bekommen
  TCritGuard(TMutex * aCritSec):mCritSec(aCritSec)
  {
    mCritSec->Lock();
  };
  /// Mutex freigeben im Destruktor
  ~TCritGuard()
  {
    try
    {
      mCritSec->Unlock();
    }
    catch(...) {}
  };
};

/// Nicht-blockierende Guardklasse fuer einen pthread-Mutex
class TTryCritGuard
{
protected:
  /// Pointer auf pthread-Mutex;
  TMutex * mCritSec;
public:
  /// Versucht im Konstruktor den Mutex zu bekommen
  TTryCritGuard(TMutex * aCritSec,bool & aSucc):mCritSec(aCritSec)
  {
    aSucc=mCritSec->TryLock();
  };
  /// Mutex freigeben im Destruktor
  ~TTryCritGuard()
  {
    try
    {
      mCritSec->Unlock();
    }
    catch(...) {}
  };
};


/// Klasse zum Erzeugen einer mit einem Mutex geschuetzen Variable.
/// Die Get/Set-Funktionen sind jeweils durch einen Mutex gelockt.
/// Am besten fuer kleine Objekte geeignet, da durch die Set/Get-Funktion stets eine
/// Kopie erzeugt wird.

template<class T> class TCritVar
{
protected:
  /// Lock auf Variable
  TMutex mVarLock;
  /// Zu schuetzende Variable
  T mVar;
public:
  /// CTOR
  TCritVar():mVar() {};
  /// CTOR
  TCritVar(const T & aVar)
  {
    mVar=aVar;
  };
  /// Lesen der Variable
  T Get()
  {
    TCritGuard VarGuard(&mVarLock);
    return mVar;
  };
  /// Setzen der Variable
  void Set(const T & aVar)
  {
    TCritGuard VarGuard(&mVarLock);
    mVar=aVar;
  };
  /// Variable anfordern zum Lesen/Schreiben. Hierbei ist vom Benutzer selbst
  /// darauf zu achten, dass entsprechende Lock/Release-Anweisungen gegeben werden,
  /// bzw. dass ein TCritGuard fuer mVarLock benutzt wird.
  T & GetUnsafe()
  {
    return mVar;
  };
  /// Lock setzen
  void Lock()
  {
    mVarLock.Lock();
  };
  /// Lock freigeben
  void Release()
  {
    mVarLock.Unlock();
  };
  /// Versuche Lock zu bekommen
  bool TryLock()
  {
    return mVarLock.TryLock();
  };
  /// Pointer auf CriticalSection, fuer TCritGuard
  TMutex * GetCritSec()
  {
    return &mVarLock;
  };
  /// Thread-Safe Zuweisungsoperator
  void operator= (const TCritVar & rhs)
  {
    Set(rhs.mVar);
  };
  /// Thread-Safe += operator
  void operator+= (const T & rhs)
  {
    TCritGuard VarGuard(&mVarLock);
    mVar+=rhs;
  };
  /// Thread-Safe -= operator
  void operator-= (const T & rhs)
  {
    TCritGuard VarGuard(&mVarLock);
    mVar-=rhs;
  };

  /// Atomically pre-increment <value_>.
  const T & operator++ (void)
  {
    TCritGuard VarGuard(&mVarLock);
    return ++mVar;
  };
  /// Atomically post-increment <value_>.
  const T operator++ (int)
  {
    TCritGuard VarGuard(&mVarLock);
    T tmp=mVar;
    ++mVar;
    return tmp;
  };
  /// Atomically pre-decrement <value_>.
  const T & operator-- (void)
  {
    TCritGuard VarGuard(&mVarLock);
    return --mVar;
  };
  /// Atomically post-decrement <value_>.
  const T operator-- (int)
  {
    TCritGuard VarGuard(&mVarLock);
    T tmp=mVar;
    --mVar;
    return tmp;
  };
};

/// Kapselung einer Condition-Variable -> vgl. TEvent unter Windows
class TEvent
{
protected:
  pthread_cond_t mCondVar;
  TMutex mCondMutex;
  bool AutoReset;
  bool Signaled;
public:
  /// CTOR
  /// \param aAutoReset true: Nach dem Warten wird Signaled=false gesetzt
  /// \param aSignaled Anfangszustand
  TEvent(const bool aAutoReset=true, const bool aSignaled=false);
  /// Warten auf den Event.
  /// \param aTimeout_ms Timeout in ms. 0=unendlich lange warten
  /// \return Rueckgabewert von pthread_cond_wait und pthread_cond_timedwait
  int WaitFor(const unsigned int aTimeout_ms=0);
  /// Event auf signalisiert setzen
  void SetEvent();
  /// Event auf nicht signalisiert setzen
  void ResetEvent();
  ~TEvent();
};

/// Thread-Klasse zur Kapselung von pthreads
/// Um einen Thread zu erzeugen muss die rein virtuelle Funktion Execute der
/// Basisklasse überschrieben werden. Diese Funktion wird dann vom
/// Konstruktor aufgerufen
class TThread
{
protected:
  /// Thread-ID
  int mID;
  /// Thread-Handle
  pthread_t Handle;
  /// Merker, damit Run() nicht mehrfach aufgerufen wird
  bool AlreadyRunning;
  /// Zustand Thread läuft oder ist terminiert
  TCritVar<bool> Terminated;
  /// Merker, damit sich der Thread terminiert
  TCritVar<bool> TerminateRequested;
  /// Rückgabewert des Threads, hier immer NULL
  void * ThreadResult;
  /// Hilfsvariable für pthreads-Funktionen
  int Status;
  /// Synchronisationsobjekt für Run()-Aufruf
  TEvent SyncEvent;
  /// Threadfunktion. Muss von jeder abgeleiteten Klasse überschrieben werden
  virtual void Execute()=0;
  /// Statische Hilfsfunktion, um auch Objekt-Methoden als Thread ausführen zu können
  static void * ThreadFunc(void * aInstance);
public:
  TThread();
  virtual ~TThread();
  /// Thread ausführen. Die Funktion wartet so lange, bis der Thread
  /// tatsächlich gestartet wurde.
  void Run();
  /// Aufforderung an den Thread sich zu beenden. Setzt RequestTerminated auf true
  void Terminate();
  /// Wartet, bis der Thread beendet ist
  void WaitFor();
  /// Thread "hart" beenden, noch nicht implementiert
  void Kill();
  /// Thread beendet?
  bool IsTerminated()
  {
    return Terminated.Get();
  };
  /// Fehlerspeicher, falls Exception im Thread auftritt
  TCritVar<std::auto_ptr<TPiShowErr> > mThreadErr;
  /// Gibt das Handle des aktuellen Threads zurueck
  pthread_t & GetHandle()
  {
    return Handle;
  };
};

#endif
