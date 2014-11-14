/*!
 * Author: Seb James
 * Date: Nov 2014
 */

#ifndef _COMPONENT_H_
#define _COMPONENT_H_

#include <string>
#include <map>
#include "rapidxml.hpp"
#include "allocandread.h"

namespace spineml
{
    /*!
     * A class to represent a SpineML component - reads and then
     * stores relevant information from the component's XML file for
     * later use. It only stores what is relevant to preflighting.
     */
    class Component
    {
    public:
        /*!
         * Constructor takes a directory path @param d and a component name
         * @param n  The component XML file is expected to be called n.xml
         * and found in @param d
         */
        Component(const std::string& d, const std::string& n);

        /*!
         * Output a comma separated list of the state variable
         * names. @see stateVariables
         *
         * @return a comma separated list of the state variable names.
         */
        std::string listStateVariables (void);

        /*!
         * @return true if this component contains the passed in state
         * variable name @param sv
         */
        bool containsStateVariable (const std::string& sv);

    private:

        /*!
         * Read the component data from the file, assumed to be in
         * "name".xml (@see name)
         */
        void read (void);

        /*!
         * If the ComponentClass node hasn't been found and stored in
         * @see class_node, then do so.
         */
        void getClassNode (void);

        /*!
         * Read @see type and verify @see name from xml.
         */
        void readNameAndType (void);

        /*!
         * Read state variables and their dimensions.
         */
        void readStateVariables (void);

        /*!
         * Read the single state variable pointed to by @param sv_node and
         * add it to @see stateVariables.
         */
        void readStateVariable (const rapidxml::xml_node<>* sv_node);

        /*!
         * Directory containing the component's xml file. This should
         * include the trailing slash.
         */
        std::string dir;

        /*!
         * Component name, as given by the user.
         */
        std::string name;

        /*!
         * Component type; e.g. "neuron_body"
         */
        std::string type;

        /*!
         * The map of state variable names to their dimensions for
         * this component.
         */
        std::map<std::string, std::string> stateVariables;

        /*!
         * An object into which to read the xml text prior to parsing.
         */
        spineml::AllocAndRead xmlraw;

        /*!
         * the root node pointer.
         */
        rapidxml::xml_node<>* root_node;

        /*!
         * The node pointer for the ComponentClass element.
         */
        rapidxml::xml_node<>* class_node;

        /*!
         * The components xml_document object. Allocate only whilst
         * reading. Deallocate thereafter so that objects of type
         * Component can be copied.
         */
        rapidxml::xml_document<>* doc;
    };

} // namespace spineml

#endif // _COMPONENT_H_
