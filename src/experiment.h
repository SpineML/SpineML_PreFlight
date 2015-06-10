/*!
 * An experiment.xml reading class.
 *
 * Author: Seb James
 * Date: Oct 2014
 */

#ifndef _EXPERIMENT_H_
#define _EXPERIMENT_H_

#include <string>
#include <vector>
#include <stdexcept>
#include <utility>
#include "rapidxml.hpp"

namespace spineml
{
    /*!
     * A class to read the parameters stored in the SpineML
     * experiment.xml file.
     *
     * This class also handles the task of reading experiment.xml and
     * then adding "Model Configurations" to experiment.xml to
     * override parameter or state variable initial values at model
     * execution time. This allows spineml_preflight to take command
     * line options to change parameter/state variable properties.
     */
    class Experiment
    {
    public:
        /*!
         * Constructors
         */
        //@{
        Experiment();
        Experiment(const std::string& path);
        //@}

        /*!
         * parse the experiment.xml file and populate the member
         * attributes.
         *
         * Note that this method instatiates its own xml_document, and
         * this xml_document exists only within the scope of this
         * method. If this->addPropertyChangeRequest is called, then a
         * new xml_document is instatiated within the scope of that
         * method.
         */
        void parse (void);

        /*!
         * Accessors
         */
        //@{
        double getSimDuration (void);
        double getSimFixedDt (void);
        std::string modelUrl (void);
        //@}

        /*!
         * Return the simulation (fixed) solver rate in s^-1.
         */
        double getSimFixedRate (void);

        /*!
         * Add the raw property change request, as provided on the
         * command line. If the request has a bad format, throw an exception.
         *
         * This method splits up the single command line option passed
         * as @param pcrequest  It then checks in the model.xml to ensure
         * that the requested property exists. If this turns out to be
         * the case, it then calls @see insertModelConfig to add the
         * property change/model configuration update.
         */
        void addPropertyChangeRequest (const std::string& pcrequest);

        /*!
         * Add the raw constant current request, as provided on the
         * command line. If the request has a bad format, throw an
         * exception.
         *
         * Like addPropertyChangeRequest, this method splits up the
         * single command line option passed as @param pcrequest It
         * then checks in the model.xml to ensure that the requested
         * property exists. If this turns out to be the case, it then
         * calls @see insertModelConfig to add the constant
         * current/model configuration update.
         */
        void addConstantCurrentRequest (const std::string& ccrequest);

        /*!
         * This actually does the work of inserting a new
         * Configuration node to the experiment's Model node. Its
         * passed a vector of 3 elements: target population, target
         * property and new value which have been extracted from a
         * command line argument.
         */
        void insertModelConfig (rapidxml::xml_node<>* property_node,
                                const std::vector<std::string>& elements);

        void insertExptConstCurrent (const std::vector<std::string>& elements);

        //! A simple accessor for this->modelDir.
        void setModelDir (const std::string& dir);

    private:
        //! write The XML document provided in @param the_doc out to
        //! file.
        void write (const rapidxml::xml_document<>& the_doc);

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

        //! The location of experiment.xml and model.xml
        std::string modelDir;

        /*
         * Could also add time varying inputs and log output fields if
         * they were required (which they're not)
         */
    };

} // namespace spineml

#endif // _EXPERIMENT_H_
