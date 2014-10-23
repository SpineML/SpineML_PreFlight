/*!
 * A model.xml parsing/modifying class.
 *
 * Author: Seb James
 * Date: Oct 2014
 */

#ifndef _MODELPREFLIGHT_H_
#define _MODELPREFLIGHT_H_

#include <string>
#include "rapidxml.hpp"
#include "allocandread.h"

/*!
 * It may be that we need to run this for HL and LL models.
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
        ModelPreflight(const std::string& path);
        ~ModelPreflight();

        /*!
         * Start the work. Begin searching for Population nodes and
         * then search within each Population for anything we need to
         * modify in this preflight process.
         */
        void preflight (void);

        /*!
         * Find the number of neurons in the destination population, starting
         * from the root node or the first population node (globals/members).
         */
        int find_num_neurons (const std::string& dst_population);

        /*!
         * Take a population node, and process this for any changes we need to
         * make. Sub-calls preprocess_projection.
         */
        void preprocess_population (rapidxml::xml_node<>* pop_node);

        /*!
         * Process the passed-in projection, making any changes necessary.
         */
        void preprocess_projection (rapidxml::xml_node<>* proj_node,
                                    const std::string& src_name, const std::string& src_num);

        /*!
         * Process the synapse. Search for KernelConnection and modify if found.
         */
        void preprocess_synapse (rapidxml::xml_node<>* proj_node,
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
         * Backup the existing model.xml file, then overwrite it with
         * the current content of this->doc.
         */
        void write (void);

    private:

        /*!
         * Path to the XML text file.
         */
        std::string filepath;

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
    };

} // namespace spineml

#endif // _MODELPREFLIGHT_H_
