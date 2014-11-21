/*!
 * A model.xml parsing/modifying class.
 *
 * Author: Seb James
 * Date: Oct 2014
 */

#ifndef _MODELPREFLIGHT_H_
#define _MODELPREFLIGHT_H_

#include <string>
#include <map>
#include "rapidxml.hpp"
#include "allocandread.h"
#include "component.h"
#include "connection_list.h"

/*!
 * It may be that we need to run this for HL and LL models, in which
 * case this would become a runtime-determined string.
 */
#define LVL "LL:"

/*!
 * This encloses some code which should only be a temporary feature of
 * SpineML_PreFlight.
 */
#define EXPLICIT_BINARY_DATA_CONVERSION 1

namespace spineml
{
    /*!
     * A class to read, then find and replace relevant sections of
     * model.xml.
     *
     * EXPLICIT_BINARY_DATA_CONVERSION:
     * This class also contains code to swap formats between explicitData
     * formats int,float and int,double. This code allows me to build the
     * program e2b_float2double.
     */
    class ModelPreflight
    {
    public:
        /*!
         * Construct a ModelPreflight object with the given directory
         * and filename.
         *
         * @param fdir The directory containing model.xml
         *
         * @param fname The filename of the model xml file (probably model.xml)
         */
        ModelPreflight(const std::string& fdir, const std::string& fname);

        /*!
         * Some initialisation - parse the doc and find the root node.
         */
        void init (void);

        /*!
         * Start the work. Begin searching for Population nodes and
         * then search within each Population for anything we need to
         * modify in this preflight process.
         */
        void preflight (void);

        /*!
         * Find a property called @param propertyName in, for example,
         * a Neuron population named @param containerName Return
         * pointer to the node if found, null pointer if not. The
         * container may be a neuron population or a projection - any
         * object in the model which can contain a property.
         *
         * This is a recursive function.
         *
         * @param current_node The current node being searched.
         *
         * @param parentName The name attribute of the parent node of
         * current_node.
         *
         * @param containerName The name attribute of the container of
         * the property we're searching for.
         *
         * @param propertyName The name attribute of the property
         * we're searching for.
         */
        rapidxml::xml_node<>* findProperty (rapidxml::xml_node<>* current_node,
                                            const std::string& parentName,
                                            const std::string& containerName,
                                            const std::string& propertyName);

        /*!
         * Backup the existing model.xml file, then overwrite it with
         * the current content of @see doc
         */
        void write (void);

#ifdef EXPLICIT_BINARY_DATA_CONVERSION
    public:
        /*!
         * A utility - convert all explicit data binary files
         * referenced by the model from int,float format to int,double
         * format.  The int,float format came in with commit eac72eb
         * in SpineCreator.
         *
         * SpineCreator commit 19a3a42 converted this to int,double,
         * to match up with BRAHMS.
         */
        void binaryDataFloatToDouble (bool forwards = true);

        /*!
         * The inverse of binaryDataFloatToDouble.
         */
        void binaryDataDoubleToFloat (void);

    private:
        /*!
         * Set to true if we're converting int,float to int,double;
         * false if we're converting int,double to int,float.
         */
        bool binaryDataF2D;

        rapidxml::xml_node<>* findExplicitData (rapidxml::xml_node<>* current_node,
                                                const unsigned int& run);

        void binaryDataVerify (rapidxml::xml_node<>* binaryfile_node);

        void binaryDataModify (rapidxml::xml_node<>* binaryfile_node);
#endif // EXPLICIT_BINARY_DATA_CONVERSION

    private:
        /*!
         * Find the number of neurons in the destination population, starting
         * from the root node or the first population node (globals/members).
         *
         * @param dst_population The name attribute of the destination
         * population.
         */
        int find_num_neurons (const std::string& dst_population);

        /*!
         * Take a population node, and process this for any changes we need to
         * make. Sub-calls preflight_projection.
         *
         * @param pop_node The node to preflight.
         */
        void preflight_population (rapidxml::xml_node<>* pop_node);

        /*!
         * Process the passed-in projection, making any changes necessary.
         *
         * @param proj_node The projection to preflight.
         *
         * @param src_name The source population.
         *
         * @param src_num The number of members in the source population.
         */
        void preflight_projection (rapidxml::xml_node<>* proj_node,
                                   const std::string& src_name, const std::string& src_num);

        /*!
         * Process the synapse. Search for FixedProbabilityConnection
         * and modify if found. Also search for ConnectionList
         * containing explicit <Connection/> elements and replace
         * those with a binary explicit list.
         *
         * Not sure whether to process other synapses - in particular
         * ANY synapse connectivity type may contain a random
         * distribution Delay element. For now, I'll NOT process these.
         *
         * Here are the connection types encountered:
         *
         * \verbatim
         * OneToOne - leave unmodified
         *  <LL:Synapse>
         *      <OneToOneConnection>
         *          <Delay Dimension="ms">
         *              <FixedValue value="3"/>
         *          </Delay>
         *      </OneToOneConnection>
         * \endverbatim
         *
         * AllToAll - leave unmodified
         *
         * \verbatim
         *  <LL:Synapse>
         *      <AllToAllConnection>
         *          <Delay Dimension="ms">
         *              <FixedValue value="4"/>
         *          </Delay>
         *      </AllToAllConnection>
         * \endverbatim
         *
         * FixedProbability - replace connectivity and delay with expl. list.
         *
         * \verbatim
         *   <LL:Synapse>
         *      <FixedProbabilityConnection probability="0.01" seed="123">
         *          <Delay Dimension="ms">
         *              <NormalDistribution mean="0" variance="1" seed="123"/>
         *          </Delay>
         *      </FixedProbabilityConnection>
         * \endverbatim
         *
         * Explicit list in XML - replace with explicit binary list:
         *
         * \verbatim
         *  <LL:Synapse>
         *      <ConnectionList>
         *          <Connection src_neuron="1" dst_neuron="2" delay="4"/>
         *          <Connection src_neuron="1" dst_neuron="3" delay="5"/>
         *      </ConnectionList>
         * \endverbatim
         *
         * Finally KernelConnection is deprecated and will be removed
         * in a future version of SpineCreator.
         *
         * @param syn_node The synapse to preflight.
         *
         * @param src_name The source population.
         *
         * @param src_num The number of members in the source population.
         *
         * @param dst_population The destination population for the synapse.
         */
        void preflight_synapse (rapidxml::xml_node<>* syn_node,
                                const std::string& src_name, const std::string& src_num,
                                const std::string& dst_population);

        /*!
         * Do the work of replacing a FixedProbability connection with a
         * ConnectionList
         *
         * Go from this:
         * \verbatim
         *          <LL:Synapse>
         *               <FixedProbabilityConnection probability="0.11" seed="123">
         *                   <Delay Dimension="ms">
         *                       <FixedValue value="0.2"/>
         *                   </Delay>
         *               </FixedProbabilityConnection>
         * \endverbatim
         *
         * to this:
         * \verbatim
         *          <LL:Synapse>
         *               <ConnectionList>
         *                   <BinaryFile file_name="connection0.bin" num_connections="87360"
         *                               explicit_delay_flag="0" packed_data="true"/>
         *                   <Delay Dimension="ms">
         *                       <FixedValue value="1"/>
         *                   </Delay>
         *               </ConnectionList>
         * \endverbatim
         *
         * (an explicit list of connections, with distribution generated like the
         * code in SpineML_2_BRAHMS_CL_weight.xsl)
         *
         * @param fixedprob_node The FixedProbabilityConnection node to replace.
         *
         * @param src_name The source population.
         *
         * @param src_num The number of members in the source population.
         *
         * @param dst_population The destination population for the synapse.
         */
        void replace_fixedprob_connection (rapidxml::xml_node<>* fixedprob_node,
                                           const std::string& src_name,
                                           const std::string& src_num,
                                           const std::string& dst_population);

        /*!
         * Do the work of replacing an XML-only ConnectionList connection with a
         * BinaryFile ConnectionList
         *
         * Go from this:
         * \verbatim
         *          <LL:Synapse>
         *               <ConnectionList>
         *                   <Connection src_neuron="1" dst_neuron="2" delay="7"/>
         *                   <Connection src_neuron="1" dst_neuron="3" delay="7"/>
         *                   <Connection src_neuron="2" dst_neuron="4" delay="6"/>
         *                   <Connection src_neuron="2" dst_neuron="5" delay="7"/>
         *               </ConnectionList>
         * \endverbatim
         *
         * to this:
         * \verbatim
         *          <LL:Synapse>
         *               <ConnectionList>
         *                   <BinaryFile file_name="connection0.bin" num_connections="4"
         *                               explicit_delay_flag="1" packed_data="true"/>
         *               </ConnectionList>
         * \endverbatim
         *
         * @param connlist_node The ConnectionList node to update.
         *
         * @param src_name The source population.
         *
         * @param src_num The number of members in the source population.
         *
         * @param dst_population The destination population for the synapse.
         */
        void connection_list_to_binary (rapidxml::xml_node<> *connlist_node,
                                        const std::string& src_name,
                                        const std::string& src_num,
                                        const std::string& dst_population);

        /*!
         * Configure connection delays in @param cl using the delays specified in
         * @param parent_node.
         */
        void setup_connection_delays (rapidxml::xml_node<> *parent_node,
                                      spineml::ConnectionList& cl);

        /*!
         * Write out the pf_connectionN.bin file out. The @param
         * parent_node is used for the destination in the XML to
         * write, The @param cl contains the @see ConnectionList#write
         * method.
         */
        void write_connection_out (rapidxml::xml_node<> *parent_node,
                                   spineml::ConnectionList& cl);

        /*!
         * Replace a fixed value or random distribution state variable
         * property with an explicit binary file. For example, if it's
         * a fixed value property replace this:
         *
         * \verbatim
         * <Property name="m" dimension="?">
         *   <FixedValue value="1"/>
         * </Property>
         * \endverbatim
         *
         * with something like this:
         *
         * \verbatim
         * <Property name="m" dimension="?">
         *   <ValueList>
         *     <BinaryFile file_name="pf_explicitDataBinaryFile5.bin" num_elements="2500"/>
         *   </ValueList>
         * </Property>
         * \endverbatim
         *
         * As well as writing out the data file.
         *
         * Note that we have to look in the component xml to see if
         * the property is a state variable (which we should
         * preflight) or a parameter (which we shouldn't).
         *
         * @param prop_node The property within which the (e.g.)
         * FixedValue will be replaced.
         *
         * @param pop_size the number of members in the parent
         * population (this goes in the num_elements attrbute of the
         * BinaryFile node).
         */
        void replace_statevar_property (rapidxml::xml_node<>* prop_node, unsigned int pop_size);

        /*!
         * Generate the next file path for an explicit data file.
         *
         * @return explicit binary data file path.
         */
        std::string nextExplicitDataPath (void);

    private:
        /*!
         * Name of the XML text file.
         */
        std::string modelfile;

        /*!
         * Path to directory containing modelfile. Include trailing
         * '/' here.
         */
        std::string modeldir;

        /*!
         * An object into which to read the xml text prior to parsing.
         */
        spineml::AllocAndRead modeldata;

        /*!
         * Main xml_document object.
         */
        rapidxml::xml_document<> doc;

        /*!
         * the first population node pointer.
         */
        rapidxml::xml_node<>* first_pop_node;

        /*!
         * the root node pointer.
         */
        rapidxml::xml_node<>* root_node;

        /*!
         * The number for the next binary file name for connection lists,
         * e.g. '3' for pf_connection3.bin
         */
        unsigned int binfilenum;

        /*!
         * The number for the next binary file name for explicit data,
         * e.g. '4' for "pf_explictData4.bin"
         */
        unsigned int explicitData_binfilenum;

        /*!
         * A store of the properties which are state variables for
         * each component.
         */
        std::map<std::string, spineml::Component> components;

    public:
        /*!
         * If true, then make a backup of model.xml
         */
        bool backup;
    };

} // namespace spineml

#endif // _MODELPREFLIGHT_H_
