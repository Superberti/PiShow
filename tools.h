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

/// String-Parsefunktion
/// \param line Zu parsende Zeile
/// \retval result Einzelne Bestandteile der Zeile
void ParseLine(const std::string& line, std::vector<std::string> & result);

/// Return and Endline aus einem C-String löschen
void KillReturnAndEndl(char * MyString);

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

/// Funktionen zum Trimmen eines std::strings

/// Links trimmen
std::string& trimleft( std::string& s );
/// Rechts trimmen
std::string& trimright( std::string& s );
/// Beidseitig trimmen
std::string& trim( std::string& s );
/// Beidseitig trimmen, Original nicht verändern
std::string trim( const std::string& s );

/// HexDump eines Speicherbereiches auf stdout ausgeben
/// \param Buffer Pointer auf Speicher
/// \param dwBytes Größe des Speicherbereiches
/// \param offset Offset zum Pointer
void HexDump(unsigned char * Buffer, const unsigned int dwBytes,const unsigned int offset=0);

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

/// Strings ersetzen
/// \param aString String, in dem etwas ersetzt werden soll
/// \param aToReplace Zeichen(kette), welche ersetzt werden soll
/// \param aReplaceWith Zeichen(kette), die als Ersatz dient
void ReplaceAll(std::string & aString, const std::string & aToReplace, const std::string & aReplaceWith);

/// Differenz von timespec-Strukturen bilden. Liefert end-start (Minuend-Subtrahent)
/// \param start Subtrahend
/// \param end Minuend
/// \return Differenz
timespec ts_diff(const timespec & start, const timespec & end);

/// Summe von zwei timespec-Strukturen bilden (MyTime+=MyAddend
/// \retval MyTime Wert der Summe
/// \param MyAddend Summand
void ts_add(timespec & MyTime, const timespec & MyAddend);

/// Thread-Safe Fehlernummer in Fehlernachricht umwandeln
/// \param aErrorNumber Betriebssystem-Fehlercode
std::string GetOsErrString(const int aErrorNumber);

/// Eine Zeile in der shell ausführen
/// \param aCmdLine In der shell auszuführende Kommandozeile
/// \param aInOut Je nach aInDirection der an die shell zu sendende Text aInDirection=true
/// ODER der Inhalt von stdout des shell-Kommandos (aInDirection=false)
/// \param aInDirection true:aInOut wird gesendet, false: in aInOut steht steht die Ausgabe
/// \return ExitCode des shell-Kommandos oder <0, wenn Fehler aufgetreten sind(0 wenn alles O.K.)
int ShellExecute(const std::string aCmdLine, std::string & aInOut, const bool aInDirection=false);

/// Umwandlung Little- nach Big-Endian und umgekehrt
unsigned int SwapEndian(const unsigned int x);

/// Abfrage der IP-Adresse.
/// @param aInterface Interface-Name.
std::string GetIPAddress(const std::string & aInterface);

/// Abfrage der Netmask.
/// @param aInterface Interface-Name.
std::string GetNetMask(const std::string & aInterface);

/// Abfragen des Hostnames vom System.
/// @return Hostname des Systems.
std::string Hostname();

/// Überprüfen des übergebenen Hostnamens auf korrekte Zeichen.
/// Ein Hostname muß zwischen 1 und 15 Zeichen lang sein und darf
/// die Zeichen A-Z, a-z, 0-9 und den Bindestrich (-) enthalten.
/// Ferner kann als erstes Zeichen nur ein Buchstabe verwendet werden.
/// @param aHostname Hostname, der überprüft werden soll.
/// @return Liefert true zurück, wenn der übergebene Hostname OK ist.
bool CheckHostname(const std::string & aHostname);

/// Ueberprueft ob die uebergebene IP-Adresse formal korrekt ist.
/// @param aIPAddress IP-Adresse als Zeichenkette.
/// @return Liefert true zurueck, wenn die IP-Adresse formal korrekt ist.
bool CheckIP(const std::string & aIPAddress);

class ValueList
{
  private:
    std::vector<std::string> mIdents;
    std::vector<std::string> mValues;
    void EatToken(const std::string & aToken);
    void Parse(const std::string & aLine, const char aSeparator);
  public:
    ValueList(const std::string & aValueStringList, const char aSeparator);
    std::string GetValue(const std::string & aIdent) const;
    void SetValue(const std::string & aIdent, const std::string & aValue);
    std::string ToString(const char aSeparator) const;
};

/// CCITT-CRC eines Char-Arrays berechnen
/// \param data_ptr Pointer auf Char-Array
/// \param data_len Länge des Arrays
unsigned short compute_crc(unsigned char *data_ptr, unsigned short data_len);

#endif
