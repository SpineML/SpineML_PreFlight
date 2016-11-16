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
#include <set>
#include "rapidxml.hpp"
#include "allocandread.h"
#include "component.h"
#include "connection_list.h"
#include "delaychange.h"

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
         * Like preflight(void), but takes a vector of DelayChanges
         * specified by the user, which will appear in the
         * experimentN.xml file. This vector is passed in so that
         * where there are connections whose delays are stored in
         * binary connection lists, the changed, fixed value delay can
         * be incorporated.
         */
        void preflight (const std::vector<DelayChange>& exptDelayChanges);

        /*!
         * Get the set of components used in the model. Used with the
         * command line option to get the list of components in the
         * model.
         */
        std::set<std::string> get_component_set (void);

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
         * Find the next XML node named @param elementName, starting
         * from @param current_node and searching down into child
         * nodes.
         */
        rapidxml::xml_node<>* findNamedElement (rapidxml::xml_node<>* current_node,
                                                std::string& elementName);

        /*!
         * Find the nearest parent XML node named @param elementName,
         * starting from @param current_node.
         */
        rapidxml::xml_node<>* findNamedParent (rapidxml::xml_node<>* current_node,
                                               std::string& elementName);

        /*!
         * Find an LL:Input element with src @param src, source port
         * @param srcPort, destination port @param dstPort in a Neuron
         * population named @param dst. Return pointer to the node if
         * found, null pointer if not. The container should be a
         * neuron population or I guess a postsynapse - any object in
         * the model which can contain an LL:Input.
         *
         * This is a recursive function.
         *
         * @param current_node The current node being searched.
         *
         * @param parentName The name attribute of the parent node of
         * current_node.
         *
         * @param src The src attribute of the LL:Input element -
         * where the connection comes from.
         *
         * @param srcPort The src_port attribute of the LL:Input
         * element. The port on the src population from which the
         * connection originates.
         *
         * @param dst The name of the object to which the connection
         * connects - the container in which the LL:Input resides.
         *
         * @param dstPort The dst_port attribute of the LL:Input
         * element.
         */
        rapidxml::xml_node<>* findLLInput (rapidxml::xml_node<>* current_node,
                                           const std::string& parentName,
                                           const std::string& src,
                                           const std::string& srcPort,
                                           const std::string& dst,
                                           const std::string& dstPort);

        /*!
         * Find an LL:WeightUpdate element with src @param src, source
         * port @param srcPort, destination port @param dstPort in a
         * Neuron population named @param dst. Return pointer to the
         * node if found, null pointer if not. The container should be
         * a neuron population or I guess a postsynapse - any object
         * in the model which can contain an LL:Input.
         *
         * This is a recursive function.
         *
         * @param current_node The current node being searched.
         *
         * @param name The name attribute of the LL:WeightUpdate
         * element - this specifies the source population, the
         * destination population and the synapse.
         */
        rapidxml::xml_node<>* findLLWeightUpdate (rapidxml::xml_node<>* current_node,
                                                  const std::string& name);

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
         * Search our delayChanges vector to find if there is a delay
         * change matching the passed-in arguments. If so, return the
         * delay as a float. If not, return a number <0 (-1.0).
         */
        float searchDelayChanges (const std::string& src, const std::string& dst,
                                  const std::string& synapseNum);

        /*!
         * Search our delayChanges vector to find if there is a delay
         * change matching the passed-in arguments. If so, return the
         * delay as a float. If not, return a number <0 (-1.0).
         */
        float searchDelayChanges (const std::string& src, const std::string& srcPort,
                                  const std::string& dst, const std::string& dstPort);

        /*!
         * Find the number of neurons in the destination population, starting
         * from the root node or the first population node (globals/members).
         *
         * @param dst_population The name attribute of the destination
         * population.
         */
        int find_num_neurons (const std::string& dst_population);

        /*!
         * Given the population node, just get the name of the
         * component used by that population.
         */
        std::string get_population_component_name (rapidxml::xml_node<>* pop_node);

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
         * Preflight a raw input in the model. Something looking like
         * this example, in which a population called Target connects
         * with a fixed probability connection to a population
         * "ToHCC":
         *
         *  <LL:Population>
         *    <LL:Neuron name="ToHCC" size="2" url="diffx.xml">
         *      <LL:Input src="Target" src_port="out" dst_port="xtargs">
         *        <FixedProbabilityConnection probability="0.11" seed="123">
         *          <Delay Dimension="ms">
         *            <FixedValue value="0"/>
         *          </Delay>
         *        </FixedProbabilityConnection>
         *      </LL:Input>
         *    </LL:Neuron>
         *    <Layout url="none.xml" seed="123" minimum_distance="0"/>
         *  </LL:Population>
         *  <LL:Population>
         *    <LL:Neuron name="Target" size="2" url="passthroughnb.xml"/>
         *    <Layout url="none.xml" seed="123" minimum_distance="0"/>
         *  </LL:Population>
         *
         * @param input_node The <Input> node. This gives the number of members in the
         * source population.
         *
         * @param dest_name The name of the destination population (or
         * projection, I guess).
         *
         * @param dest_num The number of members in the destination population.
         */
        void preflight_input (rapidxml::xml_node<>* input_node,
                              const std::string& dest_name,
                              const std::string& dest_num);

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
         * @param src_num The number of members in the source population.
         *
         * @param dst_num The number of members in the destination population.
         *
         * @param fixedValDelayChange If this is a connection which
         * has had its delay overridden in the experiment layer, then
         * the new delay is passed in as this argument.
         */
        void replace_fixedprob_connection (rapidxml::xml_node<>* fixedprob_node,
                                           const std::string& src_num,
                                           const std::string& dst_num,
                                           float fixedValDelayChange = -1.0);

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
         * @param fixedValDelayChange If this is a connection which
         * has had its delay overridden in the experiment layer, then
         * the new delay is passed in as this argument.
         */
        void connection_list_to_binary (rapidxml::xml_node<> *connlist_node,
                                        float fixedValDelayChange = -1);

        /*!
         * Configure connection delays in @param cl using the delays
         * specified in @param parent_node. This reads the <Delay>
         * element from the XML into the ConnectionList object.
         *
         * @param fixedValDelayChange If this is a connection which
         * has had its delay overridden in the experiment layer, then
         * the new delay is passed in as this argument.
         *
         * @return true if a Delay element was found, false if the
         * Delay element was not found.
         *
         * Throws exceptions on errors.
         */
        bool setup_connection_delays (rapidxml::xml_node<> *parent_node,
                                      spineml::ConnectionList& cl,
                                      float fixedValDelayChange = -1);

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
         * BinaryFile node). May also be the number of ? in the
         * postsynapse.
         */
        void replace_statevar_property (rapidxml::xml_node<>* prop_node, unsigned int pop_size);

        /*!
         * Replace a fixed value or random distribution state variable
         * property with an explicit binary file.
         *
         * Sub-calls replace_statevar_property
         *
         * \see replace_statevar_property
         *
         * @param prop_node The property within which the (e.g.)
         * FixedValue will be replaced.
         *
         * @param pop_size the number of members in the parent
         * population (this goes in the num_elements attrbute of the
         * BinaryFile node).
         *
         * @param component_name The component name for the component
         * enclosing the property. This may be a neuron body
         * component, a weight component or a postsynapse component.
         */
        void try_replace_statevar_property (rapidxml::xml_node<>* prop_node, unsigned int pop_size,
                                            const std::string& component_name);

        /*!
         * For the given component node (it might be a neuron body, a
         * weight component or a postsynapse component), find the
         * component name and return this.
         *
         * @param component_node Pointer to the component node whose
         * name is to be found.
         *
         * @return The component name.
         */
        std::string get_component_name (rapidxml::xml_node<>* component_node);

        /*!
         * Generate the next file path for an explicit data file.
         *
         * @return explicit binary data file path.
         */
        std::string nextExplicitDataPath (void);

        /*!
         * Determine the number of connections from a synapse given
         * the number in the destination population.
         *
         * @param synapse_node The \verbatim<LL:Synapse>\endverbatim
         *
         * @param num_in_src_population The number of neurons in the
         * source population from which this synapse projects.
         *
         * @param num_in_dst_population The number of neurons in the
         * destination population to which this synapse projects.
         *
         * @return the number of connections.
         */
        unsigned int get_num_connections (rapidxml::xml_node<>* synapse_node,
                                          unsigned int num_in_src_population,
                                          unsigned int num_in_dst_population);

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

        /*!
         * A set of the user-specified experiment-layer delay changes
         * that have been applied. Passed in and then stored here so
         * that these delay changes can be applied in cases where the
         * delays are expanded into binary lists.
         */
        std::vector<DelayChange> delayChanges;

    public:
        /*!
         * If true, then make a backup of model.xml
         */
        bool backup;
    };

} // namespace spineml

#endif // _MODELPREFLIGHT_H_
