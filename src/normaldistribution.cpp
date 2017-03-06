/*
 * Implementation of NormalDistribution class.
 * Author: Seb James
 * Date: Nov 2014
 */

#include <string>
#include <vector>
#include <sstream>
#include <ostream>
#include <stdexcept>
#include "normaldistribution.h"
#include "rapidxml.hpp"
#include "rng.h"
#include "util.h"

using namespace std;
using namespace spineml;
using namespace rapidxml;

NormalDistribution::NormalDistribution(xml_node<>* nd_node, const unsigned int num_in_pop)
    : PropertyContent (nd_node, num_in_pop)
    , mean (0.0)
    , variance (1.0)
    , seed (123)
{
    // Get distribution parameters from node.
    xml_attribute<>* attr;
    if ((attr = nd_node->first_attribute ("mean"))) {
        std::stringstream ss;
        ss << attr->value();
        ss >> this->mean;
    } // else mean remains 0.

    if ((attr = nd_node->first_attribute ("variance"))) {
        std::stringstream ss;
        ss << attr->value();
        ss >> this->variance;
    } // else variance remains 1.

    if ((attr = nd_node->first_attribute ("seed"))) {
        std::stringstream ss;
        ss << attr->value();
        ss >> this->seed;
    } // else seed remains 123.
}

NormalDistribution::NormalDistribution()
    : PropertyContent ()
{
}

void
NormalDistribution::writeVLBinaryData (ostream& f)
{
    RngData rngData;
    rngDataInit (&rngData);
    zigset (&rngData, static_cast<unsigned int>(this->seed+1));
    rngData.seed = static_cast<int>(this->seed);

    for (unsigned int i = 0; i<this->numInPopulation; ++i) {
        // Write out values from a normal distribution
        double val = _randomNormal(&rngData) * this->variance + this->mean;
        f.write (reinterpret_cast<const char*>(&i), sizeof(unsigned int));
        f.write (reinterpret_cast<const char*>(&val), sizeof(double));
    }
}

void
NormalDistribution::writeULPropertyValue (xml_document<>* the_doc,
                                          xml_node<>* into_node)
{
    if (!into_node) {
        throw runtime_error ("NormalDistribution::writeULPropertyValue: target node is null");
        return;
    }
    if (!the_doc) {
        throw runtime_error ("NormalDistribution::writeULPropertyValue: doc is null");
    }
    // Allocate new fixed value node
    xml_node<>* nd_node = the_doc->allocate_node (node_element, "UL:NormalDistribution");

    // Allocate and append attributes mean, variance and seed.
    stringstream mean_ss;
    mean_ss << this->mean;
    char* mean_alloced = the_doc->allocate_string (mean_ss.str().c_str());
    xml_attribute<>* mean_attr = the_doc->allocate_attribute ("mean", mean_alloced);
    nd_node->append_attribute (mean_attr);

    stringstream variance_ss;
    variance_ss << this->variance;
    char* variance_alloced = the_doc->allocate_string (variance_ss.str().c_str());
    xml_attribute<>* variance_attr = the_doc->allocate_attribute ("variance", variance_alloced);
    nd_node->append_attribute (variance_attr);

    stringstream seed_ss;
    seed_ss << this->seed;
    char* seed_alloced = the_doc->allocate_string (seed_ss.str().c_str());
    xml_attribute<>* seed_attr = the_doc->allocate_attribute ("seed", seed_alloced);
    nd_node->append_attribute (seed_attr);

    // Add the normal distribution node to the into_node (A UL:Property)
    into_node->prepend_node (nd_node);
}

void
NormalDistribution::setFromString (const string& str)
{
    // String should look like NORM(1,2,124)
    if (str.find ("NORM(") != 0) {
        string emsg = string("'") + str + "' is an invalid normal distribution specification string.";
        throw runtime_error (emsg);
    }
    // Get contents of brackets.
    string::size_type p1 = str.find ("(");
    string::size_type p2 = str.find (")");
    if (p1 == string::npos || p2 == string::npos || p2 < p1) {
        string emsg = string("'") + str + "' is an invalid normal distribution specification string.";
        throw runtime_error (emsg);
    }
    ++p1; --p2;
    string vals = str.substr (p1, p2-p1+1);

    vector<string> vs = Util::stringToVector (vals, ",");

    stringstream meanss;
    meanss << vs[0];
    meanss >> this->mean;
    stringstream variancess;
    variancess << vs[1];
    variancess >> this->variance;
    stringstream seedss;
    seedss << vs[2];
    seedss >> this->seed;
}
