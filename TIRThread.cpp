#include <errno.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <lirc/lirc_client.h>

#include "TIRThread.h"


// Einfacher iterativer Server für Informationszwecke.
// Wird von anderen Dämonen benutzt, um festzustellen, wer
// gerade verbunden ist usw.
TIRThread::TIRThread()
, mIRSocket(0)
{
  // Am Ende des Konstruktors: Thread starten
  Run();
}

// ----------------------------------------------------------------------------

TIRThread::~TIRThread()
{
  Terminate();
  // Thread unterbrechen, falls der in einem blockierenden Aufruf steckt
  if (!Terminated.Get())
  {

    //pthread_kill( Handle, SIGUSR1 );
  }
  //DebugOut(LOG_INFO, "Waiting for thread end in info thread DTOR.");
  WaitFor();

}

// ----------------------------------------------------------------------------

void TIRThread::Execute()
{
  fprintf(stderr, "Starte Fernbedienungs-Thread.");
  try
  {
    while (!Terminated.Get())
    {
      HandleCommand(1);
    }
  }
  catch(TPiShowErr & err)
  {
    fprintf(stderr, "Exception in info thread: %s",err.what());
  }
  catch(...)
  {
    fprintf(stderr, "Unknown exception in info thread.");
  }
  fprintf(stderr, "Beende Fernbedienungs-Thread.");
}

// ----------------------------------------------------------------------------

void TIRThread::HandleCommand(int aCmd)
{
  fprintf(stderr, "Kommando empfangen: %d",aCmd);
}
