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

void TIRThread::Execute()
{
  fprintf(stdout, "Starte Fernbedienungs-Thread.");
  /*
  struct lirc_config *config;
  char *code;
  char *c;
  try
  {
    if( lirc_init("irexec",1) == -1)
      throw TOSErr("lirc_init failed.",errno);

    if (lirc_readconfig(NULL,&config,NULL) == -1)
    {
      lirc_deinit();
      throw TOSErr("lirc_readconfig failed.",errno);
    }

    while( lirc_nextcode(&code) == 0 && !Terminated.Get())
    {
      if (code == NULL)
      {
        // 100 ms Pause
        nanosleep((const struct timespec[])
        {
          {
            0, 100000000L
          }
        }, NULL);
        continue;
      }
      while(( ret = lirc_code2char( config,code,&c)) == 0 && c != NULL)
      {
        fprintf(stderr,"Received command \"%%s\"\\n",c);
        //system(c);
      }
      free(code);
      if(ret==-1)
        break;
    }

    lirc_freeconfig(config);
    lirc_deinit();*/

#define SOCK_PATH "/var/run/lirc/lircd"
  //HandleCommand(1);
  int t, len;
  struct sockaddr_un remote;
  char str[256];

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
      //perror("connect");
      //exit(1);
    }

    fprintf(stdout,"Connected.\n");

    while(!TerminateRequested.Get())
    {
      /*
        if (send(s, str, strlen(str), 0) == -1)
        {
          perror("send");
          exit(1);
        }*/

      if ((t=recv(mLircSocket, str, 100, 0)) > 0)
      {
        str[t] = '\0';
        fprintf(stdout,"%s", str);
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

    close(mLircSocket);

  }
  catch(std::exception & err)
  {
    fprintf(stderr, "Exception in infrared thread: %s",err.what());
  }
  catch(...)
  {
    fprintf(stderr, "Unknown exception in info thread.");
  }
  fprintf(stdout, "Beende Fernbedienungs-Thread.");
}

// ----------------------------------------------------------------------------

void TIRThread::HandleCommand(int aCmd)
{
  fprintf(stderr, "Kommando empfangen: %d",aCmd);
}
