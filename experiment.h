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
         * Constructors and destructor
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
        ~Experiment() {}
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

#ifdef WANT_TO_SEE_ENCODING
            if (doc.first_node()->first_attribute("encoding")) {
                std::string encoding = doc.first_node()->first_attribute("encoding")->value();
                std::cout << "encoding: " << encoding << std::endl;
            }
#endif

            // NB: This really DOES have to be the root node.
            rapidxml::xml_node<>* root_node = doc.first_node("SpineML");
            if (!root_node) {
                std::cerr << "no root SpineML node\n";
                return;
            }

            rapidxml::xml_node<>* expt_node = root_node->first_node ("Experiment");
            if (!expt_node) {
                std::cerr << "no Experiment node\n";
                return;
            }

            rapidxml::xml_node<>* sim_node = expt_node->first_node ("Simulation");
            if (!sim_node) {
                std::cerr << "no Simulation node\n";
                return;
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

    private:
        //! Path to the experiment xml file.
        std::string filepath;

        //! Expt name, no need for this.
        //std::string name;

        //! Expt desc. Also no need
        //std::string description;

        //! Model network layer path. Not required.
        //std::string network_layer_path

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
