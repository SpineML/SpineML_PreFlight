/*!
 * An experiment.xml reading class.
 *
 * Author: Seb James
 * Date: Oct 2014
 */

#ifndef _EXPERIMENT_H_
#define _EXPERIMENT_H_

#include <string>
#include <sstream>
#include <stdexcept>
#include <iostream>
#include "rapidxml.hpp"
#include "allocandread.h"

namespace spineml
{
    /*!
     * A class to read the parameters stored in the SpineML
     * experiment.xml file.
     */
    class Experiment
    {
    public:
        /*!
         * Constructors
         */
        //@{
        Experiment()
            : filepath("model/experiment.xml")
            , simDuration (0)
            , simFixedDt (0)
            , simType ("Unknown")
        {
            this->parse();
        }
        Experiment(const std::string& path)
            : filepath(path)
            , simDuration (0)
            , simFixedDt (0)
            , simType ("Unknown")
        {
            this->parse();
        }
        //@}

        /*!
         * parse the experiment.xml file and populate the member
         * attributes.
         */
        void parse (void)
        {
            rapidxml::xml_document<> doc;

            AllocAndRead ar(this->filepath);
            char* textptr = ar.data();

            doc.parse<rapidxml::parse_declaration_node | rapidxml::parse_no_data_nodes>(textptr);

            // NB: This really DOES have to be the root node.
            rapidxml::xml_node<>* root_node = doc.first_node("SpineML");
            if (!root_node) {
                throw std::runtime_error ("experiment XML: no root SpineML node");
            }

            rapidxml::xml_node<>* expt_node = root_node->first_node ("Experiment");
            if (!expt_node) {
                throw std::runtime_error ("experiment XML: no Experiment node");
            }

            rapidxml::xml_node<>* model_node = expt_node->first_node ("Model");
            if (!model_node) {
                throw std::runtime_error ("experiment XML: no Model node");
            }
            rapidxml::xml_attribute<>* url_attr;
            if ((url_attr = model_node->first_attribute ("network_layer_url"))) {
                this->network_layer_path = url_attr->value();
            }

            rapidxml::xml_node<>* sim_node = expt_node->first_node ("Simulation");
            if (!sim_node) {
                throw std::runtime_error ("experiment XML: no Simulation node");
            }

            // Find duration
            rapidxml::xml_attribute<>* dur_attr;
            if ((dur_attr = sim_node->first_attribute ("duration"))) {
                std::stringstream sdss;
                sdss << dur_attr->value();
                sdss >> this->simDuration;
            }
            rapidxml::xml_node<>* euler_node = sim_node->first_node ("EulerIntegration");
            if (euler_node) {
                this->simType = "EulerIntegration";
                rapidxml::xml_attribute<>* dt_attr;
                if ((dt_attr = euler_node->first_attribute ("dt"))) {
                    std::stringstream dtss;
                    dtss << dt_attr->value();
                    dtss >> this->simFixedDt;
                    // Convert from ms to seconds:
                    this->simFixedDt /= 1000;
                }
            }
        }

        /*!
         * Accessors
         */
        //@{
        double getSimDuration (void)
        {
            return this->simDuration;
        }

        double getSimFixedDt (void)
        {
            return this->simFixedDt;
        }

        std::string modelUrl (void)
        {
            return this->network_layer_path;
        }
        //@}

        /*!
         * Return the simulation (fixed) solver rate in s^-1.
         */
        double getSimFixedRate (void)
        {
            double rate = 0.0;
            if (this->simFixedDt != 0) {
                rate = 1.0/this->simFixedDt;
            }
            return rate;
        }

        /*!
         * Add the raw property change request, as provided on the
         * command line. If the request has a bad format, throw an exception.
         */
        void addPropertyChangeRequest (const std::string& s)
        {
            std::cout << "property change request: " << s << "\n";
        }

    private:
        //! Path to the experiment xml file.
        std::string filepath;

        //! Model network layer path.
        std::string network_layer_path;

        //! Simulation duration in seconds
        double simDuration;

        //! Simulation fixed time step in *seconds*. (note: this is
        //! stored in experiment.xml in ms).
        double simFixedDt;

        //! Simulation solver type
        std::string simType;

        /*
         * Could also add time varying inputs and log output fields if
         * they were required (which they're not)
         */
    };

} // namespace spineml

#endif // _EXPERIMENT_H_
