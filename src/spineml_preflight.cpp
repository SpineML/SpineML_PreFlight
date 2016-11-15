/*
 * spineml_preflight main() function
 *
 * Author: Seb James, 2014.
 */

/*!
 * \mainpage SpineML_PreFlight
 *
 * \section intro_sec Introduction
 *
 * This program parses the SpineML model, updating any aspects of the
 * model where parameters, weights or connectivity are specified in
 * meta-form. For example, where connections are given in fixed
 * probability form, this program creates a connection list file and
 * modifies the \verbatim<FixedProbabilityConnection>\endverbatim xml
 * element into a \verbatim<ConnectionList>\endverbatim element with
 * an associated binary connection list file.
 *
 * It also replaces those \verbatim<Property>\endverbatim elements
 * which are state variable initial values with binary connection
 * lists.
 *
 * The original model.xml file is optonally renamed model.xml.bu and a
 * new model.xml file is written out containing the new, specific
 * information.
 *
 * If the user requests "property changes" via the command line, then
 * this program also modified the experiment.xml file, adding model
 * configuration changes there.
 *
 * The dependency-free rapidxml header-only xml parser is used to
 * read, modify and write out XML files.
 */

#include <exception>
#include <iostream>
#include <string>
#include "experiment.h"
#include "modelpreflight.h"
#include "util.h"

extern "C" {
#include <popt.h>
}

using namespace std;
using spineml::Util;

/*!
 * libpopt features - the features that are available to change on the
 * command line.
 */
struct CmdOptions {
    //! To hold the path to the experiment.xml file. The -e option.
    char * expt_path;
    //! To hold a flag to say whether the model.xml file should be backed up before being modified. The -b option.
    int backup_model;
    //! To take the option to list components used in the model
    int list_components;
    //! To take the option to show the model file name
    int show_model_file;
    //! To hold the current property change option string. Used temporarily by the property change option (-p).
    char * property_change;
    //! To hold a list of all property changes requested by the user
    vector<string> property_changes;
    //! To hold the current constant current option string. Used temporarily by the constant current option (-c).
    char * constant_current;
    //! To hold the time varying current option string. Used temporarily by the time varying current option (-t).
    char * tvarying_current;
    //! To hold a list of all constant currents requested by the user
    vector<string> constant_currents;
    //! To hold a list of all time varying currents requested by the user
    vector<string> tvarying_currents;
    //! To hold a list of all connection delay changes requested by the user
    char * delay_change;
    vector<string> delay_changes;

    // FIXME: Add array_tvarying_currents and array_constant_currents
};

/*!
 * Initializes a CmdOptions object via a @param copts pointer
 */
void zeroCmdOptions (CmdOptions* copts)
{
    copts->expt_path = NULL;
    copts->backup_model = 0;
    copts->property_change = NULL;
    copts->property_changes.clear();
    copts->constant_current = NULL;
    copts->constant_currents.clear();
    copts->tvarying_current = NULL;
    copts->tvarying_currents.clear();
    copts->delay_change = NULL;
    copts->delay_changes.clear();
}

/*
 * Lazily make CmdOptions global; allows callback to access this
 * easily.
 */
struct CmdOptions cmdOptions;

/*!
 * This callback is used when there's a -p option, to allow me to
 * collect multiple property change directives from the user.
 *
 * e.g. spineml_preflight --property_change="Pop:tau:43" --property_change="Pop:m:0.203"
 *
 * It also handles -c options for constant currents.
 */
void property_change_callback (poptContext con,
                               enum poptCallbackReason reason,
                               const struct poptOption * opt,
                               const char * arg,
                               void * data)
{
    switch(reason)
    {
    case POPT_CALLBACK_REASON_PRE:
        // We don't see this
        break;
    case POPT_CALLBACK_REASON_POST:
        // Ignore
        break;
    case POPT_CALLBACK_REASON_OPTION:
        // Do stuff here.
        if (opt->shortName == 'c') {
            cmdOptions.constant_currents.push_back (cmdOptions.constant_current);
        } else if (opt->shortName == 'p') {
            cmdOptions.property_changes.push_back (cmdOptions.property_change);
        } else if (opt->shortName == 'd') {
            cmdOptions.delay_changes.push_back (cmdOptions.delay_change);
        } else if (opt->shortName == 't') {
            cmdOptions.tvarying_currents.push_back (cmdOptions.tvarying_current);
        }
        break;
    }
}

/*!
 * main entry point for spineml_preflight
 */
int main (int argc, char * argv[])
{
    int rtn = 0;

    // popt command line argument processing setup
    zeroCmdOptions (&cmdOptions);

    struct poptOption opt[] = {
        POPT_AUTOHELP

        {"expt_path", 'e',
         POPT_ARG_STRING, &(cmdOptions.expt_path), 0,
         "Provide the path to the experiment.xml file for the model you wish to preflight."},

        {"backup_model", 'b',
         POPT_ARG_NONE, &(cmdOptions.backup_model), 0,
         "If set, make a backup of model.xml as model.xml.bu."},

        {"list_components", 'l',
         POPT_ARG_NONE, &(cmdOptions.list_components), 0,
         "If set, list the components of the model, one per line on stdout."},

        {"show_model_file", 's',
         POPT_ARG_NONE, &(cmdOptions.show_model_file), 0,
         "If set, list the name of the network layer file on stdout (often called model.xml)."},

        // options following this will cause the callback to be executed.
        { "callback", '\0',
          POPT_ARG_CALLBACK|POPT_ARGFLAG_DOC_HIDDEN, (void*)&property_change_callback, 0,
          NULL, NULL },

        {"property_change", 'p',
         POPT_ARG_STRING, &(cmdOptions.property_change), 0,
         "Change a property. Provide an argument like \"Population:tau:45\". "
         "This option can be used multiple times."},

        {"delay_change", 'd',
         POPT_ARG_STRING, &(cmdOptions.delay_change), 0,
         "Change a delay on a projection of generic connection. For projections, "
         "provide an argument like \"PopA:PopB:0:45\" to set a"
         "delay of 45 ms to the projection from PopA to PopB on synapse 0."
         "For generic connections the argument should be \"PopA:PortA:PopB:PortB:45\" "
         "to set a 45 ms delay to the connection from PortA on PopA to PortB on PopB. "
         "It is only possible to set fixed delays using this argument. "
         "This option can be used multiple times."},

        {"constant_current", 'c',
         POPT_ARG_STRING, &(cmdOptions.constant_current), 0,
         "Override the input current(s) with constant currents. Provide an "
         "argument like \"Population:Port:45\". "
         "This option can be used multiple times."},

        {"tvarying_current", 't',
         POPT_ARG_STRING, &(cmdOptions.tvarying_current), 0,
         "Override the input current(s) with time varying currents. Provide an "
         "argument like \"Population:Port:0,0,100,150,300,0\". The comma separated "
         "string of numbers is a set of time(ms)/current pairs. The example above "
         "means start with 0 current, set it to 150 at time 100 ms and to 0 again at "
         "time 300ms. If the input should be a spiking input, then add the "
         "spike train distribution to the specification; something like "
         "\"Population:Port:poisson:0,0,100,150,300,0\". The distribution could be "
         "\"poisson\" or \"uniform\" - it is the presence of this distribution which "
         "tells spineml_preflight that you want spiking inputs. "
         // Uncomment this when the feature is implemented:
         //"For an array of time "
         //"varying inputs, add additional arrays to the list, using a semi-colon to "
         //"separate: \"Population:Port:poisson:0,0,100,150,300,0;0,100,300,300,0\" or "
         //"\"Population:Port:0,0,100,150,300,0;0,100,300,300,0\" for a non-spiking input. "
         "This option can be used multiple times."},

        POPT_AUTOALIAS
        POPT_TABLEEND
    };
    poptContext con;
    con = poptGetContext (argv[0], argc, (const char**)argv, opt, 0);
    while (poptGetNextOpt(con) != -1) {}

    try {
        if (cmdOptions.expt_path == NULL) {
            throw runtime_error ("Please supply the path to experiment xml file "
                                 "with the -e option.");
        }

        string expt_path(cmdOptions.expt_path);
        string model_dir(cmdOptions.expt_path);
        Util::stripUnixFile (model_dir);
        if (expt_path == model_dir) {
            // Then there was no path to strip and so there's no model_dir
            model_dir = "";
        } else {
            // Append a / to the model dir path
            model_dir += "/";
        }

        spineml::Experiment expt (cmdOptions.expt_path);
        expt.setModelDir (model_dir);

        vector<string>::const_iterator pciter = cmdOptions.property_changes.begin();
        while (pciter != cmdOptions.property_changes.end()) {
            // Add this "property change request" to the experiment.
            expt.addPropertyChangeRequest (*pciter);
            ++pciter;
        }

        pciter = cmdOptions.delay_changes.begin();
        while (pciter != cmdOptions.delay_changes.end()) {
            // Add this "delay change request" to the experiment.
            cout << "Add delay change request";
            expt.addDelayChangeRequest (*pciter);
            ++pciter;
        }

        pciter = cmdOptions.constant_currents.begin();
        while (pciter != cmdOptions.constant_currents.end()) {
            // Add this "constant current request" to the experiment.
            expt.addConstantCurrentRequest (*pciter);
            ++pciter;
        }

        pciter = cmdOptions.tvarying_currents.begin();
        while (pciter != cmdOptions.tvarying_currents.end()) {
            // Add this "time varying current request" to the experiment.
            expt.addTimeVaryingCurrentRequest (*pciter);
            ++pciter;
        }

        // Get path from the expt above
        spineml::ModelPreflight model (model_dir, expt.modelUrl());
        if (cmdOptions.backup_model > 0) {
            model.backup = true;
        }
        if (cmdOptions.list_components > 0 || cmdOptions.show_model_file > 0) {
            if (cmdOptions.list_components > 0) {
                set<string> clist = model.get_component_set();
                set<string>::const_iterator clisti = clist.begin();
                while (clisti != clist.end()) {
                    cout << *clisti << endl;
                    ++clisti;
                }
            }
            if (cmdOptions.show_model_file > 0) {
                cout << expt.modelUrl() << endl;
            }
        } else {
            model.preflight();
            // Write out the now modified xml:
            model.write();
            cout << "Preflight Finished.\n";
        }
    } catch (const exception& e) {
        cerr << "Preflight Error: " << e.what() << endl;
        rtn = -1;
    }

    poptFreeContext(con);
    return rtn;
}
