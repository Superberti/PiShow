#include <errno.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
//#include <lirc/lirc_client.h>
#include "tools.h"

#include "TIRThread.h"

#define SOCK_PATH "/var/run/lirc/lircd"

// Einfacher iterativer Server für Informationszwecke.
// Wird von anderen Dämonen benutzt, um festzustellen, wer
// gerade verbunden ist usw.
TIRThread::TIRThread()
: mLircSocket(0)
{
  // Am Ende des Konstruktors: Thread starten
  Run();
}

TIRThread::~TIRThread()
{
  printf("Schließe Socket\r\n");
  shutdown(mLircSocket, SHUT_RDWR);
  close(mLircSocket);
  printf("Socket geschlossen\r\n");
}

// ----------------------------------------------------------------------------

#define str_length 256

void TIRThread::Execute()
{
  fprintf(stdout, "Starte Fernbedienungs-Thread.");

  //HandleCommand(1);
  int t, len;
  struct sockaddr_un remote;
  char str[str_length];

  try
  {
    if ((mLircSocket = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
    {
      throw TOSErr("socket failed.",errno);
      //perror("socket");
      //exit(1);
    }

    fprintf(stdout,"Trying to connect...\n");

    remote.sun_family = AF_UNIX;
    strcpy(remote.sun_path, SOCK_PATH);
    len = strlen(remote.sun_path) + sizeof(remote.sun_family);
    if (connect(mLircSocket, (struct sockaddr *)&remote, len) == -1)
    {
      throw TOSErr("connect failed.",errno);
    }

    fprintf(stdout,"Connected.\n");

    while(!TerminateRequested.Get())
    {

      if ((t=recv(mLircSocket, str, str_length-1, 0)) > 0)
      {
        str[t] = '\0';
        //fprintf(stdout,"%s\r\n",str);
        KillReturnAndEndl(str);
        std::string irString=std::string(str);
        std::vector<std::string> mCommands=ParseLine(irString);
        if (mCommands.size()>=3)
        {
          std::string IRNumberString=mCommands[0].substr(mCommands[0].size()-4,4);
          int IRNumber=0;
          int RepeatCode=0;
          bool res=HexStringToInt(IRNumberString, IRNumber);
          res&=HexStringToInt(mCommands[1],RepeatCode);
          fprintf(stdout,"Taste %d gedrückt. Res: %d RepeatCode: %d String %s\r\n", IRNumber,res, RepeatCode, IRNumberString.c_str());
          {
            TCritGuard cg(IRCommandQueue.GetCritSec());
            // Bis zu 10 IR-Kommandos werden in der Queue zwischengespeichert
            if (IRCommandQueue.GetUnsafe().size()<=10)
              IRCommandQueue.GetUnsafe().push_back(IRCode(IRNumber, RepeatCode));
          }
        }
        fflush(stdout);
      }
      else
      {
        if (t < 0)
          perror("recv");
        else
          fprintf(stdout,"Server closed connection\n");
        throw TOSErr("recv failed.",errno);
      }
    }

  }
  catch(std::exception & err)
  {
    fprintf(stderr, "Exception in infrared thread: %s",err.what());
  }
  catch(...)
  {
    fprintf(stderr, "Unknown exception in info thread.");
  }
  close(mLircSocket);
  fprintf(stdout, "Beende Fernbedienungs-Thread.");
}

// ----------------------------------------------------------------------------

void TIRThread::HandleCommand(int aCmd)
{
  fprintf(stderr, "Kommando empfangen: %d",aCmd);
}
