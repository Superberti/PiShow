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
#include <vector>

// -----------------------------------------------------------
// -----------------------------------------------------------
// -----------------------------------------------------------

/// Parse-Funktion für Zeilen. Kommt auch mit
/// in Anführungszeichen stehenden Parametern
/// (also solche mit Whitespace) zurecht
std::vector<std::string> ParseLine(const std::string& line)
{
	std::vector<std::string> result;
	const char delim=' ';
	std::string CurrentToken;
	char CurrentChar;
	const unsigned int LineSize=line.size();
	bool qm_on=false; // state quotation mark
	for (unsigned int i=0;i<LineSize;i++)
	{
		CurrentChar=line[i];
		if (i==LineSize-1)
		{
			if (CurrentChar!=delim && CurrentChar!='"')
				CurrentToken+=CurrentChar;
			if (CurrentToken.size())
			{
				result.push_back(CurrentToken);
				CurrentToken="";
			}
		}
		if (CurrentChar=='"')
		{
			qm_on=!qm_on;
			continue;
		}
		if (qm_on)
		{
			CurrentToken+=CurrentChar;
		}
		else if (CurrentChar==delim)
		{
			if (CurrentToken.size())
			{
				result.push_back(CurrentToken);
				CurrentToken="";
			}
		}
		else
			CurrentToken+=CurrentChar;
	}
	return result;
}

//----------------------------------------------------------------------------

void KillReturnAndEndl(char * MyString)
{
	const int s=strlen(MyString);
	for (int i=0;i<s;i++)
	{
		if (MyString[i]=='\n' || MyString[i]=='\r')
		{
			MyString[i]=0;
			break;
		}
	}
	return;
}

//----------------------------------------------------------------------------

// Eigene Exception-Klasse
TPiShowErr::TPiShowErr(const std::string & what)
	: mErrorCode(0)
{
	what_=what;
}

TPiShowErr::TPiShowErr(const std::string & what, const int aErrorCode)
	: mErrorCode(aErrorCode)
{
	what_=what;
}

TPiShowErr::TPiShowErr()
	: mErrorCode(0)
{}

TPiShowErr::TPiShowErr(const TPiShowErr& rhs)
{
	*this = rhs;
}

//----------------------------------------------------------------------------

TOSErr::TOSErr(const std::string & what, const int aOSErrorCode)
{
	mErrorCode=aOSErrorCode;
	what_=what+" "+GetOsErrString(aOSErrorCode);
}

TOSErr::TOSErr(const int aOSErrorCode)
{
	mErrorCode=aOSErrorCode;
	what_=GetOsErrString(aOSErrorCode);
}

//----------------------------------------------------------------------------

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

bool HexStringToInt(const std::string aValue, int & aNumber)
{
	const char * start=aValue.c_str();
	char * end=NULL;
	aNumber=strtol(start,&end,16);
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

timespec ts_diff(const timespec & start, const timespec & end)
{
	timespec temp;
	if ((end.tv_nsec-start.tv_nsec)<0)
	{
		temp.tv_sec = end.tv_sec-start.tv_sec-1;
		temp.tv_nsec = 1000000000+end.tv_nsec-start.tv_nsec;
	}
	else
	{
		temp.tv_sec = end.tv_sec-start.tv_sec;
		temp.tv_nsec = end.tv_nsec-start.tv_nsec;
	}
	return temp;
}

//----------------------------------------------------------------------------

void ts_add(timespec & MyTime, const timespec & MyAddend)
{
	if ((MyTime.tv_nsec+MyAddend.tv_nsec)>=1000000000)
	{
		MyTime.tv_sec+=MyAddend.tv_sec+1;
		MyTime.tv_nsec = MyTime.tv_nsec+MyAddend.tv_nsec-1000000000;
	}
	else
	{
		MyTime.tv_sec += MyAddend.tv_sec;
		MyTime.tv_nsec += MyAddend.tv_nsec;
	}
}

bool VerboseLogging=false;

void DebugOut(std::string aOut)
{
  if(!VerboseLogging)
    return;
  fprintf(stdout, aOut.c_str());
}
