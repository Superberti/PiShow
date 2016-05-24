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
//----------------------------------------------------------------------------


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


#endif
