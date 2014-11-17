/*
 * Implementation of the class ModelPreflight.
 *
 * Author: Seb James <seb.james@sheffield.ac.uk>
 * Date: Oct-Nov 2014
 * Licence: GNU GPL
 */

#include <string>
#include <sstream>
#include <fstream>
#include <stdexcept>
#include "rapidxml_print.hpp"
#include "rapidxml.hpp"
#include "util.h"
#include "modelpreflight.h"
#include "connection_list.h"
#include "fixedvalue.h"
#include "uniformdistribution.h"
#include "normaldistribution.h"
#include "valuelist.h"

using namespace std;
using namespace rapidxml;
using namespace spineml;

ModelPreflight::ModelPreflight(const std::string& fdir, const std::string& fname)
    : root_node (static_cast<xml_node<>*>(0))
    , binfilenum (0)
    , explicitData_binfilenum (0)
    , backup (false)
{
    this->modeldir = fdir;
    this->modelfile = fname;
    string filepath = this->modeldir + this->modelfile;
    this->modeldata.read (filepath);
}

void
ModelPreflight::write (void)
{
    string filepath = this->modeldir + this->modelfile;

    // If requested, backup model.xml:
    if (this->backup == true) {
        stringstream cmd;
        cmd << "cp " << filepath << " " << filepath << ".bu";
        system (cmd.str().c_str());
    }

    // Write model.xml:
    ofstream f;
    f.open (filepath.c_str(), ios::out|ios::trunc);
    if (!f.is_open()) {
        stringstream ee;
        ee << "Failed to open '" << filepath << "' for writing";
        throw runtime_error (ee.str());
    }
    f << this->doc;
    f.close();
}

void
ModelPreflight::init (void)
{
    if (!this->root_node) {
        // we are choosing to parse the XML declaration
        // parse_no_data_nodes prevents RapidXML from using the somewhat
        // surprising behaviour of having both values and data nodes, and
        // having data nodes take precedence over values when printing
        // >>> note that this will skip parsing of CDATA nodes <<<
        this->doc.parse<parse_declaration_node | parse_no_data_nodes>(this->modeldata.data());

        // Get the root node.
        this->root_node = this->doc.first_node (LVL"SpineML");
        if (!this->root_node) {
            // Possibly look for HL:SpineML, if we have a high level model (not
            // used by anyone at present).
            stringstream ee;
            ee << "No root node " << LVL << "SpineML!";
            throw runtime_error (ee.str());
        }
    }
}

void
ModelPreflight::preflight (void)
{
    this->init();
    // Search each population for stuff.
    for (this->first_pop_node = this->root_node->first_node(LVL"Population");
         this->first_pop_node;
         this->first_pop_node = this->first_pop_node->next_sibling(LVL"Population")) {
        this->preflight_population (this->first_pop_node);
    }
}

int
ModelPreflight::find_num_neurons (const string& dst_population)
{
    int numNeurons = -1;
    xml_node<>* pop_node = this->first_pop_node;
    xml_node<>* neuron_node;
    while (pop_node) {
        // Dive into this population:
        // <LL:Population>
        //    <LL:Neuron name="Population 0" size="10" url="New_Component_1.xml">
        neuron_node = pop_node->first_node(LVL"Neuron");
        if (neuron_node) {
            // Find name.
            string name("");
            xml_attribute<>* name_attr;
            if ((name_attr = neuron_node->first_attribute ("name"))) {
                name = name_attr->value();
                if (name == dst_population) {
                    // Match! Get the size:
                    xml_attribute<>* size_attr;
                    if ((size_attr = neuron_node->first_attribute ("size"))) {
                        stringstream ss;
                        ss << size_attr->value();
                        ss >> numNeurons;
                        break;
                    } // else failed to get size attr of Neuron node
                } // else no match, move on.
            } // else failed to get name attr
        } // else no neuron node.
        pop_node = pop_node->next_sibling(LVL"Population");
    }
    return numNeurons;
}

void
ModelPreflight::preflight_population (xml_node<>* pop_node)
{
    // Within each population: Find the "population name"; this is
    // actually given by the LL:Neuron name attribute; also have a
    // size attr. Then search out projections.
    xml_node<>* neuron_node = pop_node->first_node(LVL"Neuron");
    if (!neuron_node) {
        // No src name. Does that mean we return or carry on?
        return;
    }

    // Get the name of the population.
    string src_name("");
    xml_attribute<>* name_attr;
    if ((name_attr = neuron_node->first_attribute ("name"))) {
        src_name = name_attr->value();
    } // else failed to get src name

    // Output some info to stdout
    cout << "Preflight: processing population: '" << src_name << "'\n";

    // Now get the component name - this is user specified and there
    // should be an XML file associated with the component with this
    // name + .xml
    string c_name("");
    xml_attribute<>* cname_attr;
    if ((cname_attr = neuron_node->first_attribute ("url"))) {
        c_name = cname_attr->value();
    }
    Util::stripFileSuffix (c_name);

    if (c_name.empty()) {
        stringstream ee;
        ee << "Failed to read component name for population "
           << src_name << "; can't proceed";
        throw runtime_error (ee.str());
    }

    // Can now read additional information about the component, if
    // necessary.
    if (!this->components.count (c_name)) {
        try {
            spineml::Component c (modeldir, c_name);
            this->components.insert (make_pair (c_name, c));
        } catch (const std::exception& e) {
            cerr << "Failed to read component " << c_name << ": " << e.what() << ".\n";
        }
    }

    string src_num("");
    unsigned int src_number(0);
    xml_attribute<>* num_attr;
    if ((num_attr = neuron_node->first_attribute ("size"))) {
        src_num = num_attr->value();
        stringstream src_num_ss;
        src_num_ss << src_num;
        src_num_ss >> src_number;
    } // else failed to get src num

    // Now find all Projections out from the neuron and expand any
    // connections into explict lists, as necessary.
    for (xml_node<>* proj_node = pop_node->first_node(LVL"Projection");
         proj_node;
         proj_node = proj_node->next_sibling(LVL"Projection")) {
        preflight_projection (proj_node, src_name, src_num);
    }

    // Next, replace any Properties with explicit binary data
    for (xml_node<>* prop_node = neuron_node->first_node("Property");
         prop_node;
         prop_node = prop_node->next_sibling("Property")) {

        // If this is a state variable property, then replace it.
        string prop_name("");
        xml_attribute<>* prop_name_attr = prop_node->first_attribute ("name");
        if (prop_name_attr) {
            prop_name = prop_name_attr->value();
        } else {
            throw runtime_error ("Failed to get property name");
        }
        if (this->components.at(c_name).containsStateVariable (prop_name)) {
            // This property is a state variable and not a parameter
            this->replace_statevar_property (prop_node, src_number);
        }
    }
}

void
ModelPreflight::replace_statevar_property (xml_node<>* prop_node,
                                           unsigned int pop_size)
{
    // Depending on what we find in the property, call differing
    // replace methods:
    xml_node<>* fixedvalue_node = prop_node->first_node("FixedValue");
    if (fixedvalue_node) {
        spineml::FixedValue fv (fixedvalue_node, pop_size);
        if (!fv.writeAsBinaryValueList (fixedvalue_node, this->modeldir,
                                        this->nextExplicitDataPath())) {
            // if writeAsBinaryValueList returned false, the explicit
            // data binary path was not used, so decrement it again.
            this->explicitData_binfilenum--;
        }
    }

    xml_node<>* udist_node = prop_node->first_node("UniformDistribution");
    if (udist_node) {
        spineml::UniformDistribution ud (udist_node, pop_size);
        if (!ud.writeAsBinaryValueList (udist_node, this->modeldir,
                                        this->nextExplicitDataPath())) {
            this->explicitData_binfilenum--;
        }
    }

    xml_node<>* ndist_node = prop_node->first_node("NormalDistribution");
    if (ndist_node) {
        spineml::NormalDistribution nd (ndist_node, pop_size);
        if (!nd.writeAsBinaryValueList (ndist_node, this->modeldir,
                                        this->nextExplicitDataPath())) {
            this->explicitData_binfilenum--;
        }
    }

    xml_node<>* vallist_node = prop_node->first_node("ValueList");
    if (vallist_node) {
        spineml::ValueList vl (vallist_node, pop_size);
        if (!vl.writeAsBinaryValueList (vallist_node, this->modeldir,
                                        this->nextExplicitDataPath())) {
            this->explicitData_binfilenum--;
        }
    }
}

string
ModelPreflight::nextExplicitDataPath (void)
{
    string binfilepath ("pf_explicitData");
    stringstream numss;
    numss << this->explicitData_binfilenum++;
    binfilepath += numss.str();
    binfilepath += ".bin";
    return binfilepath;
}

void
ModelPreflight::preflight_projection (xml_node<>* proj_node,
                                      const string& src_name,
                                      const string& src_num)
{
    // Get the destination.
    string dst_population("");
    xml_attribute<>* dst_pop_attr;
    if ((dst_pop_attr = proj_node->first_attribute ("dst_population"))) {
        dst_population = dst_pop_attr->value();
    } // else failed to get src name

    // And then for each synapse in the projection:
    for (xml_node<>* syn_node = proj_node->first_node(LVL"Synapse");
         syn_node;
         syn_node = syn_node->next_sibling(LVL"Synapse")) {
        preflight_synapse (syn_node, src_name, src_num, dst_population);
    }
}

void
ModelPreflight::preflight_synapse (xml_node<>* syn_node,
                                   const string& src_name,
                                   const string& src_num,
                                   const string& dst_population)
{
    // For each synapse... Is there a FixedProbability?
    xml_node<>* fixedprob_connection = syn_node->first_node("FixedProbabilityConnection");
    xml_node<>* connection_list = syn_node->first_node("ConnectionList");
    if (fixedprob_connection) {
        replace_fixedprob_connection (fixedprob_connection, src_name, src_num, dst_population);
    } else if (connection_list) {
        // Check if it's already binary, if not, expand.
        connection_list_to_binary (connection_list, src_name, src_num, dst_population);
    }
}

void
ModelPreflight::connection_list_to_binary (xml_node<>* connlist_node,
                                           const string& src_name,
                                           const string& src_num,
                                           const string& dst_population)
{
    xml_node<>* binaryfile = connlist_node->first_node("BinaryFile");
    if (binaryfile) {
        // No further work to do; this ConnectionList is already a binary list.
        return;
    }

    // Ok, no binary file, so convert.
    spineml::ConnectionList cl;

    // Read XML to get each connection and insert this into ConnectionList.
    int c_idx = 0; // Connection index
    int src, dst; float delay;
    xml_attribute<>* src_attr;
    xml_attribute<>* dst_attr;
    xml_attribute<>* delay_attr;
    for (xml_node<>* conn_node = connlist_node->first_node("Connection");
         conn_node;
         conn_node = conn_node->next_sibling("Connection"), c_idx++) {

        if ((src_attr = conn_node->first_attribute ("src_neuron"))) {
            stringstream ss;
            ss << src_attr->value();
            ss >> src;
        } else {
            throw runtime_error ("Failed to get src_neuron, malformed XML.");
        }

        if ((dst_attr = conn_node->first_attribute ("dst_neuron"))) {
            stringstream ss;
            ss << dst_attr->value();
            ss >> dst;
        } else {
            throw runtime_error ("Failed to get dst_neuron, malformed XML.");
        }

        if ((delay_attr = conn_node->first_attribute ("delay"))) {
            stringstream ss;
            ss << delay_attr->value();
            ss >> delay;
        } else {
            throw runtime_error ("Failed to get delay, malformed XML.");
        }

        // Ensure connectivityS2C is large enough for the sources.
        if (src >= cl.connectivityS2C.size()) {
            cl.connectivityS2C.reserve(++src);
            cl.connectivityS2C.resize(src--);
        }
        cl.connectivityS2C[src].push_back (c_idx); // bad_alloc
        cl.connectivityC2D.push_back (dst);
        cl.connectivityC2Delay.push_back (delay);
    }

    // Lastly, write these out:
    this->write_connection_out (connlist_node, cl);
}

void
ModelPreflight::replace_fixedprob_connection (xml_node<>* fixedprob_node,
                                              const string& src_name,
                                              const string& src_num,
                                              const string& dst_population)
{
    // Get the FixedProbability probabilty and seed from this bit of the model.xml:
    // <FixedProbabilityConnection probability="0.11" seed="123">
    float probabilityValue = 0;
    {
        string fp_probability("");
        xml_attribute<>* fp_probability_attr;
        if ((fp_probability_attr = fixedprob_node->first_attribute ("probability"))) {
            fp_probability = fp_probability_attr->value();
        } else {
            // failed to get probability; can't proceed.
            throw runtime_error ("Failed to get FixedProbability's probability attr from xml");
        }
        stringstream ss;
        ss << fp_probability;
        ss >> probabilityValue;
    }

    int seed = 0;
    {
        string fp_seed("");
        xml_attribute<>* fp_seed_attr;
        if ((fp_seed_attr = fixedprob_node->first_attribute ("seed"))) {
            fp_seed = fp_seed_attr->value();
        } else {
            // failed to get seed; can't proceed.
            throw runtime_error ("Failed to get FixedProbability's seed attr from model.xml");
        }
        stringstream ss;
        ss << fp_seed;
        ss >> seed;
    }

    spineml::ConnectionList cl;
    this->setup_connection_delays (fixedprob_node, cl);

    unsigned int srcNum = 0;
    {
        stringstream ss;
        ss << src_num;
        ss >> srcNum;
    }

    // Find the number of neurons in the destination population
    int dstNum_ = this->find_num_neurons (dst_population);
    unsigned int dstNum(0);
    if (dstNum_ != -1) {
        dstNum = static_cast<unsigned int>(dstNum_);
    } else {
        throw runtime_error ("Failed to find the number of neurons in the destination population.");
    }

    cl.generateFixedProbability (seed, probabilityValue, srcNum, dstNum);
    cl.generateDelays();

    this->write_connection_out (fixedprob_node, cl);
}

void
ModelPreflight::setup_connection_delays (xml_node<>* parent_node, ConnectionList& cl)
{
    // The connection list object which we'll populate.
    float dimMultiplier = 1.0;
    xml_node<>* delay_node = parent_node->first_node ("Delay");
    if (delay_node) {
        xml_attribute<>* dim_attr = delay_node->first_attribute ("Dimension");
        if (dim_attr) {
            cl.delayDimension = dim_attr->value();
            if (cl.delayDimension == "ms") {
                // This is the dimension we need - all delays in
                // the ConnectionList need to be stored in
                // ms. Leave dimMultiplier at its original value
                // of 1.
            } else if (cl.delayDimension == "s") {
                dimMultiplier = 1000.0; // to convert to ms
            } else {
                stringstream ee;
                ee << "Unknown delay dimension '" << cl.delayDimension << "'";
                throw runtime_error (ee.str());
            }
        }
        // Delays can be fixed value, uniform, normal or "none". If "none" then the
        // Delay element just looks like this: <Delay dimension="ms"/>
        xml_node<>* delay_value_node = delay_node->first_node ("FixedValue");
        xml_node<>* delay_normal_node = delay_node->first_node ("NormalDistribution");
        xml_node<>* delay_uniform_node = delay_node->first_node ("UniformDistribution");
        if (delay_value_node) {
            cl.delayDistributionType = spineml::Dist_FixedValue;
            xml_attribute<>* value_attr = delay_value_node->first_attribute ("value");
            if (value_attr) {
                stringstream ss;
                ss << value_attr->value();
                ss >> cl.delayFixedValue;
                cl.delayFixedValue *= dimMultiplier;
            }
        } else if (delay_normal_node) {
            cl.delayDistributionType = spineml::Dist_Normal;
            xml_attribute<>* mean_attr = delay_normal_node->first_attribute ("mean");
            if (mean_attr) {
                stringstream ss;
                ss << mean_attr->value();
                ss >> cl.delayMean;
                cl.delayMean *= dimMultiplier;
            }
            xml_attribute<>* variance_attr = delay_normal_node->first_attribute ("variance");
            if (variance_attr) {
                stringstream ss;
                ss << variance_attr->value();
                ss >> cl.delayVariance;
                cl.delayVariance *= dimMultiplier;
            }
            xml_attribute<>* seed_attr = delay_normal_node->first_attribute ("seed");
            if (seed_attr) {
                stringstream ss;
                ss << seed_attr->value();
                ss >> cl.delayDistributionSeed;
            }
        } else if (delay_uniform_node) {
            cl.delayDistributionType = spineml::Dist_Uniform;
            xml_attribute<>* minimum_attr = delay_uniform_node->first_attribute ("minimum");
            if (minimum_attr) {
                stringstream ss;
                ss << minimum_attr->value();
                ss >> cl.delayRangeMin;
                cl.delayRangeMin *= dimMultiplier;
            }
            xml_attribute<>* maximum_attr = delay_uniform_node->first_attribute ("maximum");
            if (maximum_attr) {
                stringstream ss;
                ss << maximum_attr->value();
                ss >> cl.delayRangeMax;
                cl.delayRangeMax *= dimMultiplier;
            }
            xml_attribute<>* seed_attr = delay_uniform_node->first_attribute ("seed");
            if (seed_attr) {
                stringstream ss;
                ss << seed_attr->value();
                ss >> cl.delayDistributionSeed;
            }
        }
    }
}

void
ModelPreflight::write_connection_out (xml_node<>* parent_node, ConnectionList& cl)
{
    string binfilepath ("pf_connection");
    stringstream numss;
    numss << this->binfilenum++;
    binfilepath += numss.str();
    binfilepath += ".bin";
    cl.write (parent_node, this->modeldir, binfilepath);
}

#define STRLEN_PROPERTY 8
xml_node<>*
ModelPreflight::findProperty (xml_node<>* current_node,
                              const std::string& parentName,
                              const std::string& containerName,
                              const std::string& propertyName)
{
    xml_node<>* rtn = static_cast<xml_node<>*>(0);

    if (current_node == rtn /* i.e. static_cast<xml_node<>*>(0) */) {
        current_node = this->root_node;
    }

    // 1. Is current_node a Property?
    string cname = current_node->name();

    xml_attribute<>* nattr = current_node->first_attribute ("name");
    string pname("");
    if (nattr) {
        pname = nattr->value();
    }

    if (current_node->name_size() == STRLEN_PROPERTY && cname == "Property") {
        // This node is a Property. Does the name of the parent match?
        if (parentName == containerName) {
            // Yes, matches. does name attribute of this Property match propertyName?q
            if (!nattr) {
                // no match, we don't have a property name attribute.
            } else {
                if (pname == propertyName) {
                    // Match!
                    rtn = current_node;
                } else {
                    // No match, wrong property name
                }
            }
        } else {
            // This is a Property, but it has the wrong parent
            // container: parentName != containerName.
        }

    } else {
        // Not a property, so search down then sideways
        xml_node<>* next_node;
        for (next_node = current_node->first_node();
             next_node;
             next_node = next_node->next_sibling()) {

            if ((rtn = this->findProperty (next_node, pname, containerName, propertyName)) != static_cast<xml_node<>*>(0)) {
                // next_node was a property of interest!
                break;
            }
        }
    }

    return rtn;
}
