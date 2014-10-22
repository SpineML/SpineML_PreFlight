/*!
 * A connection list object.
 *
 * Author: Seb James
 * Date: Oct 2014
 */

#include <vector>
#include <iostream>
#include "rng.h"
#include "rapidxml.hpp"
#include "rapidxml_print.hpp"

using namespace std;
using namespace rapidxml;

namespace s2b
{
    /*!
     * A random distribution type.
     */
    enum Distribution {
        FixedValue,
        Uniform,
        Normal
    };

    /*!
     * This is a connection list class. It holds the information about
     * a set of source neuron indexes and a set of neuron destination
     * indexes; these are the indexes into the source and destination
     * populations which are connected together by this explicit
     * connection list.
     *
     * Here's an email from Alex C defining what
     * connectivityS2C/connectivityC2D are. These were originally
     * found in code which was used to generate connectivity maps
     * from, for example, fixed probability connectivity definitions.
     *
     * C stands for Connection - i.e. S2C is the lookup from the
     * source index to the index of the connection (or more correctly
     * each index in connectivityS2C refers to a source neuron index,
     * and contains a vector of all the connectivity indices that that
     * link to the source neuron). So for this simple example:
     *
     * connectivity list:
     * 0 0  0
     * 0 1  1
     * 0 2  2
     * 1 1  3
     * 2 0  4
     *
     * Here the first column is the source index, the second is the
     * destination index, and the third is the connection index, so
     * the vector < vector >s would be:
     *
     * connectivityS2C:
     * 0 1 2
     * 3
     * 4
     *
     * Where each line is an entry in the first vector, and the
     * indices on it are the data in the second vector.
     *
     * Likewise:
     *
     * connectivityC2D:
     * 0
     * 1
     * 2
     * 1
     * 0
     *
     * This is a flat list as each connection only connects to one dst
     * index.
     *
     * I also have the reverse lookups as these are needed if any
     * learning needs to connect in, or if the WeightUpdate needs to
     * connect back to the source population, although there is
     * currently no way of hooking into these.
     *
     * (This refers to connectivity D2C and connectivityC2S.)
     */
    class ConnectionList
    {
    public:
        /*!
         * Constructors and destructor.
         */
        //@{
        ConnectionList () {}
        ConnectionList (unsigned int srcNum, unsigned int dstNum)
        {
            // run through connections, creating connectivity pattern:
            this->connectivityS2C.reserve (srcNum); // probably src_num.
            this->connectivityS2C.resize (srcNum);  // We have to resize connectivityS2C here.
            this->connectivityC2D.reserve (dstNum); // probably num from dst_population
        }
        ~ConnectionList() {}
        //@}

        /*!
         * Write out the connectivity lists into the passed in node
         * node of an XML document (which is probably the SpineML
         * project's model.xml). The @param into_node is emptied and
         * replaced. into_node might look like:
         *
         * <FixedProbabilityConnection probability="0.11" seed="123">
         *   <Delay Dimension="ms">
         *     <FixedValue value="0.2"/>
         *   </Delay>
         * </FixedProbabilityConnection>
         *
         * It's called into_node because we're copying the
         * ConnectionList INTO that node.
         *
         * Here's an example of the output XML:
         *
         * <LL:Projection dst_population="Retina_1">
         *   <LL:Synapse>
         *     <ConnectionList>
         *       <BinaryFile file_name="connection0.bin" num_connections="87360"
         *                   explicit_delay_flag="0" packed_data="true"/>
         *         <Delay Dimension="ms">
         *           <FixedValue value="1"/>
         *         </Delay>
         *     </ConnectionList>
         *   <LL:WeightUpdate/>
         *     <LL:PostSynapse/>
         *   </LL:Synapse>
         * </LL:Projection>
         *
         * The binary part of the connection list is written into
         * @param binary_path.
         *
         * NB: This only writes out the source to destination
         * connections (connectivityS2C/connectivityC2D).
         */
        void write (xml_node<> *into_node, const string& model_root, const string& binary_file_name)
        {
            this->writeXml (into_node, model_root, binary_file_name);
            this->writeBinary (into_node, model_root, binary_file_name);
        }

        /*!
         * Write out the connection list as an explicit binary file.
         */
        void writeBinary (xml_node<> *into_node, const string& model_root, const string& binary_file_name)
        {
            if (this->connectivityC2Delay.size() != this->connectivityC2D.size()) {
                stringstream ee;
                ee << __FUNCTION__ << " Error: Don't have the same number of delays as destinations.";
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

        /*!
         * Re-writes the ConnectionList node's XML, in preparation for
         * writing out the connection list as an explicit binary file.
         */
        void writeXml (xml_node<> *into_node, const string& model_root, const string& binary_file_name)
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

        /*!
         * Generate the explicit list of connection delays. This may
         * be a fixed value, or a uniform or normal distribution.
         */
        void generateDelays ()
        {
            // First, how many connections do we have? It's assumed
            // that generateFixedProbability was called first to
            // generate the connectivity maps.

            switch (this->delayDistributionType) {
            case s2b::Normal:
                cerr << "Uh oh - unimplemented" << endl;
                break;
            case s2b::Uniform:
                cerr << "Uh oh - unimplemented" << endl;
                break;
            case s2b::FixedValue:
            default:
                this->connectivityC2Delay.assign (this->connectivityC2D.size(), this->delayFixedValue);
                break;
            }
        }

        /*!
         * Generate a fixed probability connection mapping using the
         * passed in probability and RNG seed for the passed in number
         * of source neurons and the passed in number of destination
         * neurons.
         *
         * Note this accepts seed, probability in the arg list,
         * whereas generateDelays works on member attributes such as
         * delayMean, delayVariance, etc.
         */
        void generateFixedProbability (const int& seed, const float& probability,
                                       const unsigned int& srcNum, const unsigned int& dstNum)
        {
            this->connectivityS2C.reserve (srcNum); // probably src_num.
            this->connectivityS2C.resize (srcNum);  // We have to resize connectivityS2C here.
            this->connectivityC2D.reserve (dstNum); // probably num from dst_population

            RngData rngData;

            // seed the rng: 2 questions about origin code. Why the additional
            // 1 in front of the seed in 2nd arg to zigset, and why was
            // rngData.seed hardcoded to 123?
            zigset (&rngData, /*1*/seed);
            rngData.seed = seed; // or 123??

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
                        cout << "Pushing back connection " << (this->connectivityC2D.size()-1)
                             << " into connectivityS2C[" << srcIndex << "] (size " << this->connectivityS2C.size()
                             << ") and dstIndex " << dstIndex
                             << " into connectivityC2D." << endl;
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

    public:
        /*!
         * A list of "Source" to "Connection index" connection
         * lists. This covers a one to many connection scheme, where
         * one neuron on the source population connects to many
         * neurons in the destination population.
         */
        vector<vector<int> > connectivityS2C;

        /*!
         * A list of "Connection index" to "Destination"
         * connections. Forms a "pair of data structures" with
         * connectivityS2C.
         */
        vector<int> connectivityC2D;

        /*!
         * A list of the delays, indexed against "connection index",
         * like connectivityC2D.
         */
        vector<float> connectivityC2Delay;

        /*!
         * A list of "Destination" to "Connection index" connection
         * lists. This covers many to one connectivity from a set of
         * source neurons to a single destination neuron.
         */
        vector<vector<int> > connectivityD2C;

        /*!
         * The "Connection index" to "Source" connection list forming
         * a pair with connectivityD2C.
         */
        vector<int> connectivityC2S;

        /*!
         *
         * The delay can have a uniform distribution, fixed value or
         * normal distribution:
         *
         * <Delay Dimension="ms">
         *   <UniformDistribution minimum="0.1" maximum="1.1" seed="123"/>
         * </Delay>
         *
         * <Delay Dimension="ms">
         *   <FixedValue value="0.1"/>
         * </Delay>
         *
         * <Delay Dimension="ms">
         *   <NormalDistribution mean="0.2" variance="1" seed="123"/>
         * </Delay>
         *
         */
        Distribution delayDistributionType;

        /*!
         * The connection delay IF the delay is a fixed scalar. If
         * there is a connection-dependent delay, that goes in the
         * connection lists. I think.
         */
        float delayFixedValue;

        /*!
         * If the delay is a normal distribution, it has a mean and
         * variance.
         */
        //@{
        float delayMean;
        float delayVariance;
        //@}

        /*!
         * If the delay is a uniform distribution, it has a range.
         */
        //@{
        float delayRangeMin;
        float delayRangeMax;
        //@}

        /*!
         * Normal and uniform delay distributions require a seed.
         */
        float delayDistributionSeed;

        /*!
         * Dimensions string for the delay. E.g. "ms".
         */
        string delayDimension;
    };

} // namespace s2b
