#include <string>
#include <sstream>
#include <fstream>
#include <stdexcept>
#include "rapidxml_print.hpp"
#include "rapidxml.hpp"
#include "modelpreflight.h"
#include "connection_list.h"

using namespace std;
using namespace rapidxml;
using namespace spineml;

ModelPreflight::ModelPreflight(const std::string& fdir, const std::string& fname)
    : root_node (static_cast<xml_node<>*>(0))
    , binfilenum (0)
{
    this->modeldir = fdir;
    this->modelfile = fname;
    string filepath = this->modeldir + this->modelfile;
    cout << "filepath: " << filepath;
    this->modeldata.read (filepath);
}

ModelPreflight::~ModelPreflight()
{
}

void
ModelPreflight::write (void)
{
    // Backup model.xml:
    string filepath = this->modeldir + this->modelfile;

    stringstream cmd;
    cmd << "cp " << filepath << " " << filepath << ".bu";
    system (cmd.str().c_str());

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
ModelPreflight::preflight (void)
{

    // we are choosing to parse the XML declaration
    // parse_no_data_nodes prevents RapidXML from using the somewhat
    // surprising behaviour of having both values and data nodes, and
    // having data nodes take precedence over values when printing
    // >>> note that this will skip parsing of CDATA nodes <<<
    this->doc.parse<parse_declaration_node | parse_no_data_nodes>(this->modeldata.data());

#ifdef CARE_ABOUT_ENCODING
    if (this->doc.first_node()->first_attribute("encoding")) {
        string encoding = this->doc.first_node()->first_attribute("encoding")->value();
    }
#endif

    // Get the root node.
    this->root_node = this->doc.first_node (LVL"SpineML");
    if (!this->root_node) {
        // Possibly look for HL:SpineML, if we have a high level model (not
        // used by anyone at present).
        stringstream ee;
        ee << "No root node " << LVL << "SpineML!";
        throw runtime_error (ee.str());
    }

    // Search each population for stuff.
    for (this->first_pop_node = this->root_node->first_node(LVL"Population");
         this->first_pop_node;
         this->first_pop_node = this->first_pop_node->next_sibling(LVL"Population")) {
        cout << "preprocess_population" << endl;
        this->preprocess_population (this->first_pop_node);
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
ModelPreflight::preprocess_population (xml_node<> *pop_node)
{
    // Within each population:
    // Find source name; this is given by LL:Neuron name attribute; also have size attr.
    // search out projections.
    xml_node<> *neuron_node = pop_node->first_node(LVL"Neuron");
    if (!neuron_node) {
        // No src name. Does that mean we return or carry on?
        return;
    }

    string src_name("");
    xml_attribute<>* name_attr;
    if ((name_attr = neuron_node->first_attribute ("name"))) {
        src_name = name_attr->value();
    } // else failed to get src name

    string src_num("");
    xml_attribute<>* num_attr;
    if ((num_attr = neuron_node->first_attribute ("size"))) {
        src_num = num_attr->value();
    } // else failed to get src num

    // Now find all Projections.
    for (xml_node<> *proj_node = pop_node->first_node(LVL"Projection");
         proj_node;
         proj_node = proj_node->next_sibling(LVL"Projection")) {

        preprocess_projection (proj_node, src_name, src_num);
    }
}

void
ModelPreflight::preprocess_projection (xml_node<> *proj_node,
                                       const string& src_name,
                                       const string& src_num)
{
    cout << __FUNCTION__ << " called" << endl;
    // Get the destination.
    string dst_population("");
    xml_attribute<>* dst_pop_attr;
    if ((dst_pop_attr = proj_node->first_attribute ("dst_population"))) {
        dst_population = dst_pop_attr->value();
    } // else failed to get src name

    // And then for each synapse in the projection:
    for (xml_node<> *syn_node = proj_node->first_node(LVL"Synapse");
         syn_node;
         syn_node = syn_node->next_sibling(LVL"Synapse")) {
        preprocess_synapse (syn_node, src_name, src_num, dst_population);
    }
}

void
ModelPreflight::preprocess_synapse (xml_node<> *syn_node,
                                    const string& src_name,
                                    const string& src_num,
                                    const string& dst_population)
{
    cout << __FUNCTION__ << " called" << endl;
    // For each synapse... Is there a FixedProbability?
    xml_node<>* fixedprob_connection = syn_node->first_node("FixedProbabilityConnection");
    if (!fixedprob_connection) {
        return;
    }
    replace_fixedprob_connection (fixedprob_connection, src_name, src_num, dst_population);
    // Plus any other modifications which need to be made...
}

void
ModelPreflight::replace_fixedprob_connection (xml_node<> *fixedprob_node,
                                              const string& src_name,
                                              const string& src_num,
                                              const string& dst_population)
{
    cout << __FUNCTION__ << " called" << endl;

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
            throw runtime_error ("Failed to get FixedProbability's probability attr from model.xml");
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

    // The connection list object which we'll populate.
    spineml::ConnectionList cl;
    {
        float dimMultiplier = 1.0;
        xml_node<>* delay_node = fixedprob_node->first_node ("Delay");
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
            // Do we have a FixedValue distribution?
            xml_node<>* delay_value_node = delay_node->first_node ("FixedValue");
            xml_node<>* delay_normal_node = delay_node->first_node ("NormalDistribution");
            xml_node<>* delay_uniform_node = delay_node->first_node ("UniformDistribution");
            if (delay_value_node) {
                cl.delayDistributionType = spineml::FixedValue;
                xml_attribute<>* value_attr = delay_value_node->first_attribute ("value");
                if (value_attr) {
                    stringstream ss;
                    ss << value_attr->value();
                    ss >> cl.delayFixedValue;
                    cl.delayFixedValue *= dimMultiplier;
                }
            } else if (delay_normal_node) {
                cl.delayDistributionType = spineml::Normal;
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
                cl.delayDistributionType = spineml::Uniform;
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

    unsigned int srcNum = 0;
    {
        stringstream ss;
        ss << src_num;
        ss >> srcNum;
    }

    cout << "probability: " << probabilityValue << ", seed: " << seed
         << ", srcNum: " << srcNum << endl;

    // Find the number of neurons in the destination population
    int dstNum_ = find_num_neurons (dst_population);
    unsigned int dstNum(0);
    if (dstNum_ != -1) {
        dstNum = static_cast<unsigned int>(dstNum_);
    } else {
        throw runtime_error ("Failed ot find the number of neurons in the destination population.");
    }
    cout << "dstNum: " << dstNum << endl;

    cl.generateFixedProbability (seed, probabilityValue, srcNum, dstNum);
#ifdef NEED_SAMPLE_TIMESTEP
    cl.setSampleDt (exptSampleDt);
#endif
    cl.generateDelays();
    string binfilepath ("pp_connection");
    stringstream numss;
    numss << this->binfilenum++;
    binfilepath += numss.str();
    binfilepath += ".bin";
    cl.write (fixedprob_node, this->modeldir, binfilepath);
}
