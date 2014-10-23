/*
 * This program parses the SpineML model, updating any aspects of the
 * model where parameters, weights or connectivity are specified in
 * meta-form. For example, where connections are given in kernel form,
 * this program creates a connection list file and modifies the
 * <KernelConnection> xml element into a <ConnectionList> element with
 * an associated binary connection list file.
 *
 * The original model.xml file is renamed model_orig.xml and a new
 * model.xml file is written out containing the new, specific
 * information.
 *
 * The dependency-free rapidxml header-only xml parser is used to
 * read, modify and write out model.xml.
 *
 * Author: Seb James, 2014.
 */

#include "rapidxml_print.hpp"
#include "rapidxml.hpp"
#include <fstream>
#include <string>
#include <sstream>
#include <iostream>
#include <vector>
#include <stdexcept>
#include <unistd.h>
#include <string.h>
#include "connection_list.h"
#include "experiment.h"
#include "allocandread.h"

using namespace std;
using namespace rapidxml;

/*!
 * It may be that we need to run this for HL and LL models.
 */
#define LVL "LL:"

/*!
 * A global for the first population node pointer. I anticipate this
 * going into a class at some point, hence just sticking it in as a
 * global for now.
 */
xml_node<>* first_pop_node;

/*!
 * A global for the root node pointer. Same comments apply as for
 * first_pop_node.
 */
xml_node<>* root_node;

/*!
 * A lazy global (for now) for the sample timestep, as read in from
 * the experiment.
 */
double exptSampleDt;

#ifdef REQUIRED
/*!
 * A cartesian location structure. This is a copy of the same struct
 * found in SpineCreator/globalHeader.h.
 */
struct loc {
    float x;
    float y;
    float z;
};

/*!
 * A connection structure, to hold information about a connection from
 * one neuron body to another. This is a copy of the same struct found
 * in SpineCreator/globalHeader.h.
 */
struct conn {
    int src;
    int dst;
    float metric;
};
#endif

/*!
 * Read out model.xml file into a char*, allocating enough memory for
 * the task. Return the number of bytes allocated, or -1 on error.
 */
int alloc_and_read_xml_text (char* text);

/*!
 * Find the number of neurons in the destination population, starting
 * from the root node or the first population node (globals/members).
 */
int find_num_neurons (const string& dst_population);

/*!
 * Take a population node, and process this for any changes we need to
 * make. Sub-calls preprocess_projection.
 */
void preprocess_population (xml_node<>* pop_node);

/*!
 * Process the passed-in projection, making any changes necessary.
 */
void preprocess_projection (xml_node<>* proj_node,
                            const string& src_name, const string& src_num);

/*!
 * Process the synapse. Search for KernelConnection and modify if found.
 */
void preprocess_synapse (xml_node<>* proj_node,
                         const string& src_name, const string& src_num,
                         const string& dst_population);

/*!
 * Do the work of replacing a FixedProbability connection with a
 * ConnectionList
 *
 * Go from this:
 *          <LL:Synapse>
 *               <FixedProbabilityConnection probability="0.11" seed="123">
 *                   <Delay Dimension="ms">
 *                       <FixedValue value="0.2"/>
 *                   </Delay>
 *               </FixedProbabilityConnection>
 *
 * to this:
 *          <LL:Synapse>
 *               <ConnectionList>
 *                   <BinaryFile file_name="connection0.bin" num_connections="87360"
 *                               explicit_delay_flag="0" packed_data="true"/>
 *                   <Delay Dimension="ms">
 *                       <FixedValue value="1"/>
 *                   </Delay>
 *               </ConnectionList>
 *
 * (an explicit list of connections, with distribution generated like the
 * code in SpineML_2_BRAHMS_CL_weight.xsl)
 */
void replace_fixedprob_connection (xml_node<>* syn_node,
                                   const string& src_name,
                                   const string& src_num,
                                   const string& dst_population);

/*!
 * Global function implementations
 */
//@{
int alloc_and_read_xml_text (char* text)
{
    char* textpos = text;
    string line("");

    ifstream model;
    // Fixme: This'll take a passed-in path to the model OR a passed
    // in value for the experiment and from that get the model? OR
    // pass in both?
    model.open ("./model/model.xml", ios::in);
    if (!model.is_open()) {
        cerr << "model.xml could not be opened." << endl;
        return -1;
    }

    size_t curmem = 0; // current allocated memory count (chars)
    size_t llen = 0;   // line length (chars)
    size_t curpos = 0;
    while (getline (model, line)) {
        // Restore the newline
        line += "\n";
        // Allocate enough memory in char* text for this line
        llen = line.size();
        curmem += llen;

        // This attempt to dynamically realloc mucks things
        // up. probably need to pass a char** if I want to do this.
        //
        // cout << "realloc " << curmem << " bytes for text." << endl;
        // text = (char*) realloc (text, curmem);

        // Restore textpos pointer in the reallocated memory:
        textpos = text + curpos;
        // copy line to textpos:
        strncpy (textpos, line.c_str(), llen);
        // Update current character position
        curpos += llen;
    }
    model.close();

    // Note: text is already null terminated
    return curmem;
}

// For debug, I added a name to the Populations in my test model.xml
#define POP_NAME_TESTING 1

int find_num_neurons (const string& dst_population)
{
    int numNeurons = -1;
    xml_node<>* pop_node = first_pop_node;
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

void preprocess_population (xml_node<> *pop_node)
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

void preprocess_projection (xml_node<> *proj_node,
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

void preprocess_synapse (xml_node<> *syn_node,
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

void replace_fixedprob_connection (xml_node<> *fixedprob_node,
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
    s2b::ConnectionList cl;
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
                cl.delayDistributionType = s2b::FixedValue;
                xml_attribute<>* value_attr = delay_value_node->first_attribute ("value");
                if (value_attr) {
                    stringstream ss;
                    ss << value_attr->value();
                    ss >> cl.delayFixedValue;
                    cl.delayFixedValue *= dimMultiplier;
                }
            } else if (delay_normal_node) {
                cl.delayDistributionType = s2b::Normal;
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
                cl.delayDistributionType = s2b::Uniform;
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
    cl.write (fixedprob_node, "./model/", "pp_connectionN.bin");
}
//@} End global function implementations

int main()
{
    s2b::Experiment expt ("./model/experiment.xml");
    cout << "Sim timestep: " << expt.getSimFixedRate() << " s\n";
    exptSampleDt = expt.getSimFixedDt(); // seconds

    // Alternative scheme - an alloc and read class?
    s2b::AllocAndRead ar("./model/model.xml");
    xml_document<> doc;    // character type defaults to char

    // we are choosing to parse the XML declaration
    // parse_no_data_nodes prevents RapidXML from using the somewhat
    // surprising behaviour of having both values and data nodes, and
    // having data nodes take precedence over values when printing
    // >>> note that this will skip parsing of CDATA nodes <<<
    doc.parse<parse_declaration_node | parse_no_data_nodes>(ar.data());

#ifdef CARE_ABOUT_ENCODING
    if (doc.first_node()->first_attribute("encoding")) {
        string encoding = doc.first_node()->first_attribute("encoding")->value();
    }
#endif

    // Get the root node.
    root_node = doc.first_node (LVL"SpineML");
    if (!root_node) {
        // Possibly look for HL:SpineML, if we have a high level model (not
        // used by anyone at present).
        cout << "No root node " << LVL << "SpineML!" << endl;
        return -1;
    }

    // Search each population for stuff.
    for (first_pop_node = root_node->first_node(LVL"Population");
         first_pop_node;
         first_pop_node = first_pop_node->next_sibling(LVL"Population")) {
        cout << "preprocess_population" << endl;
        preprocess_population (first_pop_node);
    }

    // Backup model.xml
    system ("cp ./model/model.xml ./model/model.bu.xml");

    // Write out the now modified xml:
    ofstream f;
    f.open ("./model/model.xml", ios::out|ios::trunc);
    if (f.is_open()) {
        f << doc;
        f.close();
    }

    return 0;
}
