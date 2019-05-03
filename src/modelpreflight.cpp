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

rapidxml::xml_attribute<>*
ModelPreflight::allocate_attribute (const std::string& attr_name, const std::string& attr_value)
{
    char* _alloced = this->doc.allocate_string (attr_value.c_str());
    rapidxml::xml_attribute<>* _attr = this->doc.allocate_attribute ("probability", _alloced);
    return _attr;
}

void
ModelPreflight::write (void)
{
    string filepath = this->modeldir + this->modelfile;

    // If requested, backup model.xml:
    if (this->backup == true) {
        stringstream cmd;
        cmd << "cp " << filepath << " " << filepath << ".bu";
        int rtn = system (cmd.str().c_str());
        if (rtn) {
            stringstream ee;
            ee << "ModelPreflight::write: Call to cp failed with return code: "
               << rtn << endl;
        }
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
    this->first_pop_node = this->root_node->first_node(LVL"Population");
    xml_node<>* pop_node = this->first_pop_node;
    for (pop_node = this->root_node->first_node(LVL"Population");
         pop_node;
         pop_node = pop_node->next_sibling(LVL"Population")) {
        this->preflight_population (pop_node);
    }
}

void
ModelPreflight::preflight (const std::vector<DelayChange>& exptDelayChanges)
{
    this->init();
    this->delayChanges = exptDelayChanges;
    // Search each population for stuff.
    this->first_pop_node = this->root_node->first_node(LVL"Population");
    xml_node<>* pop_node = this->first_pop_node;
    for (pop_node = this->root_node->first_node(LVL"Population");
         pop_node;
         pop_node = pop_node->next_sibling(LVL"Population")) {
        this->preflight_population (pop_node);
    }
}

set<string>
ModelPreflight::get_component_set (void)
{
    set<string> component_list;
    this->init();
    // Search each population for stuff.
    this->first_pop_node = this->root_node->first_node(LVL"Population");
    xml_node<>* pop_node = this->first_pop_node;
    for (pop_node = this->root_node->first_node(LVL"Population");
         pop_node;
         pop_node = pop_node->next_sibling(LVL"Population")) {
        string pname = this->get_population_component_name (pop_node);
        if (!pname.empty()) {
            component_list.insert(pname);
        }
    }
    return component_list;
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

string
ModelPreflight::get_population_component_name (xml_node<>* pop_node)
{
    string c_name("");
    xml_node<>* neuron_node = pop_node->first_node(LVL"Neuron");
    if (neuron_node) {
        c_name = this->get_component_name (neuron_node);
    }
    return c_name;
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
    string pop_name("");
    xml_attribute<>* name_attr;
    if ((name_attr = neuron_node->first_attribute ("name"))) {
        pop_name = name_attr->value();
    } // else failed to get src name

    // Now get the component name - this is user specified and there
    // should be an XML file associated with the component with this
    // name + .xml
    string c_name = this->get_component_name (neuron_node);

    string pop_num("");
    unsigned int pop_number(0);
    xml_attribute<>* num_attr;
    if ((num_attr = neuron_node->first_attribute ("size"))) {
        pop_num = num_attr->value();
        stringstream pop_num_ss;
        pop_num_ss << pop_num;
        pop_num_ss >> pop_number;
    } // else failed to get src num

    // Output some info to stdout
    cout << "Preflight: processing population: '" << pop_name << "' (size " << pop_num << ")\n";

    // Now find all Projections out from the neuron and expand any
    // connections into explicit lists, as
    // necessary. preflight_projection() also expands state variable
    // properties into binary data.
    for (xml_node<>* proj_node = pop_node->first_node(LVL"Projection");
         proj_node;
         proj_node = proj_node->next_sibling(LVL"Projection")) {
        // pop_name and pop_num are "src_name" and "src_num" for projections.
        this->preflight_projection (proj_node, pop_name, pop_num);
    }

    // Now do a similar thing for connections which are not projections.
    for (xml_node<>* input_node = neuron_node->first_node(LVL"Input");
         input_node;
         input_node = input_node->next_sibling(LVL"Input")) {
        // A difference here is that pop_name and pop_num are the "dest_name" and
        // "dst_num", because the <Input> element sits inside the DESTINATION population,
        // whereas Projection elements sit inside the source population. The source
        // population can be found from the Input attribute "src".
        //cout << "preflight_input for current name/num: " << pop_name << "," << pop_num << "\n";
        this->preflight_input (input_node, pop_name, pop_num);
    }

    if (c_name == "SpikeSource") {
        // No statevar properties to change in the special neuron type "SpikeSource"
        return;
    }

    // Next, replace any Properties with explicit binary data in the
    // Populations neuron_node (Is there only one of these? What if
    // neuron is compartmentalised?)
    for (xml_node<>* prop_node = neuron_node->first_node("Property");
         prop_node;
         prop_node = prop_node->next_sibling("Property")) {
        this->try_replace_statevar_property (prop_node, pop_number, c_name);
    }
}

void
ModelPreflight::try_replace_statevar_property (xml_node<>* prop_node,
                                               unsigned int pop_size,
                                               const string& component_name)
{
    // If this is a state variable property, then replace it.
    string prop_name("");
    xml_attribute<>* prop_name_attr = prop_node->first_attribute ("name");
    if (prop_name_attr) {
        prop_name = prop_name_attr->value();
    } else {
        throw runtime_error ("Failed to get property name");
    }

    if (this->components.at(component_name).containsStateVariable (prop_name)) {
        // This property is a state variable and not a parameter
        this->replace_statevar_property (prop_node, pop_size);
    }
}

void
ModelPreflight::replace_statevar_property (xml_node<>* prop_node,
                                           unsigned int pop_size)
{
    // Depending on what we find in the property, call differing
    // replace methods:
    xml_node<>* fixedvalue_node = prop_node->first_node("FixedValue");
    xml_node<>* udist_node = prop_node->first_node("UniformDistribution");
    xml_node<>* ndist_node = prop_node->first_node("NormalDistribution");
    xml_node<>* vallist_node = prop_node->first_node("ValueList");

    if (udist_node) {
        spineml::UniformDistribution ud (udist_node, pop_size);
        if (!ud.writeAsBinaryValueList (udist_node, this->modeldir,
                                        this->nextExplicitDataPath())) {
            this->explicitData_binfilenum--;
        }

    } else if (ndist_node) {
        spineml::NormalDistribution nd (ndist_node, pop_size);
        if (!nd.writeAsBinaryValueList (ndist_node, this->modeldir,
                                        this->nextExplicitDataPath())) {
            this->explicitData_binfilenum--;
        }

    } else if (vallist_node) {
        spineml::ValueList vl (vallist_node, pop_size);
        if (!vl.writeAsBinaryValueList (vallist_node, this->modeldir,
                                        this->nextExplicitDataPath())) {
            this->explicitData_binfilenum--;
        }

    } else if (fixedvalue_node) {
        spineml::FixedValue fv (fixedvalue_node, pop_size);
        if (!fv.writeAsBinaryValueList (fixedvalue_node, this->modeldir,
                                        this->nextExplicitDataPath())) {
            // if writeAsBinaryValueList returned false, the explicit
            // data binary path was not used, so decrement it again.
            this->explicitData_binfilenum--;
        }

    } else {
        // none of the above - assume property is empty and so treat
        // as if it had FixedValue 0.
        // First create node fixedvalue_node and then add it as a child
        // to prop_node.
        // NB: expect fixedvalue_node to be null at this point.
        fixedvalue_node = doc.allocate_node (node_element, "FixedValue");
        prop_node->prepend_node (fixedvalue_node);

        // Now create a new fv object and write it out into fixedvalue_node.
        spineml::FixedValue fv; // (fixedvalue_node, pop_size);
        fv.setValue (0.0);
        fv.setNumInPopulation (pop_size);
        if (!fv.writeAsBinaryValueList (fixedvalue_node, this->modeldir,
                                        this->nextExplicitDataPath())) {
            // if writeAsBinaryValueList returned false, the explicit
            // data binary path was not used, so decrement it again.
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
    } // else failed to get attribute

    cout << "PreFlight: processing projection " << src_name << " to " << dst_population << endl;

    // And then for each synapse in the projection:
    int src_syn_num = 0;
    for (xml_node<>* syn_node = proj_node->first_node(LVL"Synapse");
         syn_node;
         syn_node = syn_node->next_sibling(LVL"Synapse")) {
        this->preflight_synapse (syn_node, src_name, src_syn_num, src_num, dst_population);
        ++src_syn_num;
    }
}

void
ModelPreflight::preflight_input (xml_node<>* input_node,
                                 const string& dest_name,
                                 const string& dest_num)
{
    string src_name("");
    xml_attribute<>* src_name_attr;
    if ((src_name_attr = input_node->first_attribute ("src"))) {
        src_name = src_name_attr->value();
    } // else failed to get src name

    string src_port("");
    xml_attribute<>* src_port_attr;
    if ((src_port_attr = input_node->first_attribute ("src_port"))) {
        src_port = src_port_attr->value();
    } // else failed to get src port

    string dst_port("");
    xml_attribute<>* dst_port_attr;
    if ((dst_port_attr = input_node->first_attribute ("dst_port"))) {
        dst_port = dst_port_attr->value();
    } // else failed to get dst port

    cout << "PreFlight: processing generic input " << src_name << "/" << src_port
         << " to " << dest_name << "/" << dst_port << endl;
    float fixedDelay = this->searchDelayChanges (src_name, src_port, dest_name, dst_port);

    // Now just replace any FixedProbability or ConnectionList nodes.
    xml_node<>* fixedprob_connection = input_node->first_node("FixedProbabilityConnection");
    xml_node<>* connection_list = input_node->first_node("ConnectionList");
    if (fixedprob_connection) {
        // Find the number of neurons in the destination population
        int srcNum_ = this->find_num_neurons (src_name);
        string src_num("");
        if (srcNum_ != -1) {
            stringstream ss;
            ss << srcNum_;
            src_num = ss.str();
        } else {
            stringstream ee;
            ee << "Failed to find the number of neurons in the src population '"
               << src_name << "'";
            throw runtime_error (ee.str());
        }
        this->replace_fixedprob_connection (fixedprob_connection, src_num, dest_num, fixedDelay);
    } else if (connection_list) {
        // Check if it's already binary, if not, expand.
        this->connection_list_to_binary (connection_list, fixedDelay);
    }
}

float
ModelPreflight::searchDelayChanges (const string& src, const string& dst,
                                    const string& synapseNum)
{
    vector<DelayChange>::const_iterator i = this->delayChanges.begin();
    while (i != this->delayChanges.end()) {
        if (i->matches(src, dst, synapseNum)) {
            return i->delay;
        }
        ++i;
    }
    // No match, so return <0
    return -1.0;
}

float
ModelPreflight::searchDelayChanges (const string& src, const string& srcPort,
                                    const string& dst, const string& dstPort)

{
    vector<DelayChange>::const_iterator i = this->delayChanges.begin();
    while (i != this->delayChanges.end()) {
        if (i->matches(src, srcPort, dst, dstPort)) {
            return i->delay;
        }
        ++i;
    }
    // No match, so return <0
    return -1.0;
}

void
ModelPreflight::preflight_synapse (xml_node<>* syn_node,
                                   const string& src_name,
                                   int src_syn_num,
                                   const string& src_num,
                                   const string& dst_population)
{
    string synapse_num("");
    {
        stringstream synss;
        synss << src_syn_num;
        synss >> synapse_num;
    }
    // Does this synapse match any of the DelayChanges? If it does,
    // then 0 <= delay <= inf is returned. Otherwise, -1 is returned.
    float fixedDelay = this->searchDelayChanges (src_name, dst_population, synapse_num);

    // For each synapse... Is there a FixedProbability?
    xml_node<>* fixedprob_connection = syn_node->first_node("FixedProbabilityConnection");
    xml_node<>* connection_list = syn_node->first_node("ConnectionList");
    if (fixedprob_connection) {
        // Find the number of neurons in the destination population
        int dstNum_ = this->find_num_neurons (dst_population);
        string dst_num("");
        if (dstNum_ != -1) {
            stringstream ss;
            ss << dstNum_;
            dst_num = ss.str();
        } else {
            stringstream ee;
            ee << "Failed to find the number of neurons in the destination population '"
               << dst_population << "'";
            throw runtime_error (ee.str());
        }
        this->replace_fixedprob_connection (fixedprob_connection, src_num, dst_num, fixedDelay);
    } else if (connection_list) {
        // Check if it's already binary, if not, expand.
        this->connection_list_to_binary (connection_list, fixedDelay);
    }

    int dstNum_ = this->find_num_neurons (dst_population);
    unsigned int dstNum(0);
    if (dstNum_ != -1) {
        dstNum = static_cast<unsigned int>(dstNum_);
    } else {
        stringstream ee;
        ee << "Failed to find the number of neurons in the destination population '" << dst_population << "'";
        throw runtime_error (ee.str());
    }

    // Expand any Property nodes within the <LL:PostSynapse> node.
    xml_node<>* postsynapse_node = syn_node->first_node(LVL"PostSynapse");
    if (postsynapse_node) {
        string postsyn_cmpt_name = this->get_component_name (postsynapse_node);
        // Now do the property replacements
        for (xml_node<>* prop_node = postsynapse_node->first_node("Property");
             prop_node;
             prop_node = prop_node->next_sibling("Property")) {
            // size should be size of dest population
            this->try_replace_statevar_property (prop_node, dstNum, postsyn_cmpt_name);
        }
    }

    // Expand any Property nodes within the <LL:WeightUpdate> node.
    xml_node<>* weightupdate_node = syn_node->first_node(LVL"WeightUpdate");
    if (weightupdate_node) {
        string wu_cmpt_name = this->get_component_name (weightupdate_node);
        // Now do the property replacements
        for (xml_node<>* prop_node = weightupdate_node->first_node("Property");
             prop_node;
             prop_node = prop_node->next_sibling("Property")) {
            // Size should be number of connections. LL:Synapse will
            // contain a OneToOneConnection or an AllToAllConnection
            // or a ConnectionList containing a BinaryFile with some
            // num_connections.
            unsigned int srcNum = 0;
            {
                stringstream ss;
                ss << src_num;
                ss >> srcNum;
            }
            unsigned int num_connections = this->get_num_connections (syn_node, srcNum, dstNum);
            this->try_replace_statevar_property (prop_node, num_connections, wu_cmpt_name);
        }
    }
}

unsigned int
ModelPreflight::get_num_connections (xml_node<>* synapse_node,
                                     unsigned int num_in_src_population,
                                     unsigned int num_in_dst_population)
{
    // Although there can also be FixedProbability and
    // python-generated connections, we don't expect to see those
    // here. python-generated will at this point be ConnectionLists
    // and FixedProbability should have been previously preflighted
    // into a Connectionlist.
    xml_node<>* onetoone_node = synapse_node->first_node("OneToOneConnection");
    xml_node<>* alltoall_node = synapse_node->first_node("AllToAllConnection");
    xml_node<>* conn_list_node = synapse_node->first_node("ConnectionList");

    unsigned int rtn = 0;
    if (onetoone_node) {
        rtn = num_in_dst_population;
    } else if (alltoall_node) {
        rtn = num_in_src_population * num_in_dst_population;
    } else if (conn_list_node) {
        xml_node<>* binaryfile_node = conn_list_node->first_node ("BinaryFile");
        if (binaryfile_node) {
            xml_attribute<>* num_conn_attr = binaryfile_node->first_attribute ("num_connections");
            if (num_conn_attr) {
                stringstream ss;
                ss << num_conn_attr->value();
                ss >> rtn;
            }
        }
    } // else we return 0.

    return rtn;
}

std::string
ModelPreflight::get_component_name (xml_node<>* component_node)
{
    // We have a postsynapse; what's its component name?
    string cmpt_name("");
    if (!component_node) {
        throw runtime_error ("Failed to read component name; component_node is null");
    }
    xml_attribute<>* cname_attr;
    if ((cname_attr = component_node->first_attribute ("url"))) {
        cmpt_name = cname_attr->value();
    }
    Util::stripFileSuffix (cmpt_name);

    if (cmpt_name.empty()) {
        throw runtime_error ("Failed to read component name; can't proceed");
    } else if (cmpt_name == "SpikeSource") {
        // A SpikeSource is a special population which doesn't have a
        // component xml file associated with it. Return "SpikeSource"
        // as component name.
        return cmpt_name;
    }

    // Can now read additional information about the component, if
    // necessary.
    if (!this->components.count (cmpt_name)) {
        try {
            spineml::Component c (modeldir, cmpt_name);
            this->components.insert (make_pair (cmpt_name, c));
        } catch (const std::exception& e) {
            stringstream ee;
            ee << "Failed to read component " << cmpt_name << ": " << e.what() << ".\n";
            throw runtime_error (ee.str());
        }
    }

    return cmpt_name;
}

void
ModelPreflight::connection_list_to_binary (xml_node<>* connlist_node,
                                           float fixedValDelayChange)
{
    xml_node<>* binaryfile = connlist_node->first_node("BinaryFile");
    if (binaryfile && fixedValDelayChange < 0.0) {
        // No further work to do; this ConnectionList is already a
        // binary list and there's no override.
        return;
    }

    // Ok, no binary file, so convert.
    spineml::ConnectionList cl;

    // First see if we have a Delay element, and what
    // that delay is, so that we can assign delays to the Connections.
    bool have_delay_element = this->setup_connection_delays (connlist_node, cl,
                                                             fixedValDelayChange);

    // Read XML to get each connection and insert this into
    // the ConnectionList object.
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
            // Explicit delays in Connection elements mean that the
            // delays are an explicit list.
            if (cl.delayDistributionType != spineml::Dist_ExplicitList) {
                cl.delayDistributionType = spineml::Dist_ExplicitList;
            }
        } else if (have_delay_element) {
            // It's ok for a Connection not to have a delay attribute,
            // but in that case, a ConnectionList needs to contain a
            // Delay element.
            delay = -1; // -1 means "don't add delay to connectivityC2Delay"
        } else {
            throw runtime_error ("Failed to get a delay attribute for this "
                                 "Connection and there is no Delay element to use.");
        }

        // Ensure connectivityS2C is large enough for the sources.
        if (static_cast<unsigned int>(src) >= cl.connectivityS2C.size()) {
            cl.connectivityS2C.reserve(++src);
            cl.connectivityS2C.resize(src--);
        }
        cl.connectivityS2C[src].push_back (c_idx); // bad_alloc
        cl.connectivityC2D.push_back (dst);
        if (delay > -1) {
            cl.connectivityC2Delay.push_back (delay);
        }
    }

    // If the ConnectionList contained a Delay element, we have to
    // generate the delays before writing the connection out.
    if (have_delay_element || fixedValDelayChange >= 0) {
        cl.generateDelays();
    }

    // Lastly, write these out:
    this->write_connection_out (connlist_node, cl);
}

void
ModelPreflight::replace_fixedprob_connection (xml_node<>* fixedprob_node,
                                              const string& src_num, const string& dst_num,
                                              float fixedValDelayChange)
{
    // Get the FixedProbability probability and seed from this bit of the model.xml:
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
    this->setup_connection_delays (fixedprob_node, cl, fixedValDelayChange);

    unsigned int srcNum = 0;
    {
        stringstream ss;
        ss << src_num;
        ss >> srcNum;
    }
    unsigned int dstNum = 0;
    {
        stringstream ss;
        ss << dst_num;
        ss >> dstNum;
    }

    cl.generateFixedProbability (seed, probabilityValue, srcNum, dstNum);
    cl.generateDelays();

    this->write_connection_out (fixedprob_node, cl);
}

bool
ModelPreflight::setup_connection_delays (xml_node<>* parent_node,
                                         ConnectionList& cl,
                                         float fixedValDelayChange)
{
    bool have_delay_element = false;
    // The connection list object which we'll populate.
    float dimMultiplier = 1.0;
    xml_node<>* delay_node = parent_node->first_node ("Delay");
    if (delay_node || fixedValDelayChange >= 0.0) {
        xml_node<>* delay_value_node = (xml_node<>*)0;
        xml_node<>* delay_normal_node = (xml_node<>*)0;
        xml_node<>* delay_uniform_node = (xml_node<>*)0;
        if (delay_node) {
            have_delay_element = true;
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
            delay_value_node = delay_node->first_node ("FixedValue");
            delay_normal_node = delay_node->first_node ("NormalDistribution");
            delay_uniform_node = delay_node->first_node ("UniformDistribution");
        }

        if (fixedValDelayChange >= 0.0) {
            cl.delayDistributionType = spineml::Dist_FixedValue;
            cl.delayFixedValue = fixedValDelayChange; // always in ms.

        } else if (delay_value_node) {
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

    return have_delay_element;
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

rapidxml::xml_node<>*
ModelPreflight::findNamedElement (rapidxml::xml_node<>* current_node, std::string& elementName)
{
    xml_node<>* rtn = static_cast<xml_node<>*>(0);
    if (current_node == rtn /* i.e. static_cast<xml_node<>*>(0) */) {
        current_node = this->root_node;
    }
    string cname = current_node->name();
    if (current_node->name_size() == elementName.size() && cname == elementName) {
        // Match
        rtn = current_node;
    } else {
        // Not a match, so search down then sideways
        xml_node<>* next_node;
        for (next_node = current_node->first_node(); next_node; next_node = next_node->next_sibling()) {
            if ((rtn = this->findNamedElement (next_node, elementName)) != static_cast<xml_node<>*>(0)) {
                break; // found elementName
            }
        }
    }
    return rtn;
}

rapidxml::xml_node<>*
ModelPreflight::findNamedParent (rapidxml::xml_node<>* current_node, std::string& elementName)
{
    xml_node<>* rtn = static_cast<xml_node<>*>(0);
    if (current_node == rtn /* i.e. static_cast<xml_node<>*>(0) */) {
        current_node = this->root_node;
    }
    string cname = current_node->name();
    if (current_node->name_size() == elementName.size() && cname == elementName) {
        // Match
        rtn = current_node;
    } else {
        // Not a match, so search up.
        if ((rtn = this->findNamedParent (current_node->parent(), elementName)) != static_cast<xml_node<>*>(0)) {
            // found elementName
        }
    }
    return rtn;
}

#define STRLEN_LLINPUT 8
rapidxml::xml_node<>*
ModelPreflight::findLLInput (rapidxml::xml_node<>* current_node,
                             const std::string& parentName,
                             const std::string& src,
                             const std::string& srcPort,
                             const std::string& dst,
                             const std::string& dstPort)
{
    xml_node<>* rtn = static_cast<xml_node<>*>(0);

    if (current_node == rtn /* i.e. static_cast<xml_node<>*>(0) */) {
        current_node = this->root_node;
    }

    // 1. Is current_node an LL:Input?
    string cname = current_node->name();

    xml_attribute<>* nattr = current_node->first_attribute ("name");
    string pname("");
    if (nattr) {
        pname = nattr->value();
    }

    if (current_node->name_size() == STRLEN_LLINPUT && cname == "LL:Input") {
        // This node is an LL:Input. Does the name of the parent match?
        if (parentName == dst) {
            // Parent container name matches the dst attribute in the
            // LL:Input. do the src, srcPort and dstPort attributes of
            // this Input match?
            xml_attribute<>* srcAttr = current_node->first_attribute ("src");
            xml_attribute<>* srcPortAttr = current_node->first_attribute ("src_port");
            xml_attribute<>* dstPortAttr = current_node->first_attribute ("dst_port");

            if (!srcAttr || !srcPortAttr || !dstPortAttr) {
                // no match, we don't have all the requried attributes in this Input element
            } else {
                string srcStr = srcAttr->value();
                string srcPortStr = srcPortAttr->value();
                string dstPortStr = dstPortAttr->value();

                if (srcStr == src && srcPortStr == srcPort && dstPortStr == dstPort) {
                    // Match!
                    rtn = current_node;
                } // else no match.
            }
        } // else this is an LL:Input, but it has the wrong parent for the specified dst.

    } else {
        // Not an LL:Input, so search down then sideways
        xml_node<>* next_node;
        for (next_node = current_node->first_node();
             next_node;
             next_node = next_node->next_sibling()) {

            if ((rtn = this->findLLInput (next_node, pname, src, srcPort, dst, dstPort)) != static_cast<xml_node<>*>(0)) {
                // next_node was an LL:Input of interest!
                break;
            }
        }
    }

    return rtn;
}

#define STRLEN_LLWEIGHTUPDATE 15
rapidxml::xml_node<>*
ModelPreflight::findLLWeightUpdate (rapidxml::xml_node<>* current_node,
                                    const std::string& name)
{
    xml_node<>* rtn = static_cast<xml_node<>*>(0);

    if (current_node == rtn /* i.e. static_cast<xml_node<>*>(0) */) {
        current_node = this->root_node;
    }

    // 1. Is current_node an LL:WeightUpdate?
    string cname = current_node->name();

    xml_attribute<>* nattr = current_node->first_attribute ("name");
    string pname("");
    if (nattr) {
        pname = nattr->value();
    }

    if (current_node->name_size() == STRLEN_LLWEIGHTUPDATE && cname == "LL:WeightUpdate") {
        // This node is an LL:WeightUpdate. Does its name match name?
        if (pname == name) {
            // Match!
            rtn = current_node;
        } // else no match.

    } else {
        // Not an LL:WeightUpdate, so search down then sideways
        xml_node<>* next_node;
        for (next_node = current_node->first_node();
             next_node;
             next_node = next_node->next_sibling()) {

            if ((rtn = this->findLLWeightUpdate (next_node, name)) != static_cast<xml_node<>*>(0)) {
                // next_node was an LL:WeightUpdate of interest!
                break;
            }
        }
    }

    return rtn;
}

#ifdef EXPLICIT_BINARY_DATA_CONVERSION
#include <cstdio>

void
ModelPreflight::binaryDataFloatToDouble (bool forwards)
{
    // Iterate through model. For each binaryDatafile thing, convert.
    this->init();
    this->binaryDataF2D = forwards;
    if (this->binaryDataF2D) {
        cout << "PreFlight: Float to double conversion requested" << endl;
    } else {
        cout << "PreFlight: Double to float conversion requested" << endl;
    }
    // On run 1, this checks that all binary files have correct size.
    this->findExplicitData (static_cast<xml_node<>*>(0), 1);
    cout << "PreFlight: binaryDataFiles can be converted; proceeding!\n";
    // On run 2, the binary files are modified.
    this->findExplicitData (static_cast<xml_node<>*>(0), 2);
}

void
ModelPreflight::binaryDataDoubleToFloat (void)
{
    this->binaryDataFloatToDouble (false);
}

#define STRLEN_BINARYFILE 10
xml_node<>*
ModelPreflight::findExplicitData (xml_node<>* current_node, const unsigned int& run)
{
    xml_node<>* rtn = static_cast<xml_node<>*>(0);

    if (current_node == rtn /* i.e. static_cast<xml_node<>*>(0) */) {
        if (this->root_node == rtn) {
            throw runtime_error ("findExplicitData: root_node is not allocated.");
        }
        current_node = this->root_node;
    }

    // 1. Is current_node a BinaryFile?
    string cname = current_node->name();

    if (current_node->name_size() == STRLEN_BINARYFILE && cname == "BinaryFile") {
        // This node is a BinaryFile element. Is it connectionN.bin
        // (ignore) or explicitDataBinaryFileN.bin (modify)?
        xml_attribute<>* nattr = current_node->first_attribute ("file_name");
        string bf_fname("");
        if (nattr) {
            bf_fname = nattr->value();
        }

        if (bf_fname.substr(0,12) == "explicitData") {
            // Modify!
            if (run == 1) {
                this->binaryDataVerify (current_node);
            } else if (run == 2) {
                this->binaryDataModify (current_node);
            } else {
                throw runtime_error ("unexpected run number.");
            }
            rtn = current_node;
        }

    } else {
        // Not a BinaryFile, so search down then sideways
        xml_node<>* next_node;
        for (next_node = current_node->first_node();
             next_node;
             next_node = next_node->next_sibling()) {

            if ((rtn = this->findExplicitData (next_node, run)) != static_cast<xml_node<>*>(0)) {
                // next_node was a BinaryFile of interest!
            }
        }
    }

    return rtn;
}

void
ModelPreflight::binaryDataVerify (xml_node<>* binaryfile_node)
{
    xml_attribute<>* nattr = binaryfile_node->first_attribute ("file_name");
    string bf_fname("");
    if (nattr) {
        bf_fname = nattr->value();
    }
    xml_attribute<>* nelemattr = binaryfile_node->first_attribute ("num_elements");
    stringstream ss;
    unsigned int num_elements = 0;
    if (nelemattr) {
        ss << nelemattr->value();
        ss >> num_elements;
    }

    cout << "PreFlight: Verify file " << bf_fname << " which has  " << num_elements << " elements\n";

    string fname = this->modeldir + bf_fname;
    ifstream f;
    f.open (fname.c_str(), ios::in);
    if (!f.is_open()) {
        stringstream ee;
        ee << "binaryDataVerify: Failed to open file " << fname << " for reading";
    }
    // Get size;
    f.seekg (0, ios::end);
    unsigned int nbytes = f.tellg();
    f.close();

    //cout << "num_elements=" << num_elements
    //     << " nbytes=" << nbytes << " nbytes/8=" << nbytes/8
    //     << " nbytes/12=" << nbytes/12 << endl;
    if (nbytes/8 == num_elements) {
        // We're in int,float format.
        if (this->binaryDataF2D == true) {
            // Good, can move on
        } else {
            // Bad, tasked to do double to float conversion, but data already float
            throw runtime_error ("explicitBinaryData is already in int,float format.");
        }
    } else if (nbytes/12 == num_elements) {
        // We're in int,double format.
        if (this->binaryDataF2D == false) {
            // Good, can move on and do double2float conversion
        } else {
            // Bad, tasked to do float to double conversion, but data already double
            throw runtime_error ("explicitBinaryData is already in int,double format.");
        }
    } else {
        throw runtime_error ("Wrong number of bytes in explicitBinaryData");
    }
}

void
ModelPreflight::binaryDataModify (xml_node<>* binaryfile_node)
{
    xml_attribute<>* nattr = binaryfile_node->first_attribute ("file_name");
    string bf_fname("");
    if (nattr) {
        bf_fname = nattr->value();
    }
    xml_attribute<>* nelemattr = binaryfile_node->first_attribute ("num_elements");
    stringstream ss;
    unsigned int num_elements = 0;
    if (nelemattr) {
        ss << nelemattr->value();
        ss >> num_elements;
    }

    cout << "PreFlight: Modify file " << bf_fname << " which has  " << num_elements << " elements\n";

    // When this code runs, the verify function should already have run.
    string fname = this->modeldir + bf_fname;
    ifstream f;
    f.open (fname.c_str(), ios::in);
    if (!f.is_open()) {
        stringstream ee;
        ee << "binaryDataModify: Failed to open file " << fname << " for reading";
    }
    string tmpfname = fname + ".out";
    ofstream o;
    o.open (tmpfname.c_str(), ios::out|ios::trunc);
    if (!o.is_open()) {
        stringstream ee;
        ee << "binaryDataModify: Failed to open file " << tmpfname << " for writing";
    }

    int index; float value; double dvalue;
    if (this->binaryDataF2D == true) {
        // Float to double conversion
        try {
            while (!f.eof()) {
                f.read (reinterpret_cast<char*>(&index), sizeof index);
                if (f.eof()) {
                    // Finished reading.
                    break;
                }
                o.write (reinterpret_cast<char*>(&index), sizeof index);
                f.read (reinterpret_cast<char*>(&value), sizeof value);
                if (f.eof()) {
                    cout << "PreFlight: Finished reading in unexpected location\n";
                    break;
                }
                dvalue = static_cast<double>(value);
                o.write (reinterpret_cast<char*>(&dvalue), sizeof dvalue);
            }
        } catch (const std::exception& e) {
            throw runtime_error ("Exception copying");
        }
    } else {
        // Double to float conversion
        try {
            while (!f.eof()) {
                f.read (reinterpret_cast<char*>(&index), sizeof index);
                if (f.eof()) {
                    // Finished reading
                    break;
                }
                o.write (reinterpret_cast<char*>(&index), sizeof index);
                f.read (reinterpret_cast<char*>(&dvalue), sizeof dvalue);
                if (f.eof()) {
                    cout << "PreFlight: Finished reading in unexpected location\n";
                    break;
                }
                value = static_cast<float>(dvalue);
                o.write (reinterpret_cast<char*>(&value), sizeof value);
            }
        } catch (const std::exception& e) {
            throw runtime_error ("Exception copying");
        }
    }
    o.close();
    f.close();

    // Rename files
    string bufname = fname + ".bu";
    // Move existing file to file.bu
    rename (fname.c_str(), bufname.c_str());
    // Move new file.out to file
    rename (tmpfname.c_str(), fname.c_str());
}

#endif // EXPLICIT_BINARY_DATA_CONVERSION
