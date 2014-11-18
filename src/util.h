/*
 * A utility class.
 */

#ifndef _UTIL_H_
#define _UTIL_H_

#include <string>
#include <vector>
#include <utility>

namespace spineml
{
    /*!
     * A collection of static, utility functions used in
     * spineml_preflight.
     */
    class Util
    {
    public:

        /*!
         * Remove the file from a path. Modifies @param unixPath  If
         * unixPath is "/path/to/some/file.txt" before calling, then
         * afterwards, it will be "/path/to/some"
         */
        static void stripUnixFile (std::string& unixPath);

        /*!
         * Given a path like /path/to/file in str, remove all
         * the preceding /path/to/ stuff to leave just the
         * filename.
         */
        static void stripUnixPath (std::string& unixPath);

        /*!
         * A little utility. Given a @param unixPath containing
         * "blah.xml", change unixPath to "blah".
         */
        static void stripFileSuffix (std::string& unixPath);

        /*!
         * Strip any occurrences of the characters in charList from
         * input. Used by splitStringWithEncs.
         */
        //@{
        static int stripChars (std::string& input, const std::string& charList);
        static int stripChars (std::string& input, const char charList);
        //@}

        /*!
         * This splits up a "search style" string into tokens.
         *
         * \param s The string to split up
         *
         * \param separatorChars The chars used only to
         * separate tokens (" ,;")
         *
         * \param enclosureChars The characters used to
         * enclose a multi-word token ("\"\'")
         *
         * \param escapeChar the escape character. If not set to \0, then
         * this is the character used to escape the enclosure
         * chars.
         */
        //@{
        template <typename strType>
        static std::vector<strType> splitStringWithEncs (const strType& s,
                                                         const strType& separatorChars = strType(":"),
                                                         const strType& enclosureChars = strType("\"'"),
                                                         const typename strType::value_type& escapeChar = typename strType::value_type(0));
        //@}

        /*!
         * Given a @param str like "25.3ms", place 25.3 in pair->first
         * and "ms" in pair->second and return the pair.
         *
         * @return a pair containing double value and dimension string.
         */
        static std::pair<double, std::string> getValueWithDimension (const std::string& str);

    }; // utility class
} // namespace

/*
 * Templated function splitStringWithEncs implementation.
 */
template <typename strType>
std::vector<strType>
spineml::Util::splitStringWithEncs (const strType& s,
                                    const strType& separatorChars,
                                    const strType& enclosureChars,
                                    const typename strType::value_type& escapeChar) // or '\0'
{
    // Run through the string, searching for separator and
    // enclosure chars and finding tokens based on those.

    std::vector<strType> theVec;
    strType entry("");
    typename strType::size_type a=0, b=0, c=0;
    strType sepsAndEncsAndEsc = separatorChars + enclosureChars;
    sepsAndEncsAndEsc += escapeChar;

    typename strType::size_type sz = s.size();
    while (a < sz) {

        // If true, then the thing we're searching for is an enclosure
        // char, otherwise, it's a separator char.
        bool nextIsEnc(false);
        typename strType::value_type currentEncChar = 0;

        if (a == 0) { // First field.
            if (escapeChar && s[a] == escapeChar) {
                // First char is an escape char - skip this and next
                ++a; ++a;
                continue;
            } else if ((enclosureChars.find_first_of (static_cast<typename strType::value_type>(s[a]), 0)) != strType::npos) {
                // First char is an enclosure char.
                nextIsEnc = true;
                currentEncChar = s[a];
                ++a; // Skip the enclosure char
            } else if ((separatorChars.find_first_of (static_cast<typename strType::value_type>(s[a]), 0)) != strType::npos) {
                // First char is a ',' This special case means that we insert an entry for the current ',' and step past it.
                theVec.push_back ("");
                ++a;
            } // else first char is a normal char or a separator.

        } else { // Not first field

            if ((a = s.find_first_of (sepsAndEncsAndEsc, a)) == strType::npos) {
                // No enclosure, separator or escape chars in string
                theVec.push_back (s);
                return theVec;
            }

            else if (escapeChar && s[a] == escapeChar) {
                // it's an escape char - skip this and next
                ++a; ++a;
                continue;
            } else if ((enclosureChars.find_first_of (static_cast<typename strType::value_type>(s[a]), 0)) != strType::npos) {
                // it's an enclosure char.
                nextIsEnc = true;
                currentEncChar = s[a];
                ++a; // Skip the enclosure char
            } else if ((separatorChars.find_first_of (static_cast<typename strType::value_type>(s[a]), 0)) != strType::npos) {
                // It's a field separator
                ++a; // Skip the separator
                if (a >= sz) {
                    // Special case - a trailing separator character - add an empty
                    // value to the return vector of tokens.
                    theVec.push_back ("");
                } else {
                    // a < sz, so now check if we've hit an escape char
                    if ((enclosureChars.find_first_of (static_cast<typename strType::value_type>(s[a]), 0)) != strType::npos) {
                        // Enclosure char following sep char
                        nextIsEnc = true;
                        currentEncChar = s[a];
                        ++a; // Skip the enclosure char
                    }
                }
            } else {
                throw std::runtime_error ("splitStringWithEncs: Unexpected case");
            }
        }

        // Check we didn't over-run
        if (a >= sz) { break; }

        // Now get the token
        typename strType::size_type range = strType::npos;
        if (nextIsEnc) {
            //DBG2 ("Searching for next instances of enc chars: >" << enclosureChars << "< ");
            c = a;
            while ((b = s.find_first_of (currentEncChar, c)) != strType::npos) {
                // Check we didn't find an escaped enclosureChar.
                if (escapeChar) {
                    c = b; --c;
                    if (s[c] == escapeChar) {
                        // Skip b which is an escaped enclosure char
                        c = b; ++c;
                        continue;
                    }
                }
                range = b - a;
                break;
            }
        } else {
            // Search for next instances of sep chars starting from position a
            if ((b = s.find_first_of (separatorChars, a)) != strType::npos) {
                // Check it wasn't an escaped separator:
                if (escapeChar) {
                    c = b; --c;
                    if (c >= 0 && c != strType::npos && c < sz && s[c] == escapeChar) {
                        // Found escaped separator character
                        c = b; ++c;
                        continue;
                    }
                }
                range = b - a;
                // On finding a separator char at position b, have set range
            }
        }

        entry = s.substr (a, range);
        Util::stripChars (entry, escapeChar);
        theVec.push_back (entry);

        if (range != strType::npos) { // end of field was not end of string
            if (nextIsEnc) {
                a += range + 1; // +1 to take us past the closing enclosure.
            } else {
                a += range; // in new scheme, we want to find the separator, so this
                // places us ON the separator.
            }
        } else {
            a = range;
        }
    }

    return theVec;
}

#endif // _UTIL_H_
