/*!
 * An experiment.xml reading class.
 *
 * Author: Seb James
 * Date: Oct 2014
 */

#ifndef _EXPERIMENT_H_
#define _EXPERIMENT_H_

#include <string>

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
        Experiment();
        Experiment(const std::string& path);
        //@}

        /*!
         * parse the experiment.xml file and populate the member
         * attributes.
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
         */
        void addPropertyChangeRequest (const std::string& s);

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
