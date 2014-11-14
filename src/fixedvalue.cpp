/*
 * Implementation of FixedValue class.
 * Author: Seb James
 * Date: Nov 2014
 */

#include <string>
#include <sstream>
#include <ostream>
#include <stdexcept>
#include "fixedvalue.h"
#include "rapidxml.hpp"

using namespace std;
using namespace spineml;
using namespace rapidxml;

FixedValue::FixedValue(xml_node<>* fv_node, const unsigned int num_in_pop)
    : PropertyContent (fv_node, num_in_pop)
{
    // Get fixed value from node.
    xml_attribute<>* val_str_attr;
    if ((val_str_attr = fv_node->first_attribute ("value"))) {
        std::stringstream val_ss;
        val_ss << val_str_attr->value();
        val_ss >> this->value;
    } // else value stays empty
}

FixedValue::FixedValue()
    : PropertyContent ()
{
}

void
FixedValue::writeVLBinaryData (ostream& f)
{
    for (unsigned int i = 0; i<this->numInPopulation; ++i) {
        f.write (reinterpret_cast<const char*>(&i), sizeof(unsigned int));
        f.write (reinterpret_cast<const char*>(&this->value), sizeof(double));
    }
}

void
FixedValue::writeULPropertyValue (xml_document<>* the_doc,
                                  xml_node<>* into_node)
{
    if (!into_node) {
        throw runtime_error ("FixedValue::writeULPropertyValue: target node is null");
        return;
    }
    if (!the_doc) {
        throw runtime_error ("FixedValue::writeULPropertyValue: doc is null");
    }
    // Allocate new fixed value node
    xml_node<>* fv_node = the_doc->allocate_node (node_element, "FixedValue");
    // Allocate and append an attribute
    stringstream val_ss;
    val_ss << this->value;
    char* val_alloced = the_doc->allocate_string (val_ss.str().c_str());
    xml_attribute<>* value_attr = the_doc->allocate_attribute ("value", val_alloced);
    fv_node->append_attribute (value_attr);
    // Add the fixed value node to the into_node (A UL:Property)
    into_node->prepend_node (fv_node);
}

void
FixedValue::setValue (const double& v)
{
    this->value = v;
}
