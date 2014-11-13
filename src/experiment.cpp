/*
 * Implementation of Experiment class.
 * Author: Seb James
 * Date: Nov 2014
 */

#include <string>
#include <sstream>
#include <stdexcept>
#include <iostream>
#include "rapidxml.hpp"
#include "allocandread.h"
#include "experiment.h"

using namespace std;
using namespace spineml;
using namespace rapidxml;

Experiment::Experiment()
    : filepath("model/experiment.xml")
    , simDuration (0)
    , simFixedDt (0)
    , simType ("Unknown")
{
    this->parse();
}

Experiment::Experiment(const std::string& path)
    : filepath(path)
    , simDuration (0)
    , simFixedDt (0)
    , simType ("Unknown")
{
    this->parse();
}

double
Experiment::getSimDuration (void)
{
    return this->simDuration;
}

double
Experiment::getSimFixedDt (void)
{
    return this->simFixedDt;
}

string
Experiment::modelUrl (void)
{
    return this->network_layer_path;
}

double
Experiment::getSimFixedRate (void)
{
    double rate = 0.0;
    if (this->simFixedDt != 0) {
        rate = 1.0/this->simFixedDt;
    }
    return rate;
}

void
Experiment::parse (void)
{
    xml_document<> doc;

    AllocAndRead ar(this->filepath);
    char* textptr = ar.data();

    doc.parse<parse_declaration_node | parse_no_data_nodes>(textptr);

    // NB: This really DOES have to be the root node.
    xml_node<>* root_node = doc.first_node("SpineML");
    if (!root_node) {
        throw runtime_error ("experiment XML: no root SpineML node");
    }

    xml_node<>* expt_node = root_node->first_node ("Experiment");
    if (!expt_node) {
        throw runtime_error ("experiment XML: no Experiment node");
    }

    xml_node<>* model_node = expt_node->first_node ("Model");
    if (!model_node) {
        throw runtime_error ("experiment XML: no Model node");
    }
    xml_attribute<>* url_attr;
    if ((url_attr = model_node->first_attribute ("network_layer_url"))) {
        this->network_layer_path = url_attr->value();
    }

    xml_node<>* sim_node = expt_node->first_node ("Simulation");
    if (!sim_node) {
        throw runtime_error ("experiment XML: no Simulation node");
    }

    // Find duration
    xml_attribute<>* dur_attr;
    if ((dur_attr = sim_node->first_attribute ("duration"))) {
        stringstream sdss;
        sdss << dur_attr->value();
        sdss >> this->simDuration;
    }
    xml_node<>* euler_node = sim_node->first_node ("EulerIntegration");
    if (euler_node) {
        this->simType = "EulerIntegration";
        xml_attribute<>* dt_attr;
        if ((dt_attr = euler_node->first_attribute ("dt"))) {
            stringstream dtss;
            dtss << dt_attr->value();
            dtss >> this->simFixedDt;
            // Convert from ms to seconds:
            this->simFixedDt /= 1000;
        }
    }
}

void
Experiment::addPropertyChangeRequest (const string& s)
{
    cout << "property change request: " << s << "\n";
}
