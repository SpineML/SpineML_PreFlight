/*
 * Implementation of NormalDistribution class.
 * Author: Seb James
 * Date: Nov 2014
 */

#include <string>
#include <sstream>
#include <ostream>
#include <stdexcept>
#include "normaldistribution.h"
#include "rapidxml.hpp"
#include "rng.h"

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
