/*
 * Implementation of TimePointArrayValue class.
 * Author: Seb James
 * Date: January 2016
 */

#include <string>
#include <sstream>
#include <ostream>
#include <stdexcept>
#include "timepointarrayvalue.h"
#include "rapidxml.hpp"

using namespace std;
using namespace spineml;
using namespace rapidxml;

TimePointArrayValue::TimePointArrayValue(xml_node<>* tpv_node)
{
    // Get the "array_value" from the TimePointArrayValue node.
    xml_attribute<>* val_str_attr;
    if ((val_str_attr = tpv_node->first_attribute ("array_value"))) {
        std::stringstream val_ss;
        val_ss << val_str_attr->value();
        this->array_value = val_ss.str();
    } // else array_value stays empty

    // Get the "array_time" from the TimePointArrayValue node.
    xml_attribute<>* time_str_attr;
    if ((time_str_attr = tpv_node->first_attribute ("array_time"))) {
        std::stringstream time_ss;
        time_ss << time_str_attr->value();
        this->array_time = time_ss.str();
    } // else time stays empty
}

TimePointArrayValue::TimePointArrayValue()
{
}

void
TimePointArrayValue::writeXML (xml_document<>* the_doc, xml_node<>* into_node)
{
    if (!into_node) {
        throw runtime_error ("TimePointArrayValue::writeXML: target node is null");
        return;
    }
    if (!the_doc) {
        throw runtime_error ("TimePointArrayValue::writeXML: doc is null");
    }
    // Allocate new time point array value node
    xml_node<>* tpav_node = the_doc->allocate_node (node_element, "TimePointArrayValue");
    // Allocate and append time and value attributes
    stringstream idx_ss;
    idx_ss << this->index;
    char* idx_alloced = the_doc->allocate_string (idx_ss.str().c_str());
    xml_attribute<>* idx_attr = the_doc->allocate_attribute ("index", idx_alloced);
    tpav_node->append_attribute (idx_attr);
    char* val_alloced = the_doc->allocate_string (this->array_value.c_str());
    xml_attribute<>* value_attr = the_doc->allocate_attribute ("array_value", val_alloced);
    tpav_node->append_attribute (value_attr);
    char* time_alloced = the_doc->allocate_string (this->array_time.c_str());
    xml_attribute<>* time_attr = the_doc->allocate_attribute ("array_time", time_alloced);
    tpav_node->append_attribute (time_attr);
    // Add the TimePointArrayValue node to the into_node (A TimeVaryingInput node)
    into_node->prepend_node (tpav_node);
}

void
TimePointArrayValue::setArrayValue (const string& vs)
{
    this->array_value = vs;
}

void
TimePointArrayValue::setArrayTime (const string& ts)
{
    this->array_time = ts;
}

void
TimePointArrayValue::setIndex (const unsigned int& i)
{
    this->index = i;
}

void
TimePointArrayValue::setIndex (const string& s)
{
    stringstream idxss;
    idxss << s;
    idxss >> this->index;
}
