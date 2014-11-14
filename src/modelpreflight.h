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

namespace spineml
{
    /*!
     * A class to read, then find and replace relevant sections of
     * model.xml
     */
    class ModelPreflight
    {
    public:
        ModelPreflight(const std::string& fdir, const std::string& fname);
        ~ModelPreflight();

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
         * Find a property called @propertyName in, for example, a
         * Neuron population named @containerName. Return pointer to
         * the node if found, null pointer if not. The container may
         * be a neuron population or a projection - any object in the
         * model which can contain a property.
         */
        rapidxml::xml_node<>* findProperty (rapidxml::xml_node<>* current_node,
                                            const std::string& parentName,
                                            const std::string& containerName,
                                            const std::string& propertyName);

        /*!
         * Backup the existing model.xml file, then overwrite it with
         * the current content of this->doc.
         */
        void write (void);

    private:

        /*!
         * Find the number of neurons in the destination population, starting
         * from the root node or the first population node (globals/members).
         */
        int find_num_neurons (const std::string& dst_population);

        /*!
         * Take a population node, and process this for any changes we need to
         * make. Sub-calls preflight_projection.
         */
        void preflight_population (rapidxml::xml_node<>* pop_node);

        /*!
         * Process the passed-in projection, making any changes necessary.
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
         * OneToOne - leave unmodified
         *  <LL:Synapse>
         *      <OneToOneConnection>
         *          <Delay Dimension="ms">
         *              <FixedValue value="3"/>
         *          </Delay>
         *      </OneToOneConnection>
         *
         * AllToAll - leave unmodified
         *
         *  <LL:Synapse>
         *      <AllToAllConnection>
         *          <Delay Dimension="ms">
         *              <FixedValue value="4"/>
         *          </Delay>
         *      </AllToAllConnection>
         *
         * FixedProbability - replace connectivity and delay with expl. list.
         *
         *   <LL:Synapse>
         *      <FixedProbabilityConnection probability="0.01" seed="123">
         *          <Delay Dimension="ms">
         *              <NormalDistribution mean="0" variance="1" seed="123"/>
         *          </Delay>
         *      </FixedProbabilityConnection>
         *
         * Explicit list in XML - replace with explicit binary list:
         *
         *  <LL:Synapse>
         *      <ConnectionList>
         *          <Connection src_neuron="1" dst_neuron="2" delay="4"/>
         *          <Connection src_neuron="1" dst_neuron="3" delay="5"/>
         *      </ConnectionList>
         *
         * Finally KernelConnection is deprecated and will be removed
         * in a future version of SpineCreator.
         */
        void preflight_synapse (rapidxml::xml_node<>* proj_node,
                                const std::string& src_name, const std::string& src_num,
                                const std::string& dst_population);

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
        void replace_fixedprob_connection (rapidxml::xml_node<>* syn_node,
                                           const std::string& src_name,
                                           const std::string& src_num,
                                           const std::string& dst_population);

        /*!
         * Do the work of replacing an XML-only ConnectionList connection with a
         * BinaryFile ConnectionList
         *
         * Go from this:
         *          <LL:Synapse>
         *               <ConnectionList>
         *                   <Connection src_neuron="1" dst_neuron="2" delay="7"/>
         *                   <Connection src_neuron="1" dst_neuron="3" delay="7"/>
         *                   <Connection src_neuron="2" dst_neuron="4" delay="6"/>
         *                   <Connection src_neuron="2" dst_neuron="5" delay="7"/>
         *               </ConnectionList>
         *
         * to this:
         *          <LL:Synapse>
         *               <ConnectionList>
         *                   <BinaryFile file_name="connection0.bin" num_connections="4"
         *                               explicit_delay_flag="1" packed_data="true"/>
         *               </ConnectionList>
         */
        void connection_list_to_binary (rapidxml::xml_node<> *connlist_node,
                                        const std::string& src_name,
                                        const std::string& src_num,
                                        const std::string& dst_population);

        /*!
         * Configure connection delays in @param cl using the delays specified in
         * parent_node.
         */
        void setup_connection_delays (rapidxml::xml_node<> *parent_node,
                                      spineml::ConnectionList& cl);

        /*!
         * Write out the pf_connectionN.bin file out.
         */
        void write_connection_out (rapidxml::xml_node<> *parent_node,
                                   spineml::ConnectionList& cl);

        /*!
         * Replace a fixed value or random distribution state variable
         * property with an explicit binary file. For example, if it's
         * a fixed value property replace this:
         *
         * <Property name="m" dimension="?">
         *   <FixedValue value="1"/>
         * </Property>
         *
         * with something like this:
         *
         * <Property name="m" dimension="?">
         *   <ValueList>
         *     <BinaryFile file_name="pf_explicitDataBinaryFile5.bin" num_elements="2500"/>
         *   </ValueList>
         * </Property>
         *
         * As well as writing out the data file.
         *
         * Note that we have to look in the component xml to see if
         * the property is a state variable (which we should
         * preflight) or a parameter (which we shouldn't).
         */
        void replace_statevar_property (rapidxml::xml_node<>* prop_node, unsigned int pop_size);

        /*!
         * Generate the next file path for an explicit data file.
         */
        std::string nextExplicitDataPath (void);

        /*!
         * A little utility. Given a unixPath containing "blah.xml",
         * change unixPath to "blah".
         */
        void stripFileSuffix (std::string& unixPath);

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
