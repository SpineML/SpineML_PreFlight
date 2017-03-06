/*
 * Implementation of UniformDistribution class.
 * Author: Seb James
 * Date: Nov 2014
 */

#include <string>
#include <vector>
#include <sstream>
#include <ostream>
#include <stdexcept>
#include "uniformdistribution.h"
#include "rapidxml.hpp"
#include "rng.h"
#include "util.h"

using namespace std;
using namespace spineml;
using namespace rapidxml;

UniformDistribution::UniformDistribution(xml_node<>* ud_node, const unsigned int num_in_pop)
    : PropertyContent (ud_node, num_in_pop)
    , minimum (0.0)
    , maximum (1.0)
    , seed (123)
{
    // Get distribution parameters from node.
    xml_attribute<>* attr;
    if ((attr = ud_node->first_attribute ("minimum"))) {
        std::stringstream ss;
        ss << attr->value();
        ss >> this->minimum;
    } // else minimum remains 0.

    if ((attr = ud_node->first_attribute ("maximum"))) {
        std::stringstream ss;
        ss << attr->value();
        ss >> this->maximum;
    } // else maximum remains 1.

    if ((attr = ud_node->first_attribute ("seed"))) {
        std::stringstream ss;
        ss << attr->value();
        ss >> this->seed;
    } // else seed remains 123.
}

UniformDistribution::UniformDistribution()
    : PropertyContent ()
{
}

void
UniformDistribution::writeVLBinaryData (ostream& f)
{
    RngData rngData;
    rngDataInit (&rngData);
    zigset (&rngData, static_cast<unsigned int>(this->seed+1));
    rngData.seed = static_cast<int>(this->seed);

    for (unsigned int i = 0; i<this->numInPopulation; ++i) {
        // Write out values from a uniform distribution
        double val = _randomUniform(&rngData) * (this->maximum - this->minimum) + this->minimum;
        f.write (reinterpret_cast<const char*>(&i), sizeof(unsigned int));
        f.write (reinterpret_cast<const char*>(&val), sizeof(double));
    }
}

void
UniformDistribution::writeULPropertyValue (xml_document<>* the_doc,
                                           xml_node<>* into_node)
{
    if (!into_node) {
        throw runtime_error ("UniformDistribution::writeULPropertyValue: target node is null");
        return;
    }
    if (!the_doc) {
        throw runtime_error ("UniformDistribution::writeULPropertyValue: doc is null");
    }
    // Allocate new fixed value node
    xml_node<>* ud_node = the_doc->allocate_node (node_element, "UL:UniformDistribution");

    // Allocate and append attributes minimum, maximum and seed.
    stringstream min_ss;
    min_ss << this->minimum;
    char* min_alloced = the_doc->allocate_string (min_ss.str().c_str());
    xml_attribute<>* min_attr = the_doc->allocate_attribute ("minimum", min_alloced);
    ud_node->append_attribute (min_attr);

    stringstream max_ss;
    max_ss << this->maximum;
    char* max_alloced = the_doc->allocate_string (max_ss.str().c_str());
    xml_attribute<>* max_attr = the_doc->allocate_attribute ("maximum", max_alloced);
    ud_node->append_attribute (max_attr);

    stringstream seed_ss;
    seed_ss << this->seed;
    char* seed_alloced = the_doc->allocate_string (seed_ss.str().c_str());
    xml_attribute<>* seed_attr = the_doc->allocate_attribute ("seed", seed_alloced);
    ud_node->append_attribute (seed_attr);

    // Add the uniform distribution node to the into_node (A UL:Property)
    into_node->prepend_node (ud_node);
}

void
UniformDistribution::setFromString (const string& str)
{
    // String should look like UNI(1,2,124)
    if (str.find ("UNI(") != 0) {
        string emsg = string("'") + str + "' is an invalid uniform distribution specification string.";
        throw runtime_error (emsg);
    }
    // Get contents of brackets.
    string::size_type p1 = str.find ("(");
    string::size_type p2 = str.find (")");
    if (p1 == string::npos || p2 == string::npos || p2 < p1) {
        string emsg = string("'") + str + "' is an invalid uniform distribution specification string.";
        throw runtime_error (emsg);
    }
    ++p1; --p2;
    string vals = str.substr (p1, p2-p1+1);

    vector<string> vs = Util::stringToVector (vals, ",");
    stringstream minss;
    minss << vs[0];
    minss >> this->minimum;
    stringstream maxss;
    maxss << vs[1];
    maxss >> this->maximum;
    stringstream seedss;
    seedss << vs[2];
    seedss >> this->seed;
}
