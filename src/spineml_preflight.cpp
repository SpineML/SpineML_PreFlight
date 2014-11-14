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
    //! To hold the current property change option string. Used temporarily by the property change option (-p).
    char * property_change;
    //! To hold a list of all property changes requested by the user
    vector<string> property_changes;
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
}

/*
 * Lazily make CmdOptions global; allows callback to access this
 * easily.
 */
struct CmdOptions cmdOptions;

/*!
 * This callback is used when there's a -p option, to allow my to
 * collect multiple property change directives from the user.
 *
 * e.g. spineml_preflight --property_change="Pop:tau:43" --property_change="Pop:m:0.203"
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
        cmdOptions.property_changes.push_back (cmdOptions.property_change);
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

        // options following this will cause the callback to be executed.
        { "callback", '\0',
          POPT_ARG_CALLBACK|POPT_ARGFLAG_DOC_HIDDEN, (void*)&property_change_callback, 0,
          NULL, NULL },

        {"property_change", 'p',
         POPT_ARG_STRING, &(cmdOptions.property_change), 0,
         "Change a property. Provide an argument like \"Population:tau:45\". "
         "This option can be used multiple times."},

        POPT_AUTOALIAS
        POPT_AUTOHELP
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

        string model_dir(cmdOptions.expt_path);
        Util::stripUnixFile (model_dir);
        model_dir += "/";

        spineml::Experiment expt (cmdOptions.expt_path);
        expt.setModelDir (model_dir);

        vector<string>::const_iterator pciter = cmdOptions.property_changes.begin();
        while (pciter != cmdOptions.property_changes.end()) {
            // Add this "property change request" to the experiment.
            expt.addPropertyChangeRequest (*pciter);
            ++pciter;
        }

        // Get path from the expt above
        spineml::ModelPreflight model (model_dir, expt.modelUrl());
        if (cmdOptions.backup_model > 0) {
            model.backup = true;
        }
        model.preflight();
        // Write out the now modified xml:
        model.write();
        cout << "Preflight Finished.\n";
    } catch (const exception& e) {
        cout << "Preflight Error: " << e.what() << endl;
        rtn = -1;
    }

    poptFreeContext(con);
    return rtn;
}
