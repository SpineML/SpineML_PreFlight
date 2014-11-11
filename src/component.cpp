#include <stdexcept>
#include <string>
#include <sstream>

#include "component.h"

using namespace std;
using namespace rapidxml;
using namespace spineml;

void
Component::read (void)
{
    // Read data with allocandread object:
    string filepath = this->dir + this->name + ".xml";
    this->xmlraw.read (filepath);

    this->doc = new xml_document<>;
    this->doc->parse<parse_declaration_node | parse_no_data_nodes>(this->xmlraw.data());

    // Get the root node.
    this->root_node = this->doc->first_node ("SpineML");
    if (!this->root_node) {
        // Possibly look for HL:SpineML, if we have a high level model (not
        // used by anyone at present).
        stringstream ee;
        ee << "spineml::Component: No SpineML node in component " << this->name;
        throw runtime_error (ee.str());
    }

    // Now extract the information we want from this component xml
    // file; type, name (for verification), state variables (and if we
    // needed them, parameters, but we don't need those, so they're
    // ignored).
    this->readNameAndType();
    this->readStateVariables();

    // Free up this->doc.
    delete this->doc;
}

void
Component::getClassNode (void)
{
    if (this->class_node == (xml_node<>*)0) {
        this->class_node = this->root_node->first_node("ComponentClass");
        if (!this->class_node) {
            throw runtime_error ("spineml::Component: No ComponentClass node in xml");
        }
    }
}

void
Component::readNameAndType (void)
{
    this->getClassNode();
    xml_attribute<>* type_attr = this->class_node->first_attribute("type");
    if (type_attr) {
        this->type = type_attr->value();
    } else {
        throw runtime_error ("spineml::Component: No type attribute for ComponentClass");
    }
    xml_attribute<>* name_attr = this->class_node->first_attribute("name");
    if (name_attr) {
        string n = name_attr->value();
        if (this->name != n) {
            throw runtime_error ("spineml::Component: Failed to verify component name (no match)");
        }
    } else {
        throw runtime_error ("spineml::Component: Failed to verify component name (no name)");
    }
}

void
Component::readStateVariables (void)
{
    this->getClassNode();
    xml_node<>* dyn_node = this->class_node->first_node("Dynamics");
    if (!dyn_node) {
        throw runtime_error ("spineml::Component: No Dynamics node in xml");
    }
    xml_node<>* sv_node;
    for (sv_node = dyn_node->first_node("StateVariable");
         sv_node;
         sv_node = sv_node->next_sibling("StateVariable")) {
        this->readStateVariable (sv_node);
    }
}

void
Component::readStateVariable (const xml_node<>* sv_node)
{
    xml_attribute<>* name_attr = sv_node->first_attribute("name");
    if (!name_attr) {
        throw runtime_error ("spineml::Component: No name attribute for a StateVariable");
    }
    xml_attribute<>* dim_attr = sv_node->first_attribute("dimension");
    if (!dim_attr) {
        throw runtime_error ("spineml::Component: No dimension attribute for a StateVariable");
    }

    // Add to our map:
    this->stateVariables[name_attr->value()] = dim_attr->value();
}

std::string
Component::listStateVariables (void)
{
    string s("");
    map<string, string>::const_iterator svi = this->stateVariables.begin();
    while (svi != this->stateVariables.end()) {
        s += svi->first;
        s += ",";
        ++svi;
    }
    return s;
}

bool
Component::containsStateVariable (const string& sv)
{
    if (this->stateVariables.count (sv) > 0) {
        return true;
    }
    return false;
}
