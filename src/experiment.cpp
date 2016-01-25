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
#include "timepointvalue.h"
#include "timepointarrayvalue.h"
#include "util.h"

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
    vector<string> elements = Util::splitStringWithEncs (pcrequest);

    // Now sanity check the elements.
    if (elements.size() != 3) {
        stringstream ee;
        ee << "Wrong number of elements in property change request.\n";
        if (elements.size() == 2) {
            ee << "Two elements in property change request (expect 3):\n";
            ee << "Population/Projection: " << elements[0] << "\n";
            ee << "Property Name: " << elements[1] << "\n";
        } else if (elements.size() == 1) {
            ee << "One element in property change request (expect 3):\n";
            ee << "Population/Projection: " << elements[0] << "\n";
        } else {
            ee << elements.size() << " elements in constant current request (expect 3).\n";
        }
        throw runtime_error (ee.str());
    }

    cout << "Preflight: Property change request: '" << elements[0]
         << "'->'" << elements[1] << "' becomes '" << elements[2] << "'\n";

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
Experiment::addConstantCurrentRequest (const string& ccrequest)
{
    // Looks like: "Population:port:value" where value is a scalar current
    vector<string> elements = Util::splitStringWithEncs (ccrequest);

    // Now sanity check the elements.
    if (elements.size() != 3) {
        stringstream ee;
        ee << "Wrong number of elements in constant current request.\n";
        if (elements.size() == 2) {
            ee << "Two elements in constant current request (expect 3):\n";
            ee << "Population/Projection: " << elements[0] << "\n";
            ee << "Port: " << elements[1] << "\n";
        } else if (elements.size() == 1) {
            ee << "One element in constant current request (expect 3):\n";
            ee << "Population/Projection: " << elements[0] << "\n";
        } else {
            ee << elements.size() << " elements in property change request (expect 3).\n";
        }
        throw runtime_error (ee.str());
    }

    cout << "Preflight: Constant current request: '" << elements[0]
         << "'->'" << elements[1] << "' receives constant input '" << elements[2] << "'\n";

    // We have the elements, can now insert a node into our
    // experiment.
    this->insertExptConstCurrent (elements);
}

void
Experiment::addTimeVaryingCurrentRequest (const string& tvcrequest, const spineml::ModelPreflight& model)
{
    // Looks like: "Population:port:value" where value is a scalar current
    vector<string> elements = Util::splitStringWithEncs (tvcrequest);

    // Now sanity check the elements.
    unsigned int numel = elements.size();
    if (numel != 3 && numel != 4) {
        stringstream ee;
        ee << "Wrong number of elements in time varying current request.\n";
        if (elements.size() == 2) {
            ee << "Two elements in time varying current request (expect 3 or 4):\n";
            ee << "Population/Projection: " << elements[0] << "\n";
            ee << "Port: " << elements[1] << "\n";
        } else if (elements.size() == 1) {
            ee << "One element in time varying current request (expect 3):\n";
            ee << "Population/Projection: " << elements[0] << "\n";
        } else {
            ee << elements.size() << " elements in time varying current request (expect 3 or 4).\n";
        }
        throw runtime_error (ee.str());
    }

    if (numel == 3) {
        cout << "Preflight: Time varying current request: '" << elements[0]
             << "'->'" << elements[1] << "' receives time/current list '" << elements[2] << "'\n";
    } else {
        cout << "Preflight: Time varying current request: '" << elements[0]
             << "'->'" << elements[1] << "' receives time/current list '" << elements[3]
             << "' for population elements '" << elements[2] << "'\n";
    }

    // We have the elements, can now insert a node into our
    // experiment.
    this->insertExptTimeVaryingCurrent (elements, model);
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
    pair<double, string> val_with_dim = Util::getValueWithDimension (elements[2]);
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
Experiment::insertExptConstCurrent (const vector<string>& elements)
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

    // Need to replace or insert a ConstantInput node in expt_node.

    xml_node<>* into_node = static_cast<xml_node<>*>(0);
    // Go through each existing ConstantInput, if we find the one we
    // want to replace, then copy the pointer into "into_node".
    for (xml_node<>* ci_node = expt_node->first_node ("ConstantInput");
         ci_node;
         ci_node = ci_node->next_sibling ("ConstantInput")) {
        xml_attribute<>* targattr = ci_node->first_attribute ("target");
        if (targattr) {
            string target(targattr->value());
            if (target == elements[0]) {
                // target matches, now look at port.
                xml_attribute<>* portattr = ci_node->first_attribute ("port");
                if (portattr) {
                    string port(portattr->value());
                    if (port == elements[1]) {
                        // port matches, so replace this node.
                        into_node = ci_node;
                        break;
                    }
                }
            }
        }
    }

    bool created_node (false);
    if (!into_node) { // into_node will be a "ConstantInput"
        // Create into_node as it doesn't already exist.
        into_node = doc.allocate_node (node_element, "ConstantInput");
        created_node = true;
    } // else existing matching configuration found

    // 1. Remove any existing attributes and child nodes from into_node.
    into_node->remove_all_attributes();
    into_node->remove_all_nodes();
    // 2. Add new target attribute
    char* targstr_alloced = doc.allocate_string (elements[0].c_str());
    xml_attribute<>* target_attr = doc.allocate_attribute ("target", targstr_alloced);
    into_node->append_attribute (target_attr);

    // 3. Add Port
    char* portstr_alloced = doc.allocate_string (elements[1].c_str());
    xml_attribute<>* port_attr = doc.allocate_attribute ("port", portstr_alloced);
    into_node->append_attribute (port_attr);

    // 4. Add value
    char* valstr_alloced = doc.allocate_string (elements[2].c_str());
    xml_attribute<>* val_attr = doc.allocate_attribute ("value", valstr_alloced);
    into_node->append_attribute (val_attr);

    // 5. Add name attribute (same value as port)
    xml_attribute<>* name_attr = doc.allocate_attribute ("name", portstr_alloced);
    into_node->append_attribute (name_attr);

    // Now add the new ConstantInput node to the Experiment node node,
    // if ConstantInput node was newly created.
    if (created_node == true) {
        expt_node->prepend_node (into_node);
    }

    // At end, write out the xml.
    this->write (doc);
}

void
Experiment::insertExptTimeVaryingCurrent (const vector<string>& elements,
                                          const spineml::ModelPreflight& model)
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

    unsigned int numel = elements.size();

    // Need to replace or insert a TimeVaryingInput node in expt_node.

    xml_node<>* into_node = static_cast<xml_node<>*>(0);
    // Go through each existing TimeVaryingInput, if we find the one we
    // want to replace, then copy the pointer into "into_node".
    string elementMatch = "TimeVaryingInput";
    if (numel == 4) {
        elementMatch = "TimeVaryingArrayInput";
    }
    for (xml_node<>* ci_node = expt_node->first_node (elementMatch.c_str());
         ci_node;
         ci_node = ci_node->next_sibling (elementMatch.c_str())) {
        xml_attribute<>* targattr = ci_node->first_attribute ("target");
        if (targattr) {
            string target(targattr->value());
            if (target == elements[0]) {
                // target matches, now look at port.
                xml_attribute<>* portattr = ci_node->first_attribute ("port");
                if (portattr) {
                    string port(portattr->value());
                    if (port == elements[1]) {
                        // port matches, so replace this node.
                        into_node = ci_node;
                        break;
                    }
                }
            }
        }
    }

    bool created_node = false;
    if (numel == 3) {
        created_node = this->createTimeVaryingInputNode (into_node, elements, doc);
    } else {
        created_node = this->createTimeVaryingArrayInputNode (into_node, elements, doc, model);
    }

    // Now add the new ConstantInput node to the Experiment node node,
    // if ConstantInput node was newly created.
    if (created_node == true) {
        expt_node->prepend_node (into_node);
    }

    // At end, write out the xml.
    this->write (doc);
}

bool
Experiment::createTimeVaryingArrayInputNode (xml_node<>*& into_node,
                                             const vector<string>& elements,
                                             xml_document<>& doc,
                                             const spineml::ModelPreflight& model)
{
    bool created_node (false);
    if (!into_node) { // into_node may be a "TimeVaryingArrayInput"
        // Create into_node as it doesn't already exist.
        into_node = doc.allocate_node (node_element, "TimeVaryingArrayInput");
        created_node = true;
    } // else existing matching configuration found

    // 0. Find the size of the target population
    unsigned int targetSize = model.findPopulationSize(elements[0]);

    // 1. Remove any existing attributes and child nodes from into_node.
    into_node->remove_all_attributes();
    into_node->remove_all_nodes();
    // 2. Add new target attribute
    char* targstr_alloced = doc.allocate_string (elements[0].c_str());
    xml_attribute<>* target_attr = doc.allocate_attribute ("target", targstr_alloced);
    into_node->append_attribute (target_attr);

    // 3. Add Port
    char* portstr_alloced = doc.allocate_string (elements[1].c_str());
    xml_attribute<>* port_attr = doc.allocate_attribute ("port", portstr_alloced);
    into_node->append_attribute (port_attr);

    // 4. Add name attribute (same value as port)
    xml_attribute<>* name_attr = doc.allocate_attribute ("name", portstr_alloced);
    into_node->append_attribute (name_attr);

    // 5. Add TimePointValue element.
    vector<string> pairs = Util::splitStringWithEncs (elements[2], string(","));
    vector<string> arrayindices;
    unsigned int numel = elements.size();
    if (numel == 4) {
        pairs = Util::splitStringWithEncs (elements[3], string(","));
        arrayindices = Util::splitStringWithEncs (elements[2], string(","));
    }
    if (pairs.size()%2) {
        throw runtime_error ("experiment XML: Need an even number of values "
                             "in time varying current time/current list");
    }

    vector<string>::const_iterator pi = pairs.begin();
    // Build up the array_time and array_value strings

    stringstream timess;
    stringstream valuess;
    bool first = true;
    while (pi != pairs.end()) {
        // Read from cmd line option:
        if (!first) {
            timess << ",";
        }
        timess << *pi;
        ++pi;
        if (!first) {
            valuess << ",";
        } else {
            first = false;
        }
        valuess << *pi;
        ++pi;
    }

    // Do we know here how many elements in the population? Should be
    // able to figure it out from model.xml but that's going to be a
    // pain. For investigatory purposes, lets hardcode 2500.
    for (unsigned int idx = 0; idx < targetSize; ++idx) {

        bool non_zero_idx = false;

        // See if this is one of our non-zero indices:
        vector<string>::const_iterator ai = arrayindices.begin();
        while (ai != arrayindices.end()) {
            stringstream nss;
            nss << *ai;
            unsigned int aidx;
            nss >> aidx;
            if (aidx == idx) {
                TimePointArrayValue tpav;
                tpav.setIndex (*ai);
                tpav.setArrayTime (timess.str());
                tpav.setArrayValue (valuess.str());
                doc.allocate_node (node_element, "TimePointArrayValue");
                tpav.writeXML (&doc, into_node);
                non_zero_idx = true;
            }
            ++ai;
        }

        if (!non_zero_idx) {
            TimePointArrayValue tpav;
            tpav.setIndex (idx);
            tpav.setArrayTime ("0");
            tpav.setArrayValue ("0");
            doc.allocate_node (node_element, "TimePointArrayValue");
            tpav.writeXML (&doc, into_node);
        }
    }

    return created_node;
}

bool
Experiment::createTimeVaryingInputNode (xml_node<>*& into_node,
                                        const vector<string>& elements,
                                        xml_document<>& doc)
{
    bool created_node (false);
    if (!into_node) { // into_node will be a "TimeVaryingInput"
        // Create into_node as it doesn't already exist.
        into_node = doc.allocate_node (node_element, "TimeVaryingInput");
        created_node = true;
    } // else existing matching configuration found

    // 1. Remove any existing attributes and child nodes from into_node.
    into_node->remove_all_attributes();
    into_node->remove_all_nodes();
    // 2. Add new target attribute
    char* targstr_alloced = doc.allocate_string (elements[0].c_str());
    xml_attribute<>* target_attr = doc.allocate_attribute ("target", targstr_alloced);
    into_node->append_attribute (target_attr);

    // 3. Add Port
    char* portstr_alloced = doc.allocate_string (elements[1].c_str());
    xml_attribute<>* port_attr = doc.allocate_attribute ("port", portstr_alloced);
    into_node->append_attribute (port_attr);

    // 4. Add name attribute (same value as port)
    xml_attribute<>* name_attr = doc.allocate_attribute ("name", portstr_alloced);
    into_node->append_attribute (name_attr);

    // 5. Add TimePointValue element.
    vector<string> pairs = Util::splitStringWithEncs (elements[2], string(","));
    if (pairs.size()%2) {
        throw runtime_error ("experiment XML: Need an even number of values "
                             "in time varying current time/current list");
    }

    vector<string>::const_iterator pi = pairs.begin();
    while (pi != pairs.end()) {
        // Read from cmd line option:
        stringstream timess;
        double time;
        timess << *pi;
        timess >> time;
        ++pi;
        stringstream valuess;
        double value;
        valuess << *pi;
        valuess >> value;
        ++pi;
        // Got time and value of current, so insert a node:
        TimePointValue tpv;
        tpv.setTime (time);
        tpv.setValue (value);
        /*xml_node<>* tpv_node =*/ doc.allocate_node (node_element, "TimePointValue");
        // As we've not added prop_node to the document we have to
        // pass the document pointer here:
        tpv.writeXML (&doc, into_node);
    }

    return created_node;
}

void
Experiment::setModelDir (const string& dir)
{
    this->modelDir = dir;
}
