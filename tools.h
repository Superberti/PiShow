//----------------------------------------------------------------------------
#ifndef TOOLS_H
#define TOOLS_H
//----------------------------------------------------------------------------
#include <exception>
#include <string>
#include <errno.h>
#include <vector>
#include <sstream>
#include <limits.h>
#include <stdarg.h>

extern bool VerboseLogging;

/// String-Parsefunktion
/// \param line Zu parsende Zeile
/// \return result Einzelne Bestandteile der Zeile
std::vector<std::string> ParseLine(const std::string& line);

/// Return and Endline aus einem C-String löschen
void KillReturnAndEndl(char * MyString);

//----------------------------------------------------------------------------

/// Eigene Fehlerklasse
class TPiShowErr : public std::exception
{
	protected:
		std::string what_;	///< Fehlertext
		int mErrorCode;			///< Fehlercode
	public:
		TPiShowErr();
		TPiShowErr(const TPiShowErr& rhs);
		TPiShowErr(const std::string & what, const int aErrorCode);
		TPiShowErr(const std::string & what);
		~TPiShowErr() throw(){};
		const char* what() const throw() { return what_.c_str(); };
		int ErrorCode() const { return mErrorCode; };
};

class TOSErr : public TPiShowErr
{
	public:
		TOSErr(const std::string & what, const int aErrorCode);
		TOSErr(const int aOSErrorCode);
};

// mal eben schnell eine Zahl in einen String umwandeln...
template<class T>std::string ToString(T Value){std::stringstream ss;ss<<Value;return ss.str();};

/// string -> double. Es wird erwartet, dass im String nur alphanumerische Zeichen und der Punkt
/// verwendet werden!
/// \param aValue Zahlenstring
/// \retval aNumber Extrahierter numerischer Wert
/// \return false: Umwandlung nicht erfolgreich
bool StringToDouble(const std::string aValue, double & aNumber);

/// string -> integer. Es wird erwartet, dass im String nur alphanumerische verwendet werden!
/// \param aValue Zahlenstring
/// \retval aNumber Extrahierter numerischer Wert
/// \return false: Umwandlung nicht erfolgreich
bool StringToInt(const std::string aValue, int & aNumber);

/// Hex-string -> integer. Es wird erwartet, dass im String nur hexadezimale Zahlen verwendet werden (OHNE 0x vorweg)
/// \param aValue Zahlenstring
/// \retval aNumber Extrahierter numerischer Wert
/// \return false: Umwandlung nicht erfolgreich
bool HexStringToInt(const std::string aValue, int & aNumber);

/// Aktuelle Zeit in Mikrosekunden
long long GetTime_us();

/// In einen std::string mit printf drucken. Die Größe wird automatisch
/// angepasst. Oft sehr nützlich!
/// \retval aStr String, in den die Ausgabe erfolgen soll
/// \param format ... Format wie bei printf gewohnt und evtl. zusätzliche Parameter
/// \return Länge des Strings
int strprintf(std::string & aStr,const char* format, ...);

/// std::string mit printf erstellen. Die Größe wird automatisch angepasst.
/// \param format ... Format wie bei printf gewohnt und evtl. zusätzliche Parameter
/// \return String mit printf-Ausgabe
std::string strprintf(const char* format, ...);

/// Hilfsfunktion für strprintf
int strvprintf(std::string & aStr,const char* format, va_list paramList);

/// Thread-Safe Fehlernummer in Fehlernachricht umwandeln
/// \param aErrorNumber Betriebssystem-Fehlercode
std::string GetOsErrString(const int aErrorNumber);

/// Differenz von timespec-Strukturen bilden. Liefert end-start (Minuend-Subtrahent)
/// \param start Subtrahend
/// \param end Minuend
/// \return Differenz
timespec ts_diff(const timespec & start, const timespec & end);

/// Summe von zwei timespec-Strukturen bilden (MyTime+=MyAddend
/// \retval MyTime Wert der Summe
/// \param MyAddend Summand
void ts_add(timespec & MyTime, const timespec & MyAddend);


#endif
