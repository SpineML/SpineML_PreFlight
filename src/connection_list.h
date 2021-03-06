/*!
 * A connection list object.
 *
 * Author: Seb James
 * Date: Oct 2014
 */

#ifndef _CONNECTION_LIST_H_
#define _CONNECTION_LIST_H_

#include <vector>
#include <string>
#include "rapidxml.hpp"

namespace spineml
{
    /*!
     * An enum to denote the type of a random distribution.
     */
    enum Distribution {
        Dist_FixedValue,
        Dist_Uniform,
        Dist_Normal,
        Dist_ExplicitList
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
        ConnectionList ();
        ConnectionList (unsigned int srcNum, unsigned int dstNum);
        ~ConnectionList() {}
        //@}

        /*!
         * Write out the connectivity lists into the passed in node
         * node of an XML document (which is probably the SpineML
         * project's model.xml). The @param into_node is emptied and
         * replaced. into_node might look like:
         *
         * \verbatim
         * <FixedProbabilityConnection probability="0.11" seed="123">
         *   <Delay Dimension="ms">
         *     <FixedValue value="0.2"/>
         *   </Delay>
         * </FixedProbabilityConnection>
         * \endverbatim
         *
         * It's called into_node because we're copying the
         * ConnectionList INTO that node.
         *
         * Here's an example of the output XML:
         *
         * \verbatim
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
         * \endverbatim
         *
         * The binary part of the connection list is written into
         * @param binary_file_name
         *
         * @param model_root The directory path for the model's XML files.
         *
         * NB: This only writes out the source to destination
         * connections (connectivityS2C/connectivityC2D).
         */
        void write (rapidxml::xml_node<>* into_node,
                    const std::string& model_root, const std::string& binary_file_name);

        /*!
         * Generate the explicit list of connection delays for a
         * uniform or normal distribution. If the
         * delayDistributionType is Dist_FixedValue, then this
         * function will do nothing, as a Delay XML node will be
         * written out along with the connection list in that case.
         */
        void generateDelays (void);

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
                                       const unsigned int& srcNum, const unsigned int& dstNum);

    private:

        /*!
         * Generate delays for existing connection lists, using a
         * normal distribution for the stochasticity.
         */
        void generateNormalDelays (void);

        /*!
         * Generate delays for existing connection lists, using a
         * uniform distribution for the stochasticity.
         */
        void generateUniformDelays (void);

        /*!
         * Write out the connection list as an explicit binary file.
         */
        void writeBinary (rapidxml::xml_node<>* into_node,
                          const std::string& model_root,
                          const std::string& binary_file_name);

        /*!
         * Re-writes the ConnectionList node's XML, in preparation for
         * writing out the connection list as an explicit binary file.
         */
        void writeXml (rapidxml::xml_node<>* into_node,
                       const std::string& model_root,
                       const std::string& binary_file_name);

    public:
        /*!
         * A list of "Source" to "Connection index" connection
         * lists. This covers a one to many connection scheme, where
         * one neuron on the source population connects to many
         * neurons in the destination population.
         *
         * This is a vector of vector<int> because that's the way it's
         * coded in the SpineML_2_BRAHMS code. For conversion of
         * explicit lists of delays, it could make more sense for this
         * to be map<vector<int> >
         */
        std::vector<std::vector<int> > connectivityS2C;

        /*!
         * A list of "Connection index" to "Destination"
         * connections. Forms a "pair of data structures" with
         * connectivityS2C.
         */
        std::vector<int> connectivityC2D;

        /*!
         * A list of the delays, indexed against "connection index",
         * like connectivityC2D. These delays are in ms.
         */
        std::vector<float> connectivityC2Delay;

        /*!
         * A list of "Destination" to "Connection index" connection
         * lists. This covers many to one connectivity from a set of
         * source neurons to a single destination neuron.
         */
        std::vector<std::vector<int> > connectivityD2C;

        /*!
         * The "Connection index" to "Source" connection list forming
         * a pair with connectivityD2C.
         */
        std::vector<int> connectivityC2S;

        /*!
         *
         * The delay can have a uniform distribution, fixed value or
         * normal distribution:
         *
         * \verbatim
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
         * \endverbatim
         *
         */
        Distribution delayDistributionType;

        /*!
         * The connection delay IF the delay is a fixed scalar. If
         * there is a connection-dependent delay, that goes in the
         * connection lists. I think. Dimension: ms.
         */
        float delayFixedValue;

        /*!
         * If the delay is a normal distribution, it has a mean and
         * variance. Dimension: ms.
         */
        //@{
        float delayMean;
        float delayVariance;
        //@}

        /*!
         * If the delay is a uniform distribution, it has a range.
         * Dimensions: ms.
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
        std::string delayDimension;
    };

} // namespace spineml

#endif // _CONNECTION_LIST_H_
