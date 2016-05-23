//----------------------------------------------------------------------------
#include <exception>
#include <iostream>
#include "tools.h"
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>

// -----------------------------------------------------------
// -----------------------------------------------------------
// -----------------------------------------------------------

std::string GetOsErrString(const int aErrorNumber)
{
	const size_t ErrBufSize=256;
	char ErrBuf[ErrBufSize];
	memset(ErrBuf,0,256);
	char * msg=strerror_r(aErrorNumber, ErrBuf, ErrBufSize);
	// Um portabel zu bleiben wird geprüft, ober der Fehlerstring ein
	// statischer Text (immutable, GNU) als Rückgabewert der Funktion ist,
	// oder ob der Fehlertext in ErrBuf (XSI-compliant) steht.
	return ErrBuf[0]!=0 ? std::string(ErrBuf) : std::string(msg);
}

//----------------------------------------------------------------------------

bool StringToDouble(const std::string aValue, double & aNumber)
{
	const char * start=aValue.c_str();
	char * end=NULL;
	aNumber=strtod(start,&end);
	// end soll jetzt auch auf das Ende des strings zeigen, sonst Fehler!
	return (end-start==(int)aValue.size());
}

//----------------------------------------------------------------------------

bool StringToInt(const std::string aValue, int & aNumber)
{
	const char * start=aValue.c_str();
	char * end=NULL;
	aNumber=strtol(start,&end,10);
	// end soll jetzt auch auf das Ende des strings zeigen, sonst Fehler!
	return (end-start==(int)aValue.size());
}

//----------------------------------------------------------------------------

long long GetTime_us()
{
	timeval tv;
	gettimeofday(&tv,NULL);
	return ((long long) tv.tv_sec)*1000000LL+tv.tv_usec;
}

//----------------------------------------------------------------------------

int strprintf(std::string & aStr,const char* format, ...)
{
	int rc;
	va_list paramList;
	va_start(paramList, format);
	rc = strvprintf(aStr, format, paramList);
	va_end(paramList);
	return rc;
}

//----------------------------------------------------------------------------

std::string strprintf(const char* format, ...)
{
	std::string text;
	va_list paramList;
	va_start(paramList, format);
	strvprintf(text, format, paramList);
	va_end(paramList);
	return text;
}

//----------------------------------------------------------------------------

int strvprintf(std::string & aStr,const char* format, va_list paramList)
{
	va_list copy;
	va_copy(copy,paramList);
	int size = vsnprintf(NULL, 0, format, copy);
	va_end(copy);
	if (!size)
	{
		aStr="";
		return 0;
	}
	char * ar=new char[size+2];
	va_copy(copy,paramList);
	// Unter gcc scheint size eins zu klein zu sein. Im Borland-Compiler
	// ist alles OK
	size=vsnprintf(ar, size+1, format, paramList);
	va_end(copy);
	aStr=ar;
	delete[]ar;
	return size;
}

//----------------------------------------------------------------------------




