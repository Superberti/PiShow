//----------------------------------------------------------------------------
#include "threadtools.h"
#include <memory>
//----------------------------------------------------------------------------

TEvent::TEvent(const bool aAutoReset, const bool aSignaled)
  : AutoReset(aAutoReset),Signaled(aSignaled)
{
  int st=pthread_cond_init(&mCondVar,NULL);
  if (st)
    throw TOSErr("pthread_cond_init failed.",st);
}

int TEvent::WaitFor(const unsigned int aTimeout_ms)
{
  mCondMutex.Lock();
  int st=0;
  while (!Signaled)
  {
    if (!aTimeout_ms)
      st=pthread_cond_wait(&mCondVar,mCondMutex.Handle());
    else
    {
      struct timespec timeout,now;
      timeout.tv_sec=aTimeout_ms/1000;
      timeout.tv_nsec=(aTimeout_ms%1000)*1000000;
      clock_gettime(CLOCK_REALTIME, &now);
      ts_add(now, timeout);
      st=pthread_cond_timedwait(&mCondVar,mCondMutex.Handle(),&now);
    }
    if (st)		// Timeout oder Fehler
      break;
  }
  if (AutoReset)
    Signaled=false;
  mCondMutex.Unlock();
  return st;
}

void TEvent::SetEvent()
{
  mCondMutex.Lock();
  Signaled=true;
  mCondMutex.Unlock();
  if (!AutoReset)
  {
    int st=pthread_cond_broadcast(&mCondVar);
    if (st)
      throw TOSErr("pthread_cond_broadcast failed.",st);
  }
  else
  {
    int st=pthread_cond_signal(&mCondVar);
    if (st)
      throw TOSErr("pthread_cond_signal failed.",st);
  }
}

void TEvent::ResetEvent()
{
  mCondMutex.Lock();
  Signaled=false;
  mCondMutex.Unlock();
}

TEvent::~TEvent()
{
  pthread_cond_destroy(&mCondVar);
}

// -----------------------------------------------------------
// -----------------------------------------------------------
// -----------------------------------------------------------

TThread::TThread()
  : Handle(0)
  , AlreadyRunning(false)
  , ThreadResult(NULL)
  , Status(0)
  , SyncEvent(true,false)

{
  TerminateRequested.Set(false);
  Terminated.Set(false);
}

TThread::~TThread()
{
  Terminate();
  WaitFor();
}

void TThread::Run()
{
  // Mehrfachen Run-Aufruf vermeiden
  if (AlreadyRunning)
    throw TPiShowErr("Thread of this object already running.");
  AlreadyRunning=true;
  Status=pthread_create(&Handle,NULL,ThreadFunc,this);
  if (Status)
    throw TOSErr("pthread_create failed.",Status);
  // Warten, bis der Thread auch wirklich läuft...
  SyncEvent.WaitFor();

}

void * TThread::ThreadFunc(void * aInstance)
{
  TThread * MyInstance=static_cast<TThread*>(aInstance);
  if (!MyInstance)
    return NULL;
  try
  {
    // Event setzten, Thread läuft
    MyInstance->SyncEvent.SetEvent();
    MyInstance->Execute();
    MyInstance->Terminated.Set(true);
  }
  catch (std::exception & exc)
  {
    TCritGuard cg(MyInstance->mThreadErr.GetCritSec());
    MyInstance->mThreadErr.GetUnsafe().reset(new TPiShowErr(exc.what()));
    MyInstance->Terminated.Set(true);
  }
  catch (...)
  {
    TCritGuard cg(MyInstance->mThreadErr.GetCritSec());
    MyInstance->mThreadErr.GetUnsafe().reset(new TPiShowErr("Unknown thread exception."));
    MyInstance->Terminated.Set(true);
  }
  return NULL;
}

void TThread::Terminate()
{
  TerminateRequested.Set(true);
}

void TThread::WaitFor()
{
  if (Handle==0)
    return;
  // Erst nach dem Join werden die Thread-Ressourcen freigegeben!
  // Jedem pthread_create ist ein entsprechendes Join gegeueberzustellen!
  Status=pthread_join(Handle,&ThreadResult);
  Handle=0;
  //if (Status)
  //throw TOSErr("pthread_join failed.",Status);
}
