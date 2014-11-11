/*
 * Implementation of ValueList class.
 * Author: Seb James
 * Date: Nov 2014
 */

#include <string>
#include <sstream>
#include <ostream>
#include <stdexcept>
#include <map>
#include "valuelist.h"
#include "rapidxml.hpp"

using namespace std;
using namespace spineml;
using namespace rapidxml;

ValueList::ValueList(xml_node<>* vl_node, const unsigned int num_in_pop)
    : PropertyContent (vl_node, num_in_pop)
{
    // Read values from Value nodes enclosed by the value list.
    xml_node<>* v_node;
    xml_attribute<>* attr;
    int index = 0;
    double value = 0.0;

    v_node = vl_node->first_node("BinaryFile");
    if (v_node) {
        // We have a ValueList containing a BinaryFile element. In
        // this case, the "writeVLBinary" code should do nothing.
        this->alreadyBinary = true;
    }

    for (v_node = vl_node->first_node("Value");
         v_node;
         v_node = v_node->next_sibling("Value")) {

        if ((attr = v_node->first_attribute ("index"))) {
            std::stringstream ss;
            ss << attr->value();
            ss >> index;
        } else {
            throw runtime_error ("ValueList: Badly formed ValueList; no index.");
        }

        if ((attr = v_node->first_attribute ("value"))) {
            std::stringstream ss;
            ss << attr->value();
            ss >> value;
        } else {
            throw runtime_error ("ValueList: Badly formed ValueList; no value.");
        }

        this->values.insert (make_pair (index, value));
    }
}

void
ValueList::writeVLBinaryData (ostream& f)
{
    map<int, double>::const_iterator i = this->values.begin();
    while (i != this->values.end()) {
        f.write (reinterpret_cast<const char*>(&i->first), sizeof(unsigned int));
        f.write (reinterpret_cast<const char*>(&i->second), sizeof(double));
        ++i;
    }
}
