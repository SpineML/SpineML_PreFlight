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
     * A class to represent a Component - stores relevant information
     * from the component's XML file for later use. Only stores what
     * is relevant to preflighting.
     */
    class Component
    {
    public:
        Component(const std::string& d, const std::string& n)
            : dir(d)
            , name(n)
            , root_node((rapidxml::xml_node<>*)0)
            , class_node((rapidxml::xml_node<>*)0) { this->read(); }
        ~Component() {}

        /*!
         * Output a comma separated list of the state variable names.
         */
        std::string listStateVariables (void);

        /*!
         * Return true if this component contains the passed in state
         * variable name.
         */
        bool containsStateVariable (const std::string& sv);

    private:

        /*!
         * Read the component data from the file, assumed to be in
         * ./@name.xml
         */
        void read (void);

        /*!
         * If the ComponentClass node hasn't be found and stored in
         * this->class_node, then do so.
         */
        void getClassNode (void);

        /*!
         * Read this->type and verify this->name from xml.
         */
        void readNameAndType (void);

        /*!
         * Read state variables and their dimensions.
         */
        void readStateVariables (void);

        /*!
         * Read the single state variable pointed to by sv_node and
         * add it to this->stateVariables.
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
         * The map of parameter names to their dimensions for this
         * component.
         */
        std::map<std::string, std::string> parameters;

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
