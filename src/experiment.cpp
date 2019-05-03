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
#include "uniformdistribution.h"
#include "normaldistribution.h"
#include "timepointvalue.h"
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
Experiment::addDelayChangeRequest (const string& dcrequest)
{
    // Looks like: "PopA:PopB:SynNum:value" for projection or
    // "PopA:PortA:PopB:PortB:value" for generic connection.
    vector<string> elements = Util::splitStringWithEncs (dcrequest);

    // Now sanity check the elements.
    if (elements.size() != 4 && elements.size() != 5) {
        stringstream ee;
        ee << "Wrong number of elements in delay change request.\n";
        ee << elements.size() << " elements in delay change request (expect 4 or 5).\n";
        throw runtime_error (ee.str());
    }

    // Some output to cmd line
    if (elements.size() == 4) { // projection
        cout << "Preflight: Projection delay change request: '" << elements[0]
             << "'->'" << elements[1] << "', synapse " << elements[2] << " delay becomes " << elements[3] << " ms\n";
    } else { // generic input connection
        cout << "Preflight: Generic connection delay change request: '" << elements[0] << "' port " << elements[1]
             << "'->'" << elements[2] << "', port " << elements[3] << " delay becomes " << elements[4] << " ms\n";
    }

    // We have the elements, we now need to search model.xml to make
    // sure the connections exist.
    spineml::ModelPreflight model (this->modelDir, this->network_layer_path);
    model.init();

    string delayname("Delay");
    if (elements.size() == 4) { // projection

        string wuname = this->buildProjectionWUName (elements[0], elements[1], elements[2]);
        xml_node<>* weightupdate_node = model.findLLWeightUpdate (static_cast<xml_node<>*>(0), wuname);
        if (!weightupdate_node) {
            // No such weight update node in the model, so throw a runtime error
            stringstream ee;
            ee << "The model does not contain a weight update node named '" << wuname << "'";
            throw runtime_error (ee.str());
        }

        // Now find the parent synapse node, then find the Delay node.
        string synname("LL:Synapse");
        xml_node<>* synapse_node = model.findNamedParent (weightupdate_node, synname);
        if (!synapse_node) {
            throw runtime_error ("This LL:WeightUpdate does not have an LL:Synapse parent");
        }
        xml_node<>* delay_node = model.findNamedElement(synapse_node, delayname);
        if (!delay_node) {
            string cl("ConnectionList");
            xml_node<>* connlist_node = model.findNamedElement (synapse_node, cl);
            if (!connlist_node) {
                throw runtime_error ("This LL:WeightUpdate does not contain a Delay element or ConnectionList");
            }
        }

        // Can now insert a node into our experiment.
        this->insertModelProjectionDelay (delay_node, elements);

        // Record the delay changes for later use when the model
        // itself is pre-flighted. Some delays may need to be written
        // into the binary file.
        DelayChange d(DELAYCHANGE_PROJECTION);
        d.src = elements[0];
        d.dst = elements[1];
        d.setSynapseNumber(elements[2]);
        d.setDelay (elements[3]);
        this->delayChanges.push_back(d);

    } else { // generic input connection

        xml_node<>* input_node = model.findLLInput (static_cast<xml_node<>*>(0), "root", elements[0], elements[1], elements[2], elements[3]);
        if (!input_node) {
            // No such node in the model, so throw a runtime error
            stringstream ee;
            ee << "The model does not contain an LL:Input with src='" << elements[0]
               << "', src_port=" << elements[1] << " and dst_port=" << elements[3] << " in a containing LL:Neuron called '" << elements[2] << "'";
            throw runtime_error (ee.str());
        }

        // Now find the delay node which is inside the input_node.
        xml_node<>* delay_node = model.findNamedElement (input_node, delayname);
        if (!delay_node) {
            // Actually, this isn't necessarily an error - this Input
            // may contain a connection list which itself contains
            // individual delays (which we will then override).
            string cl("ConnectionList");
            xml_node<>* connlist_node = model.findNamedElement (input_node, cl);
            if (!connlist_node) {
                throw runtime_error ("This LL:Input does not contain a Delay element or ConnectionList");
            } // else no delay, but delays presumably inside connlist
              // node (could check for at least one Connection node
              // therein, containing a delay attribute).
        }

        // Can now insert a node into our experiment.
        this->insertModelGenericDelay (delay_node, elements);

        DelayChange d(DELAYCHANGE_GENERIC);
        d.src = elements[0];
        d.srcPort = elements[1];
        d.dst = elements[2];
        d.dstPort = elements[3];
        d.setDelay (elements[4]);
        this->delayChanges.push_back(d);
    }
}

void
Experiment::addFixedProbChangeRequest (const string& fprequest)
{
    // Looks like: "PopA:PopB:SynNum:value" for projection or
    // "PopA:PortA:PopB:PortB:value" for generic connection.
    vector<string> elements = Util::splitStringWithEncs (fprequest);

    // Now sanity check the elements.
    if (elements.size() != 4 && elements.size() != 5) {
        stringstream ee;
        ee << "Wrong number of elements in probability change request.\n";
        ee << elements.size() << " elements in probability change request (expect 4 or 5).\n";
        throw runtime_error (ee.str());
    }

    // Some output to cmd line
    if (elements.size() == 4) { // projection
        cout << "Preflight: FixedProbability Projection probability change request: '" << elements[0]
             << "'->'" << elements[1] << "', synapse " << elements[2] << " probability becomes " << elements[3] << "\n";
    } else { // generic input connection
        cout << "Preflight: Generic connection FixedProbability change request: '" << elements[0] << "' port " << elements[1]
             << "'->'" << elements[2] << "', port " << elements[3] << " probability becomes " << elements[4] << "\n";
    }

    // We have the elements, we now need to search model.xml to make
    // sure the connections exist.
    spineml::ModelPreflight model (this->modelDir, this->network_layer_path);
    model.init();

    string fpname("FixedProbabilityConnection");
    if (elements.size() == 4) { // projection

        string wuname = this->buildProjectionWUName (elements[0], elements[1], elements[2]);
        xml_node<>* weightupdate_node = model.findLLWeightUpdate (static_cast<xml_node<>*>(0), wuname);
        if (!weightupdate_node) {
            // No such weight update node in the model, so throw a runtime error
            stringstream ee;
            ee << "The model does not contain a weight update node named '" << wuname << "'";
            throw runtime_error (ee.str());
        }

        // Now find the parent synapse node, then find the FixedProbabilityConnection node.
        string synname("LL:Synapse");
        xml_node<>* synapse_node = model.findNamedParent (weightupdate_node, synname);
        if (!synapse_node) {
            throw runtime_error ("This LL:WeightUpdate does not have an LL:Synapse parent");
        }
        xml_node<>* fp_node = model.findNamedElement(synapse_node, fpname);
        if (!fp_node) {
            throw runtime_error ("This LL:WeightUpdate does not contain a sibling FixedProbabilityConnection element");
        }

        // Can now insert a node into our experiment. Either by
        // updating the model attribute so that when we expand the
        // FixedProb list, we expand with the new probability or, by
        // recording here.
        this->insertModelUpdateFixedProb (fp_node, elements);

    } else { // generic input connection

        xml_node<>* input_node = model.findLLInput (static_cast<xml_node<>*>(0), "root", elements[0], elements[1], elements[2], elements[3]);
        if (!input_node) {
            // No such node in the model, so throw a runtime error
            stringstream ee;
            ee << "The model does not contain an LL:Input with src='" << elements[0]
               << "', src_port=" << elements[1] << " and dst_port=" << elements[3] << " in a containing LL:Neuron called '" << elements[2] << "'";
            throw runtime_error (ee.str());
        }

        // Now find the FixedProabilityConnection node which is inside the input_node.
        xml_node<>* fp_node = model.findNamedElement (input_node, fpname);
        if (!fp_node) {
            throw runtime_error ("This LL:Input does not contain a FixedProbability element");
        }

        // Can now insert a node into our experiment.
        this->insertModelUpdateFixedProb (fp_node, elements);
    }
}

void
Experiment::addPropertyChangeRequest (const string& pcrequest)
{
    // Looks like: "Population:varname:value" where value could be
    // fixed (default) or a distribution: NORM(mean,variance,seed) or UNI(min,max,seed)
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
Experiment::addTimeVaryingCurrentRequest (const string& tvcrequest)
{
    // Looks like: "Population:port:value" where value is a scalar current
    vector<string> elements = Util::splitStringWithEncs (tvcrequest);

    // Now sanity check the elements.
    if (elements.size() != 3 && elements.size()!= 4) {
        stringstream ee;
        ee << "Wrong number of elements in time varying current request.\n";
        if (elements.size() == 2) {
            ee << "Two elements in time varying current request (expect 3):\n";
            ee << "Population/Projection: " << elements[0] << "\n";
            ee << "Port: " << elements[1] << "\n";
        } else if (elements.size() == 1) {
            ee << "One element in time varying current request (expect 3):\n";
            ee << "Population/Projection: " << elements[0] << "\n";
        } else {
            ee << elements.size() << " elements in time varying current request (expect 3).\n";
        }
        throw runtime_error (ee.str());
    }

    if (elements.size() == 4) {
        cout << "Preflight: Time varying spike input request: '" << elements[0]
             << "'->'" << elements[1] << "' receives " << elements[2] <<  " time/current list '" << elements[3] << "'\n";

    } else {
        cout << "Preflight: Time varying current request: '" << elements[0]
             << "'->'" << elements[1] << "' receives time/current list '" << elements[2] << "'\n";
    }
    // We have the elements, can now insert a node into our
    // experiment.
    this->insertExptTimeVaryingCurrent (elements);
}

std::string
Experiment::buildProjectionWUName (const std::string& src, const std::string& dst, const std::string& synapsenum)
{
    stringstream wuss;
    wuss << src << " to " << dst << " Synapse " << synapsenum << " weight_update";
    return wuss.str();
}

rapidxml::xml_node<>*
Experiment::findExperimentModel (xml_document<>& doc)
{
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

    return model_node;
}

void
Experiment::insertModelProjectionDelay (xml_node<>* unused_node, const vector<string>& elements)
{
    xml_document<> doc;
    AllocAndRead ar(this->filepath);
    char* textptr = ar.data();
    doc.parse<parse_declaration_node | parse_no_data_nodes>(textptr);

    xml_node<>* model_node = this->findExperimentModel (doc);

    xml_node<>* into_node = static_cast<xml_node<>*>(0);
    // Go through each ProjectionDelayChange
    for (xml_node<>* model_delaychange_node = model_node->first_node ("ProjectionDelayChange");
         model_delaychange_node;
         model_delaychange_node = model_delaychange_node->next_sibling ("ProjectionDelayChange")) {

        xml_attribute<>* sattr = model_delaychange_node->first_attribute ("src");
        xml_attribute<>* dattr = model_delaychange_node->first_attribute ("dst");
        xml_attribute<>* synattr = model_delaychange_node->first_attribute ("synapse");

        if (sattr && dattr && synattr) {
            string s(sattr->value());
            string d(dattr->value());
            string syn(synattr->value());
            if (s == elements[0] && d == elements[1] && syn == elements[2]) {
                // This Delay's attributes match the parameters given on the cmd line
                into_node = model_delaychange_node;
                break;
            }
        }
    }

    bool created_node (false);
    if (!into_node) { // into_node will be a "ProjectionDelayChange"
        // Create into_node as it doesn't already exist.
        into_node = doc.allocate_node (node_element, "ProjectionDelayChange");
        created_node = true;
    } // else existing matching configuration found

    // 1. Remove any existing attributes and child nodes from into_node.
    into_node->remove_all_attributes();
    into_node->remove_all_nodes();
    // 2. Add new weight_update attribute
    char* sstr_alloced = doc.allocate_string (elements[0].c_str());
    xml_attribute<>* s_attr = doc.allocate_attribute ("src", sstr_alloced);
    into_node->append_attribute (s_attr);
    char* dstr_alloced = doc.allocate_string (elements[1].c_str());
    xml_attribute<>* d_attr = doc.allocate_attribute ("dst", dstr_alloced);
    into_node->append_attribute (d_attr);
    char* synstr_alloced = doc.allocate_string (elements[2].c_str());
    xml_attribute<>* syn_attr = doc.allocate_attribute ("synapse", synstr_alloced);
    into_node->append_attribute (syn_attr);

    // 2.2 Add Delay node
    xml_node<>* delay_node = doc.allocate_node (node_element, "UL:Delay");

    // 3. Add dimension attribute
    char* dimstr_alloced = doc.allocate_string ("ms");
    xml_attribute<>* dim_attr = doc.allocate_attribute ("dimension", dimstr_alloced);
    delay_node->append_attribute (dim_attr);

    // 4. Allocate new fixed value node
    xml_node<>* fv_node = doc.allocate_node (node_element, "UL:FixedValue");
    // Allocate and append an attribute
    char* val_alloced = doc.allocate_string (elements[3].c_str());
    xml_attribute<>* value_attr = doc.allocate_attribute ("value", val_alloced);
    fv_node->append_attribute (value_attr);

    // Add FixedValue into Delay
    delay_node->prepend_node (fv_node);

    // Add Delay into the ProjectionDelayChange node
    into_node->prepend_node (delay_node);

    // Now add the new Delay node to the document's Model node, if it
    // has been newly created.
    if (created_node == true) {
        model_node->prepend_node (into_node);
    }
    // At end, write out the xml.
    this->write (doc);
}

void
Experiment::insertModelGenericDelay (xml_node<>* unused_node, const vector<string>& elements)
{
    xml_document<> doc;
    AllocAndRead ar(this->filepath);
    char* textptr = ar.data();
    doc.parse<parse_declaration_node | parse_no_data_nodes>(textptr);
    xml_node<>* model_node = this->findExperimentModel (doc);

    // Need to find delay_node in model_node which matches elements 0,1,2 and 3.

    xml_node<>* into_node = static_cast<xml_node<>*>(0);
    // Go through each GenericInputDelayChange.
    for (xml_node<>* model_delaychange_node = model_node->first_node ("GenericInputDelayChange");
         model_delaychange_node;
         model_delaychange_node = model_delaychange_node->next_sibling ("GenericInputDelayChange")) {

        xml_attribute<>* sattr = model_delaychange_node->first_attribute ("src");
        xml_attribute<>* spattr = model_delaychange_node->first_attribute ("src_port");
        xml_attribute<>* dattr = model_delaychange_node->first_attribute ("dst");
        xml_attribute<>* dpattr = model_delaychange_node->first_attribute ("dst_port");

        if (sattr && spattr && dattr && dpattr) {
            string s(sattr->value());
            string sp(spattr->value());
            string d(dattr->value());
            string dp(dpattr->value());
            if (s == elements[0] && sp == elements[1] && d == elements[2] && dp == elements[3]) {
                // This Delay's attributes match the parameters given on the cmd line
                into_node = model_delaychange_node;
                break;
            }
        }

    }

    bool created_node (false);
    if (!into_node) { // into_node will be a "GenericInputDelayChange"
        // Create into_node as it doesn't already exist.
        into_node = doc.allocate_node (node_element, "GenericInputDelayChange");
        created_node = true;
    } // else existing matching configuration found

    // 1. Remove any existing attributes and child nodes from into_node.
    into_node->remove_all_attributes();
    into_node->remove_all_nodes();
    // 2. Add new src,srcPort,dst and dstPort attributes
    char* sstr_alloced = doc.allocate_string (elements[0].c_str());
    xml_attribute<>* s_attr = doc.allocate_attribute ("src", sstr_alloced);
    into_node->append_attribute (s_attr);
    char* spstr_alloced = doc.allocate_string (elements[1].c_str());
    xml_attribute<>* sp_attr = doc.allocate_attribute ("src_port", spstr_alloced);
    into_node->append_attribute (sp_attr);
    char* dstr_alloced = doc.allocate_string (elements[2].c_str());
    xml_attribute<>* d_attr = doc.allocate_attribute ("dst", dstr_alloced);
    into_node->append_attribute (d_attr);
    char* dpstr_alloced = doc.allocate_string (elements[3].c_str());
    xml_attribute<>* dp_attr = doc.allocate_attribute ("dst_port", dpstr_alloced);
    into_node->append_attribute (dp_attr);

    // 2.2 Add Delay node
    xml_node<>* delay_node = doc.allocate_node (node_element, "UL:Delay");

    // 3. Add dimension attribute
    char* dimstr_alloced = doc.allocate_string ("ms");
    xml_attribute<>* dim_attr = doc.allocate_attribute ("dimension", dimstr_alloced);
    delay_node->append_attribute (dim_attr);

    // 4. Allocate new fixed value node
    xml_node<>* fv_node = doc.allocate_node (node_element, "UL:FixedValue");
    // Allocate and append an attribute
    char* val_alloced = doc.allocate_string (elements[4].c_str());
    xml_attribute<>* value_attr = doc.allocate_attribute ("value", val_alloced);
    fv_node->append_attribute (value_attr);
    delay_node->prepend_node (fv_node);

    into_node->prepend_node (delay_node);

    // Now add the new Delay node to the document's Model node, if it
    // has been newly created.
    if (created_node == true) {
        model_node->prepend_node (into_node);
    }
    // At end, write out the xml.
    this->write (doc);
}

void
Experiment::insertModelConfig (xml_node<>* property_node, const vector<string>& elements)
{
    xml_document<> doc;
    AllocAndRead ar(this->filepath);
    char* textptr = ar.data();
    doc.parse<parse_declaration_node | parse_no_data_nodes>(textptr);
    xml_node<>* model_node = this->findExperimentModel (doc);

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
    xml_node<>* prop_node = doc.allocate_node (node_element, "UL:Property");
    if (elements[2].find("UNI") != string::npos) {
        UniformDistribution ud;
        // This takes off any "ms" or somesuch dimension from the end of "UNI(1,2,123)ms"
        pair<string, string> dist_with_dim = Util::getDistWithDimension (elements[2]);
        ud.setPropertyDim (dist_with_dim.second);
        ud.setPropertyName (elements[1]);
        ud.setFromString (dist_with_dim.first);
        // As we've not added prop_node to the document we have to pass the document pointer here:
        ud.writeULProperty (&doc, prop_node);

    } else if (elements[2].find("NORM") != string::npos) {
        NormalDistribution nd;
        // This takes off any "ms" or somesuch dimension from the end of "NORM(1,2,123)ms"
        pair<string, string> dist_with_dim = Util::getDistWithDimension (elements[2]);
        nd.setPropertyDim (dist_with_dim.second);
        nd.setPropertyName (elements[1]);
        nd.setFromString (dist_with_dim.first);
        nd.writeULProperty (&doc, prop_node);

    } else {
        FixedValue fv;
        fv.setPropertyName (elements[1]);

        // Get dimension from elements[2]
        pair<double, string> val_with_dim = Util::getValueWithDimension (elements[2]);
        fv.setPropertyDim (val_with_dim.second);
        fv.setValue (val_with_dim.first);
        fv.writeULProperty (&doc, prop_node);
    }
    into_node->prepend_node (prop_node);

    // Now add the new Configuration node to the document's Model
    // node, if the Configuration node has been newly created.
    if (created_node == true) {
        model_node->prepend_node (into_node);
    }

    // At end, write out the xml.
    this->write (doc);
}

// Change an attribute and say we did so in the experiment file.
void
Experiment::insertModelUpdateFixedProb (xml_node<>* fp_node, const vector<string>& elements)
{
    xml_document<> doc;
    AllocAndRead ar(this->filepath);
    char* textptr = ar.data();
    doc.parse<parse_declaration_node | parse_no_data_nodes>(textptr);
    xml_node<>* model_node = this->findExperimentModel (doc);

    if (elements.size() < 4) {
        throw runtime_error ("Experiment::insertModelUpdateFixedProb: expected elements to have 4 fields");
    }

    // Need to find fp_node in model_node and change (delete/replace)
    // the probability attribute. What do we know about fp_node?  We
    // have container name which is elements[0], src pop elements[1]
    // dest pop and synapse in elements[2]; elements[3] contains the
    // new value. The attribute to change is "probability"
    xml_attribute<>* probattr = fp_node->first_attribute ("probability");
    if (probattr) {
        // We have a probability attribute, delete it
        fp_node->remove_attribute (probattr);
    }
    char* newprob_alloced = doc.allocate_string (elements[3].c_str());
    xml_attribute<>* newprob_attr = doc.allocate_attribute ("probability", newprob_alloced);
    fp_node->append_attribute (newprob_attr);

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

    // Need to replace or insert a ConstantInput node in
    // expt_node.

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
Experiment::insertExptTimeVaryingCurrent (const vector<string>& elements)
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

    // Need to replace or insert a TimeVaryingInput node in expt_node.

    xml_node<>* into_node = static_cast<xml_node<>*>(0);
    // Go through each existing TimeVaryingInput, if we find the one we
    // want to replace, then copy the pointer into "into_node".
    for (xml_node<>* ci_node = expt_node->first_node ("TimeVaryingInput");
         ci_node;
         ci_node = ci_node->next_sibling ("TimeVaryingInput")) {
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

    // 4.5 If necessary add the spike train rng distribution
    if (elements.size() == 4) {
        char* diststr_alloced = doc.allocate_string (elements[2].c_str());
        xml_attribute<>* dist_attr = doc.allocate_attribute ("rate_based_input", diststr_alloced);
        into_node->append_attribute (dist_attr);
    }

    // 5. Add TimePointValue elements.
    vector<string> pairs = Util::splitStringWithEncs (elements[elements.size()-1], string(","));
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

    // Now add the new TimeVaryingInput node to the Experiment node,
    // if TimeVaryingInput node was newly created.
    if (created_node == true) {
        expt_node->prepend_node (into_node);
    }

    // At end, write out the xml.
    this->write (doc);
}

void
Experiment::setModelDir (const string& dir)
{
    this->modelDir = dir;
}
