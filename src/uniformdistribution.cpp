#include <string>
#include <sstream>
#include <ostream>
#include <stdexcept>
#include "uniformdistribution.h"
#include "rapidxml.hpp"
#include "rng.h"

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
