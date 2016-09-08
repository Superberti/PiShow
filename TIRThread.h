#ifndef TIRTHREAD_H
#define TIRTHREAD_H

#include "threadtools.h"
#include <deque>

// Keycodes für die XBox-Fernbedienung
#define KEY_POWER                0x26D9
#define KEY_TAB                  0x7689
#define KEY_LIST                 0xF609
#define KEY_UP                   0x7887
#define KEY_LEFT                 0x04FB
#define KEY_RIGHT                0x847B
#define KEY_DOWN                 0xF807
#define KEY_OK                   0x44BB
#define KEY_BACK                 0xC43B
#define KEY_CONTEXT_MENU         0x649B
#define KEY_VOLUMEUP             0x08F7
#define KEY_VOLUMEDOWN           0x8877
#define KEY_MUTE                 0x708F
#define KEY_CHANNELUP            0x48B7
#define KEY_CHANNELDOWN          0xC837
#define KEY_PLAY                 0x0EF1
#define KEY_STOP                 0x9867
#define KEY_REWIND               0xA857
#define KEY_FASTFORWARD          0x28D7
#define KEY_NEXT                 0x58A7
#define KEY_PREVIOUS             0xD827

struct IRCode
{
  int Code;
  int Repeat;
  IRCode(int aCode, int aRepeat){Code=aCode; Repeat=aRepeat;}
  IRCode(){Code=0;Repeat=0;}
};

/// Thread fuer die Fernbedienung
class TIRThread : public TThread
{
	protected:
    int mLircSocket;
		/// Threadfunktion, kümmert sich um die Verbindung
		void Execute();
		/// Kommando bearbeiten
		/// \param aCmd Infrarot-Kommando von der Fernbedienung
		void HandleCommand(int aCmd);
	public:
    TCritVar<std::deque<IRCode> >IRCommandQueue;
		/// CTOR
		TIRThread();
		virtual ~TIRThread();
};

#endif // TIRTHREAD_H
