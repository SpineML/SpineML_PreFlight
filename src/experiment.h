/*!
 * An experiment.xml reading class.
 *
 * Author: Seb James
 * Date: Oct 2014
 */

#ifndef _EXPERIMENT_H_
#define _EXPERIMENT_H_

#include <string>
#include <vector>
#include <stdexcept>
#include <utility>
#include "rapidxml.hpp"

namespace spineml
{
    /*!
     * A class to read the parameters stored in the SpineML
     * experiment.xml file.
     *
     * This class also handles the task of reading experiment.xml and
     * then adding "Model Configurations" to experiment.xml to
     * override parameter or state variable initial values at model
     * execution time. This allows spineml_preflight to take command
     * line options to change parameter/state variable properties.
     */
    class Experiment
    {
    public:
        /*!
         * Constructors
         */
        //@{
        Experiment();
        Experiment(const std::string& path);
        //@}

        /*!
         * parse the experiment.xml file and populate the member
         * attributes.
         *
         * Note that this method instatiates its own xml_document, and
         * this xml_document exists only within the scope of this
         * method. If this->addPropertyChangeRequest is called, then a
         * new xml_document is instatiated within the scope of that
         * method.
         */
        void parse (void);

        /*!
         * Accessors
         */
        //@{
        double getSimDuration (void);
        double getSimFixedDt (void);
        std::string modelUrl (void);
        //@}

        /*!
         * Return the simulation (fixed) solver rate in s^-1.
         */
        double getSimFixedRate (void);

        /*!
         * Add the raw property change request, as provided on the
         * command line. If the request has a bad format, throw an exception.
         *
         * This method splits up the single command line option passed
         * as @pcrequest. It then checks in the model.xml to ensure
         * that the requested property exists. If this turns out to be
         * the case, it then calls this->insertModelConfig to add the
         * property change/model configuration update.
         */
        void addPropertyChangeRequest (const std::string& pcrequest);

        /*!
         * This actually does the work of inserting a new
         * Configuration node to the experiment's Model node. Its
         * passed a vector of 3 elements: target population, target
         * property and new value which have been extracted from a
         * command line argument.
         */
        void insertModelConfig (rapidxml::xml_node<>* property_node,
                                const std::vector<std::string>& elements);

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
         * \param the escape character. If not set to \0, then
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
         * Strip any occurrences of the characters in charList from
         * input. Used by splitStringWithEncs.
         */
        //@{
        static int stripChars (std::string& input, const std::string& charList);
        static int stripChars (std::string& input, const char charList);
        //@}

        //! A simple accessor for this->modelDir.
        void setModelDir (const std::string& dir);

    private:
        //! write thedoc out to file.
        void write (const rapidxml::xml_document<>& the_doc);

        //! Given a @str like "25.3ms", place 25.3 in pair->first and
        //! "ms" in pair->second and return the pair.
        std::pair<double, std::string> getValueWithDimension (const std::string& str);

        //! Path to the experiment xml file.
        std::string filepath;

        //! Model network layer path.
        std::string network_layer_path;

        //! Simulation duration in seconds
        double simDuration;

        //! Simulation fixed time step in *seconds*. (note: this is
        //! stored in experiment.xml in ms).
        double simFixedDt;

        //! Simulation solver type
        std::string simType;

        //! The location of experiment.xml and model.xml
        std::string modelDir;

        /*
         * Could also add time varying inputs and log output fields if
         * they were required (which they're not)
         */
    };

} // namespace spineml

/*!
 * Templated function splitStringWithEncs implementation.
 */
//@{
template <typename strType>
std::vector<strType>
spineml::Experiment::splitStringWithEncs (const strType& s,
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
                // FIXME: Check we didn't find an escaped enclosureChar.
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
        Experiment::stripChars (entry, escapeChar);
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
//@}

#endif // _EXPERIMENT_H_
