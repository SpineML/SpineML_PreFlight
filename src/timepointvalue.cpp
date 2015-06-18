/*
 * Implementation of TimePointValue class.
 * Author: Seb James
 * Date: June 2015
 */

#include <string>
#include <sstream>
#include <ostream>
#include <stdexcept>
#include "timepointvalue.h"
#include "rapidxml.hpp"

using namespace std;
using namespace spineml;
using namespace rapidxml;

TimePointValue::TimePointValue(xml_node<>* tpv_node)
{
    // Get the "value" from the TimePointValue node.
    xml_attribute<>* val_str_attr;
    if ((val_str_attr = tpv_node->first_attribute ("value"))) {
        std::stringstream val_ss;
        val_ss << val_str_attr->value();
        val_ss >> this->value;
    } // else value stays empty

    // Get the "time" from the TimePointValue node.
    xml_attribute<>* time_str_attr;
    if ((time_str_attr = tpv_node->first_attribute ("time"))) {
        std::stringstream time_ss;
        time_ss << time_str_attr->value();
        time_ss >> this->time;
    } // else time stays empty
}

TimePointValue::TimePointValue()
{
}

void
TimePointValue::writeXML (xml_document<>* the_doc, xml_node<>* into_node)
{
    if (!into_node) {
        throw runtime_error ("TimePointValue::writeXML: target node is null");
        return;
    }
    if (!the_doc) {
        throw runtime_error ("TimePointValue::writeXML: doc is null");
    }
    // Allocate new fixed value node
    xml_node<>* tpv_node = the_doc->allocate_node (node_element, "TimePointValue");
    // Allocate and append time and value attributes
    stringstream val_ss;
    val_ss << this->value;
    char* val_alloced = the_doc->allocate_string (val_ss.str().c_str());
    xml_attribute<>* value_attr = the_doc->allocate_attribute ("value", val_alloced);
    tpv_node->append_attribute (value_attr);
    stringstream time_ss;
    time_ss << this->time;
    char* time_alloced = the_doc->allocate_string (time_ss.str().c_str());
    xml_attribute<>* time_attr = the_doc->allocate_attribute ("time", time_alloced);
    tpv_node->append_attribute (time_attr);
    // Add the TimePointValue node to the into_node (A TimeVaryingInput node)
    into_node->prepend_node (tpv_node);
}

void
TimePointValue::setValue (const double& v)
{
    this->value = v;
}

void
TimePointValue::setTime (const double& t)
{
    this->time = t;
}
