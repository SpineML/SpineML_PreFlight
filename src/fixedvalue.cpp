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

void
FixedValue::writeVLBinaryData (ostream& f)
{
    for (unsigned int i = 0; i<this->numInPopulation; ++i) {
        f.write (reinterpret_cast<const char*>(&i), sizeof(unsigned int));
        f.write (reinterpret_cast<const char*>(&this->value), sizeof(double));
    }
}
