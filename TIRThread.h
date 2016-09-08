#ifndef TIRTHREAD_H
#define TIRTHREAD_H

#include "threadtools.h"

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
		/// CTOR
		TIRThread();
		virtual ~TIRThread();
};

#endif // TIRTHREAD_H
