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
#include <wiringPi.h>
//#include <lirc/lirc_client.h>
#include "tools.h"

#include "TIRThread.h"

#define SOCK_PATH "/var/run/lirc/lircd"

#define DISPLAY_POWER_PIN 27

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
  fprintf(stdout, "Starte Fernbedienungs-Thread.\r\n");

  //HandleCommand(1);
  int t, len;
  struct sockaddr_un remote;
  char str[str_length];
  bool gpio_ok=false;
  try
  {
    // Starte die WiringPi-Api (wichtig)
    if (wiringPiSetupGpio() == -1)
      throw TOSErr("wiringPiSetupGpio failed.",errno);
    pinMode(DISPLAY_POWER_PIN, OUTPUT);
    gpio_ok=true;
  }
  catch(std::exception & err)
  {
    fprintf(stderr, "Exception in gpio init: %s",err.what());
  }
  catch(...)
  {
    fprintf(stderr, "Unknown exception in gpio init.");
  }

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

          if (RepeatCode==0 && (IRNumber==KEY_PLAY || IRNumber==KEY_STOP || IRNumber==KEY_REWIND || IRNumber==KEY_FASTFORWARD || IRNumber==KEY_NEXT || IRNumber==KEY_PREVIOUS))
          {
            fprintf(stdout,"Taste %d gedrückt. Res: %d RepeatCode: %d String %s\r\n", IRNumber,res, RepeatCode, IRNumberString.c_str());
            TCritGuard cg(IRCommandQueue.GetCritSec());
            // Bis zu 10 IR-Kommandos werden in der Queue zwischengespeichert. Aber nur Kommandos, die der Bildsteuerung dienen
            if (IRCommandQueue.GetUnsafe().size()<=10 )
              IRCommandQueue.GetUnsafe().push_back(IRCode(IRNumber, RepeatCode));
          }
          else if (RepeatCode==0 && IRNumber==KEY_POWER && gpio_ok)
          {
            // Display ein/ausschalten
            digitalWrite(DISPLAY_POWER_PIN, 1);
            // Warte 200 ms
            delay(200);
            digitalWrite(DISPLAY_POWER_PIN, 0);
            delay(200);
          }

        }
        fflush(stdout);
      }
      else
      {
        if (t < 0)
          perror("recv");
        else
          fprintf(stdout,"Server closed connection\r\n");
        throw TOSErr("recv failed.",errno);
      }
    }

  }
  catch(std::exception & err)
  {
    fprintf(stderr, "Exception in infrared thread: %s\r\n",err.what());
  }
  catch(...)
  {
    fprintf(stderr, "Unknown exception in info thread.\r\n");
  }
  close(mLircSocket);
  fprintf(stdout, "Beende Fernbedienungs-Thread.\r\n");
}

// ----------------------------------------------------------------------------

void TIRThread::HandleCommand(int aCmd)
{
  fprintf(stderr, "Kommando empfangen: %d",aCmd);
}
