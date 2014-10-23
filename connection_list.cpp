#include "connection_list.h"

#include <vector>
#include <iostream>
#include <sstream>
#include <fstream>
#include <stdexcept>
#include "rng.h"
#include "rapidxml.hpp"
#include "rapidxml_print.hpp"

using namespace std;
using namespace rapidxml;
using namespace s2b;

ConnectionList::ConnectionList ()
    : delayDistributionType(s2b::FixedValue)
    , delayFixedValue(0)
    , delayMean(0)
    , delayVariance(0)
    , delayRangeMin(0)
    , delayRangeMax(0)
    , delayDistributionSeed(123)
    , delayDimension("")
    , sampleDt(-1.0)
    , sampleDt_ms(-1.0)
{
}

ConnectionList::ConnectionList (unsigned int srcNum, unsigned int dstNum)
    : delayDistributionType(s2b::FixedValue)
    , delayFixedValue(0)
    , delayMean(0)
    , delayVariance(0)
    , delayRangeMin(0)
    , delayRangeMax(0)
    , delayDistributionSeed(123)
    , delayDimension("")
    , sampleDt(-1.0)
    , sampleDt_ms(-1.0)
{
    // run through connections, creating connectivity pattern:
    this->connectivityS2C.reserve (srcNum); // probably src_num.
    this->connectivityS2C.resize (srcNum);  // We have to resize connectivityS2C here.
    this->connectivityC2D.reserve (dstNum); // probably num from dst_population
}

void
ConnectionList::write (xml_node<> *into_node, const string& model_root, const string& binary_file_name)
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
    case s2b::Normal:
        this->generateNormalDelays();
        break;
    case s2b::Uniform:
        this->generateUniformDelays();
        break;
    case s2b::FixedValue:
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
    zigset (&rngData, 1+seed);
    rngData.seed = seed;

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

    // set up the number of connections
    int numConn = this->connectivityC2D.size();

    // Ok, having made up the connectivity maps as above, write them
    // out into a connection binary file. Name these
    // pp_connectionN.bin as opposed to just connection.bin to
    // distinguish them.
    cout << "numConn is " << numConn << endl;
}

void
ConnectionList::setSampleDt (const float& dt_seconds)
{
    this->sampleDt = dt_seconds;
    this->sampleDt_ms = dt_seconds * 1000.0;
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

    if (this->sampleDt < 0) {
        // Error - sampleDt unset!
        throw runtime_error ("ConnectionList::generateNormalDelays: sample timestep must be set.");
    }

    cout << "sampleDt: " << this->sampleDt << endl; // in seconds.

    // This was: float most_delay_accuracy = (1000.0f * time->sampleRate.den / time->sampleRate.num);
    // sampleRate is struct SampleRate sampleRate; containing: struct SampleRate { UINT64 num; UINT64 den; };
    float timestep_in_delaydim = this->sampleDt_ms;
    if (this->delayDimension == "ms") {
        // already set it in ms, above.
    } else if (this->delayDimension == "s") {
        // Delays are in seconds; Nothing to do to sampleDt.
        timestep_in_delaydim = this->sampleDt;
    } else {
        // Delays are in unknown units
        throw runtime_error ("Don't know the dimensions of the delay (usually ms or s).");
    }

    this->connectivityC2Delay.resize (this->connectivityC2D.size());

    cout << "Delays: Normal distribution with mean " << this->delayMean
         << " and variance " << this->delayVariance << endl;

    for (unsigned int i = 0; i < this->connectivityC2Delay.size(); ++i) {

        this->connectivityC2Delay[i] = round(
            (RNOR(&rngData) * this->delayVariance + this->delayMean) / timestep_in_delaydim
            );
#ifdef DEBUG
        cout << i << "," << this->connectivityC2Delay[i] << endl;
#endif

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
    throw runtime_error ("Uniform delays are unimplemented");
/*
    this->rngData_BRAHMS.seed = delayForConnTemp[3];
    for (UINT32 i_BRAHMS = 0; i_BRAHMS < delayForConn.size(); ++i_BRAHMS) {
        delayForConn[i_BRAHMS] = round((_randomUniform(&this->rngData_BRAHMS)*(delayForConnTemp[2]-delayForConnTemp[1])+delayForConnTemp[1])/most_delay_accuracy);
        //bout <<delayForConn[i_BRAHMS] << D_INFO;
        if (delayForConn[i_BRAHMS] > max_delay_val) max_delay_val = delayForConn[i_BRAHMS];
    }
*/
}

/*!
 * Write out the connection list as an explicit binary file.
 */
void
ConnectionList::writeBinary (xml_node<> *into_node,
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

    string path = model_root + "/" + binary_file_name;
    ofstream f;
    f.open (path.c_str(), ios::out|ios::trunc);
    if (!f.is_open()) {
        stringstream ee;
        ee << __FUNCTION__ << " Failed to open file '" << path << "' for writing.";
        throw runtime_error (ee.str());
    }
    cerr << "Opened file " << path << endl;

    // Iterate over the vectors of source connection
    while (s != this->connectivityS2C.end() && d != this->connectivityC2D.end()) {

        c = s->begin();

        while (c != s->end()) {
            // Debug output
            cout << "S:" << static_cast<int>(s-s_begin) << ",D:"<< *(d) << endl;

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
ConnectionList::writeXml (xml_node<> *into_node,
                          const string& model_root,
                          const string& binary_file_name)
{
    // 1. Remove any attributes and child nodes from into_node.
    into_node->remove_all_attributes();
    into_node->remove_all_nodes();

    // 2. Change name of node from whatever
    // (e.g. FixedProbabilityConnection) to ConnectionList.
    into_node->name("ConnectionList");

    // 3. Add the Delay node.
    xml_document<>* thedoc = into_node->document();

    // Which type of Delay distribution do we have? Ah, hold on. for uniform/normal, build this into explicit list.
    xml_node<>* distribution_node;
    if (this->delayDistributionType == s2b::FixedValue) {
        distribution_node = thedoc->allocate_node (node_element, "FixedValue");
        stringstream delay_ss;
        delay_ss << this->delayFixedValue;
        // Note that we have to allocate memory in the document pool prior to allocating attributes:
        char* delay_alloced = thedoc->allocate_string(delay_ss.str().c_str());
        xml_attribute<>* v_attr = thedoc->allocate_attribute ("value", delay_alloced);
        distribution_node->append_attribute (v_attr);

    } else if (this->delayDistributionType == s2b::Normal) {
        distribution_node = thedoc->allocate_node (node_element, "NormalDistribution");
        stringstream seed_ss;
        seed_ss << this->delayDistributionSeed;
        char* seed_alloced = thedoc->allocate_string(seed_ss.str().c_str());
        xml_attribute<>* s_attr = thedoc->allocate_attribute ("seed", seed_alloced);
        distribution_node->append_attribute (s_attr);
        stringstream variance_ss;
        variance_ss << this->delayVariance;
        char* variance_alloced = thedoc->allocate_string(variance_ss.str().c_str());
        xml_attribute<>* v_attr = thedoc->allocate_attribute ("variance", variance_alloced);
        distribution_node->prepend_attribute (v_attr);
        stringstream mean_ss;
        mean_ss << this->delayMean;
        char* mean_alloced = thedoc->allocate_string(mean_ss.str().c_str());
        xml_attribute<>* m_attr = thedoc->allocate_attribute ("mean", mean_alloced);
        distribution_node->prepend_attribute (m_attr);

    } else if (this->delayDistributionType == s2b::Uniform) {
        distribution_node = thedoc->allocate_node (node_element, "UniformDistribution");
        stringstream seed_ss;
        seed_ss << this->delayDistributionSeed;
        char* seed_alloced = thedoc->allocate_string(seed_ss.str().c_str());
        xml_attribute<>* s_attr = thedoc->allocate_attribute ("seed", seed_alloced);
        distribution_node->append_attribute (s_attr);
        stringstream rangemax_ss;
        rangemax_ss << this->delayRangeMax;
        char* rangemax_alloced = thedoc->allocate_string(rangemax_ss.str().c_str());
        xml_attribute<>* rmax_attr = thedoc->allocate_attribute ("maximum", rangemax_alloced);
        distribution_node->prepend_attribute (rmax_attr);
        stringstream rangemin_ss;
        rangemin_ss << this->delayRangeMin;
        char* rangemin_alloced = thedoc->allocate_string(rangemin_ss.str().c_str());
        xml_attribute<>* rmin_attr = thedoc->allocate_attribute ("minimum", rangemin_alloced);
        distribution_node->prepend_attribute (rmin_attr);

    } else {
        // default to fixed val
        distribution_node = thedoc->allocate_node (node_element, "FixedValue");
        xml_attribute<>* v_attr = thedoc->allocate_attribute ("value", "0");
        distribution_node->append_attribute (v_attr);
    }

    // Now we've build the content of the Delay node, we can create Delay:
    xml_node<>* delay_node = thedoc->allocate_node (node_element, "Delay", "");
    cout << "delayDimension: " << this->delayDimension << endl;
    char* delaydim_alloced = thedoc->allocate_string(this->delayDimension.c_str());
    xml_attribute<>* d_attr = thedoc->allocate_attribute ("Dimension", delaydim_alloced);
    delay_node->append_attribute (d_attr);
    delay_node->append_node (distribution_node);

    into_node->append_node (delay_node);

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
    xml_attribute<>* explicit_delay_attr = thedoc->allocate_attribute ("explicit_delay", "1");

    binfile_node->append_attribute (file_name_attr);
    binfile_node->append_attribute (num_connections_attr);
    binfile_node->append_attribute (explicit_delay_attr);

    into_node->prepend_node (binfile_node);
}
