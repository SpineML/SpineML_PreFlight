/*
 * Implementation of ConnectionList class
 * Author: Seb James
 * Date: Nov 2014
 */

#include <vector>
#include <iostream>
#include <sstream>
#include <fstream>
#include <stdexcept>
#include "rng.h"
#include "rapidxml.hpp"
#include "rapidxml_print.hpp"
#include "connection_list.h"

using namespace std;
using namespace rapidxml;
using namespace spineml;

ConnectionList::ConnectionList ()
    : delayDistributionType(spineml::Dist_FixedValue)
    , delayFixedValue(0)
    , delayMean(0)
    , delayVariance(0)
    , delayRangeMin(0)
    , delayRangeMax(0)
    , delayDistributionSeed(123)
    , delayDimension("")
{
}

ConnectionList::ConnectionList (unsigned int srcNum, unsigned int dstNum)
    : delayDistributionType(spineml::Dist_FixedValue)
    , delayFixedValue(0)
    , delayMean(0)
    , delayVariance(0)
    , delayRangeMin(0)
    , delayRangeMax(0)
    , delayDistributionSeed(123)
    , delayDimension("")
{
    // run through connections, creating connectivity pattern:
    this->connectivityS2C.reserve (srcNum); // probably src_num.
    this->connectivityS2C.resize (srcNum);  // We have to resize connectivityS2C here.
    this->connectivityC2D.reserve (dstNum); // probably num from dst_population
}

void
ConnectionList::write (xml_node<>* into_node, const string& model_root,
                       const string& binary_file_name)
{
    this->writeXml (into_node, model_root, binary_file_name);
    this->writeBinary (into_node, model_root, binary_file_name);
}

void
ConnectionList::generateDelays (void)
{
    // First, how many connections do we have? It's assumed
    // that generateFixedProbability was called first to
    // generate the connectivity maps.

    switch (this->delayDistributionType) {
    case spineml::Dist_Normal:
        this->generateNormalDelays();
        break;
    case spineml::Dist_Uniform:
        this->generateUniformDelays();
        break;
    case spineml::Dist_FixedValue:
    default:
        this->connectivityC2Delay.assign (this->connectivityC2D.size(), this->delayFixedValue);
        break;
    }
}

void
ConnectionList::generateFixedProbability (const int& seed, const float& probability,
                                          const unsigned int& srcNum, const unsigned int& dstNum)
{
    this->connectivityS2C.reserve (srcNum); // probably src_num.
    this->connectivityS2C.resize (srcNum);  // We have to resize connectivityS2C here.
    this->connectivityC2D.reserve (dstNum); // probably num from dst_population

    RngData rngData;
    rngDataInit (&rngData);

    // seed the rng: 2 questions about origin code. Why the additional
    // 1 in front of the seed in 2nd arg to zigset, and why was
    // rngData.seed hardcoded to 123? I think it basically doesn't
    // matter, and the seed to zigset is different from the one to
    // rngData.seed.
    //
    // Here, I've reproduced the exact behaviour of
    // SpineML_2_BRAHMS_CL_weight.xsl around line 271
    int zigset_seed = 0;
    {
        stringstream seed_ss;
        seed_ss << "1" << seed; // "1" then the seed. So for seed=123, we pass 1123 to zigset_seed.
        seed_ss >> zigset_seed;
    }
    zigset (&rngData, zigset_seed);
    rngData.seed = 123; // Hardcoded, as in SpineML_2_BRAHMS_CL_weight.xsl around line 272

    // run through connections, creating connectivity pattern:
    this->connectivityC2D.reserve (dstNum); // probably num from dst_population
    this->connectivityS2C.reserve (srcNum); // probably src_num.
    this->connectivityS2C.resize (srcNum); // We have to resize connectivityS2C here.

    for (unsigned int i = 0; i < this->connectivityS2C.size(); ++i) {
        this->connectivityS2C[i].reserve((int) round(dstNum*probability));
    }
    for (unsigned int srcIndex = 0; srcIndex < srcNum; ++srcIndex) {
        for (unsigned int dstIndex = 0; dstIndex < dstNum; ++dstIndex) {
            if (UNI(&rngData) < probability) {
                this->connectivityC2D.push_back(dstIndex);
#ifdef DEBUG
                cout << "Pushing back connection " << (this->connectivityC2D.size()-1)
                     << " into connectivityS2C[" << srcIndex << "] (size " << this->connectivityS2C.size()
                     << ") and dstIndex " << dstIndex
                     << " into connectivityC2D." << endl;
#endif
                this->connectivityS2C[srcIndex].push_back(this->connectivityC2D.size()-1);
            }
        }
        if (float(this->connectivityC2D.size()) > 0.9*float(this->connectivityC2D.capacity())) {
            this->connectivityC2D.reserve(this->connectivityC2D.capacity()+dstNum);
        }
    }
}

void
ConnectionList::generateNormalDelays (void)
{
    RngData rngData;
    rngDataInit (&rngData);

    // The zigset seed is a different seed from the rngData.seed:
    zigset (&rngData, static_cast<unsigned int>(this->delayDistributionSeed+1));
    rngData.seed = static_cast<int>(this->delayDistributionSeed);

    float max_delay_val = 0;

    this->connectivityC2Delay.resize (this->connectivityC2D.size());

    for (unsigned int i = 0; i < this->connectivityC2Delay.size(); ++i) {

        // NB: delayVariance and delayMean HAVE to be in
        // milliseconds. this->delayVariance and this->delayMean are
        // assumed to have dimension ms.
        this->connectivityC2Delay[i] = (RNOR(&rngData) * this->delayVariance + this->delayMean);

        if (this->connectivityC2Delay[i] < 0) {
            this->connectivityC2Delay[i] = 0;
        }

        if (this->connectivityC2Delay[i] > max_delay_val) {
            max_delay_val = this->connectivityC2Delay[i];
        }
    }
}

void
ConnectionList::generateUniformDelays (void)
{
    RngData rngData;
    rngDataInit (&rngData);

    // The zigset seed is a different seed from the rngData.seed:
    zigset (&rngData, static_cast<unsigned int>(this->delayDistributionSeed+1));
    rngData.seed = static_cast<int>(this->delayDistributionSeed);

    this->connectivityC2Delay.resize (this->connectivityC2D.size());

    float max_delay_val = 0;

    for (unsigned int i = 0; i < this->connectivityC2Delay.size(); ++i) {

        this->connectivityC2Delay[i] = (_randomUniform(&rngData)
                                        * (this->delayRangeMax - this->delayRangeMin)
                                        + this->delayRangeMin);

        if (this->connectivityC2Delay[i] < 0) {
            this->connectivityC2Delay[i] = 0;
        }

        if (this->connectivityC2Delay[i] > max_delay_val) {
            max_delay_val = this->connectivityC2Delay[i];
        }
    }
}

/*!
 * Write out the connection list as an explicit binary file.
 */
void
ConnectionList::writeBinary (xml_node<>* into_node,
                             const string& model_root,
                             const string& binary_file_name)
{
    if (this->connectivityC2Delay.size() != this->connectivityC2D.size()) {
        stringstream ee;
        ee << __FUNCTION__ << " Error: Don't have the same number of delays ("
           << this->connectivityC2Delay.size() << ") as destinations ("
           << this->connectivityC2D.size() << ").";
        throw runtime_error (ee.str());
    }

    int s_idx = 0;
    vector<vector<int> >::const_iterator s_begin = this->connectivityS2C.begin();
    vector<vector<int> >::const_iterator s = s_begin;
    vector<int>::const_iterator c;
    vector<int>::const_iterator d = this->connectivityC2D.begin();
    vector<float>::const_iterator dly = this->connectivityC2Delay.begin();

    string path = model_root + binary_file_name;
    ofstream f;
    f.open (path.c_str(), ios::out|ios::trunc);
    if (!f.is_open()) {
        stringstream ee;
        ee << __FUNCTION__ << " Failed to open file '" << path << "' for writing.";
        throw runtime_error (ee.str());
    }
    cout << "Preflight: Opened connection binary file " << path << endl;

    if (this->connectivityC2D.empty()) {
        cout << "Preflight: WARNING: no connectivity between source and destination populations!\n";
    }

    // Iterate over the vectors of source connection
    while (s != this->connectivityS2C.end() && d != this->connectivityC2D.end()) {

        c = s->begin();

        while (c != s->end()) {
            // File output
            f.write (reinterpret_cast<const char*>(&(s_idx)), sizeof(int));
            f.write (reinterpret_cast<const char*>(&(*d)), sizeof(int));
            f.write (reinterpret_cast<const char*>(&(*dly)), sizeof(float));
            ++c;
            ++d;
            ++dly;
        }

        ++s;
        ++s_idx;
    }
    f.close();
}

void
ConnectionList::writeXml (xml_node<>* into_node,
                          const string& model_root,
                          const string& binary_file_name)
{
    xml_document<>* thedoc = into_node->document();

    // 1. Remove any attributes and child nodes from into_node.
    into_node->remove_all_attributes();
    into_node->remove_all_nodes();

    // 2. Change name of node from whatever
    // (e.g. FixedProbabilityConnection) to ConnectionList.
    into_node->name("ConnectionList");

    // 4. Add the BinaryFile node
    xml_node<>* binfile_node = thedoc->allocate_node (node_element, "BinaryFile");
    char* bfn_alloced = thedoc->allocate_string (binary_file_name.c_str());
    xml_attribute<>* file_name_attr = thedoc->allocate_attribute ("file_name", bfn_alloced);
    stringstream nc_ss;
    // Number of source to destination connections is same as size of the C2D map:
    nc_ss << this->connectivityC2D.size();
    char* nc_alloced = thedoc->allocate_string(nc_ss.str().c_str());
    xml_attribute<>* num_connections_attr = thedoc->allocate_attribute ("num_connections", nc_alloced);
    // We're always going to explicitly list the delay for each connection:
    xml_attribute<>* explicit_delay_attr = thedoc->allocate_attribute ("explicit_delay_flag", "1");

    binfile_node->append_attribute (file_name_attr);
    binfile_node->append_attribute (num_connections_attr);
    binfile_node->append_attribute (explicit_delay_attr);

    into_node->prepend_node (binfile_node);
}
