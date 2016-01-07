/*
 * Implementation of Util class.
 */

#include <string>
#include <utility>
#include <stdexcept>
#include <sstream>
#include <algorithm>
#include "util.h"

using namespace std;
using namespace spineml;

void
Util::stripUnixFile (std::string& unixPath)
{
    string::size_type pos (unixPath.find_last_of ('/'));
    if (pos != string::npos) {
        unixPath = unixPath.substr (0, pos);
    }
}

void
Util::stripUnixPath (std::string& unixPath)
{
    string::size_type pos (unixPath.find_last_of ('/'));

    if (pos != string::npos) {
        unixPath = unixPath.substr (++pos);
    }
}

void
Util::stripFileSuffix (string& unixPath)
{
    string::size_type pos (unixPath.rfind('.'));
    if (pos != string::npos) {
        // We have a '.' character
        string tmp (unixPath.substr (0, pos));
        if (!tmp.empty()) {
            unixPath = tmp;
        }
    }
}

int
Util::stripChars (std::string& input, const std::string& charList)
{
    int rtn(0);
    string::size_type pos(0);
    while ((pos = input.find_last_of (charList)) != string::npos) {
        input.erase (pos, 1);
        ++rtn;
    }
    return rtn;
}

int
Util::stripChars (std::string& input, const char charList)
{
    int rtn(0);
    string::size_type pos(0);
    while ((pos = input.find_last_of (charList)) != string::npos) {
        input.erase (pos, 1);
        ++rtn;
    }
    return rtn;
}

void
Util::conditionAsXmlTag (std::string& str)
{
    // 1) Replace chars which are disallowed in an XML tag
    string::size_type ptr = string::npos;

    // We allow numeric and alpha chars, the underscore and the
    // hyphen. colon strictly allowed, but best avoided.
    while ((ptr = str.find_last_not_of (CHARS_NUMERIC_ALPHA"_-", ptr)) != string::npos) {
        // Replace the char with an underscore:
        str[ptr] = '_';
        ptr--;
    }

    // 2) Check first 3 chars don't spell xml (in any case)
    string firstThree = str.substr (0,3);
    transform (firstThree.begin(), firstThree.end(),
               firstThree.begin(), spineml::to_lower());
    if (firstThree == "xml") {
        // Prepend '_'
        string newStr("_");
        newStr += str;
        str = newStr;
    }

    // 3) Prepend an '_' if initial char begins with a numeral or hyphen
    if (str[0] > 0x29 && str[0] < 0x3a) {
        // Prepend '_'
        string newStr("_");
        newStr += str;
        str = newStr;
    }
}

pair<double, string>
Util::getValueWithDimension (const std::string& str)
{
    // Two copies of str to work on.
    string valstring = str;
    string dimstring = str;

    // Remove numbers and whitespace from dimstring.
    {
        string charList("0123456789-+. \t\n\r");
        string::size_type pos(0);
        while ((pos = dimstring.find_last_of (charList)) != string::npos) {
            dimstring.erase (pos, 1);
        }
    }

    // Remove trailing non-numbers from valstring.
    {
        char c;
        string::size_type pos = valstring.size();
        while (pos > 0 &&
               ((c = valstring[pos-1]) != '.'
                && c != '+'
                && c != '-'
                && c != '0'
                && c != '1'
                && c != '2'
                && c != '3'
                && c != '4'
                && c != '5'
                && c != '6'
                && c != '7'
                && c != '8'
                && c != '9'
                   )) {
            pos--;
        }
        valstring.erase (pos);
    }

    // A return object.
    pair<double, string> rtn;

    // Add value to rtn.
    stringstream val_ss;
    val_ss << valstring;        ;
    val_ss >> rtn.first;

    // Add dimension to rtn.
    rtn.second = dimstring;

    return rtn;
}
