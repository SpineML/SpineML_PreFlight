/*
 * Implementation of Experiment class.
 * Author: Seb James
 * Date: Nov 2014
 */

#include <string>
#include <sstream>
#include <stdexcept>
#include <iostream>
#include <fstream>
#include <utility>
#include "rapidxml_print.hpp"
#include "rapidxml.hpp"
#include "allocandread.h"
#include "experiment.h"
#include "modelpreflight.h"
#include "fixedvalue.h"

using namespace std;
using namespace spineml;
using namespace rapidxml;

Experiment::Experiment()
    : filepath("model/experiment.xml")
    , simDuration (0)
    , simFixedDt (0)
    , simType ("Unknown")
    , modelDir ("model")
{
    this->parse();
}

Experiment::Experiment(const std::string& path)
    : filepath(path)
    , simDuration (0)
    , simFixedDt (0)
    , simType ("Unknown")
    , modelDir ("model")
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

pair<double, string>
Experiment::getValueWithDimension (const std::string& str)
{
    // Two copies of str to work on.
    string valstring = str;
    string dimstring = str;

    // Remove numbers and whitespace from dimstring.
    {
        string charList("0123456789-+. \t\n\r");
        string::size_type pos(0);
        while ((pos = dimstring.find_last_of (charList)) != string::npos) {
            dimstring.erase (pos, 1);
        }
    }

    // Remove trailing non-numbers from valstring.
    {
        char c;
        string::size_type pos = valstring.size();
        while (pos > 0 &&
               ((c = valstring[pos-1]) != '.'
                && c != '+'
                && c != '-'
                && c != '0'
                && c != '1'
                && c != '2'
                && c != '3'
                && c != '4'
                && c != '5'
                && c != '6'
                && c != '7'
                && c != '8'
                && c != '9'
                   )) {
            pos--;
        }
        valstring.erase (pos);
    }

    // A return object.
    pair<double, string> rtn;

    // Add value to rtn.
    double val(0.0);
    stringstream val_ss;
    val_ss << valstring;        ;
    val_ss >> rtn.first;

    // Add dimension to rtn.
    rtn.second = dimstring;

    return rtn;
}

void
Experiment::write (const xml_document<>& the_doc)
{
#if 0
    // If requested, backup experiment.xml:
    if (this->backup == true) {
        stringstream cmd;
        cmd << "cp " << this->filepath << " " << this->filepath << ".bu";
        system (cmd.str().c_str());
    }
#endif

    // Write experiment.xml:
    ofstream f;
    cout << "Opening " << this->filepath << "\n";
    f.open (this->filepath.c_str(), ios::out|ios::trunc);
    if (!f.is_open()) {
        stringstream ee;
        ee << "Failed to open '" << filepath << "' for writing";
        throw runtime_error (ee.str());
    }
    f << the_doc;
    f.close();
}

void
Experiment::addPropertyChangeRequest (const string& pcrequest)
{
    // Looks like: "Population:varname:value" where value could be
    // fixed (default) or a distribution.
    vector<string> elements = Experiment::splitStringWithEncs (pcrequest);

    // Now sanity check the elements.
    if (elements.size() != 3) {
        throw runtime_error ("Wrong number of elements in property change request.");
    }

    cout << "Property change request: Override property '" << elements[1]
         << "' in target '" << elements[0] << "' with value '" << elements[2] << "'\n";

    // We have the elements, we now need to search model.xml to make
    // sure these exist.
    spineml::ModelPreflight model (this->modelDir, this->network_layer_path);
    model.init();
    xml_node<>* property_node = model.findProperty (static_cast<xml_node<>*>(0), "root", elements[0], elements[1]);
    if (!property_node) {
        // No such node in the model, so throw a runtime error
        stringstream ee;
        ee << "The model does not contain a Property '" << elements[1] << "' in a container called '" << elements[0] << "'";
        throw runtime_error (ee.str());
    }
    // Can now insert a node into our experiment.
    this->insertModelConfig (property_node, elements);
}

void
Experiment::insertModelConfig (xml_node<>* property_node, const vector<string>& elements)
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

    // Need to find property_node in model_node and replace or insert
    // a new one. What do we know about property_node?  We have
    // container name which is elements[0], property name elements[1]
    // and value elements[2].

    xml_node<>* into_node = static_cast<xml_node<>*>(0);
    // Go through each Configuration.
    for (xml_node<>* config_node = model_node->first_node ("Configuration");
         config_node;
         config_node = config_node->next_sibling ("Configuration")) {
        xml_attribute<>* targattr = config_node->first_attribute ("target");
        if (targattr) {
            string target(targattr->value());
            if (target == elements[0]) {
                // target matches, now look at name.
                xml_node<>* ulprop_node = config_node->first_node ("UL:Property");
                if (ulprop_node) {
                    xml_attribute<>* ulprop_name_attr = ulprop_node->first_attribute ("name");
                    if (ulprop_name_attr) {
                        string ulprop_name(ulprop_name_attr->value());
                        if (ulprop_name == elements[1]) {
                            // We have an existing matching Configuration, so we need to replace it, not add.
                            into_node = config_node;
                            break;
                        }
                    }
                }
            }
        }
    }

    bool created_node (false);
    if (!into_node) { // into_node will be a "Configuration"
        // Create into_node as it doesn't already exist.
        into_node = doc.allocate_node (node_element, "Configuration");
        created_node = true;
    } // else existing matching configuration found

    // 1. Remove any existing attributes and child nodes from into_node.
    into_node->remove_all_attributes();
    into_node->remove_all_nodes();
    // 2. Add new target attribute
    char* targstr_alloced = doc.allocate_string (elements[0].c_str());
    xml_attribute<>* target_attr = doc.allocate_attribute ("target", targstr_alloced);
    into_node->append_attribute (target_attr);

    // 3. Add UL:Property node

    // if (FixedValue)....
    FixedValue fv;
    fv.setPropertyName (elements[1]);

    // Get dimension from elements[2]
    pair<double, string> val_with_dim = this->getValueWithDimension (elements[2]);
    fv.setPropertyDim (val_with_dim.second);
    fv.setValue (val_with_dim.first);
    xml_node<>* prop_node = doc.allocate_node (node_element, "UL:Property");
    // As we've not added prop_node to the document we have to pass the document pointer here:
    fv.writeULProperty (&doc, prop_node);
    into_node->prepend_node (prop_node);

    // else if NORM, UNI etc

    // Now add the new Configuration node to the document's Model
    // node, if the Configuration node has been newly created.
    if (created_node == true) {
        model_node->prepend_node (into_node);
    }

    // At end, write out the xml.
    this->write (doc);
}

void
Experiment::setModelDir (const string& dir)
{
    this->modelDir = dir;
}

int
Experiment::stripChars (std::string& input, const std::string& charList)
{
        int rtn(0);
        string::size_type pos(0);
        while ((pos = input.find_last_of (charList)) != string::npos) {
                input.erase (pos, 1);
                ++rtn;
        }
        return rtn;
}

int
Experiment::stripChars (std::string& input, const char charList)
{
        int rtn(0);
        string::size_type pos(0);
        while ((pos = input.find_last_of (charList)) != string::npos) {
                input.erase (pos, 1);
                ++rtn;
        }
        return rtn;
}
