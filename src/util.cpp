/*
 * Implementation of Util class.
 */

#include <string>
#include <utility>
#include <stdexcept>
#include <sstream>
#include "util.h"

using namespace std;
using namespace spineml;

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
    double val(0.0);
    stringstream val_ss;
    val_ss << valstring;        ;
    val_ss >> rtn.first;

    // Add dimension to rtn.
    rtn.second = dimstring;

    return rtn;
}
