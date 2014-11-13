/*
 * This program parses the SpineML model, updating any aspects of the
 * model where parameters, weights or connectivity are specified in
 * meta-form. For example, where connections are given in kernel form,
 * this program creates a connection list file and modifies the
 * <KernelConnection> xml element into a <ConnectionList> element with
 * an associated binary connection list file.
 *
 * The original model.xml file is renamed model_orig.xml and a new
 * model.xml file is written out containing the new, specific
 * information.
 *
 * The dependency-free rapidxml header-only xml parser is used to
 * read, modify and write out model.xml.
 *
 * Author: Seb James, 2014.
 */

#include <exception>
#include <iostream>
#include <string>
#include "experiment.h"
#include "modelpreflight.h"

extern "C" {
#include <popt.h>
}

using namespace std;

void stripUnixFile (std::string& unixPath)
{
    string::size_type pos (unixPath.find_last_of ('/'));
    if (pos != string::npos) {
        unixPath = unixPath.substr (0, pos);
    }
}

/*
 * libpopt features - the features that are available to change on the
 * command line.
 */
struct CmdOptions {
    char * expt_path; // -e option
    int backup_model; // -b option
    char * property_change; // Used temporarily by the prop. change option
    vector<string> property_changes; // A record of all property changes
};

void zeroCmdOptions (CmdOptions* copts)
{
    copts->expt_path = NULL;
    copts->backup_model = 0;
    copts->property_change = NULL;
    copts->property_changes.clear();
}

/*
 * Lazily make CmdOptions global; allows callback to access this easily.
 */
struct CmdOptions cmdOptions;

/*
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

        spineml::Experiment expt (cmdOptions.expt_path);

        vector<string>::const_iterator pciter = cmdOptions.property_changes.begin();
        while (pciter != cmdOptions.property_changes.end()) {
            // Add this "property change request" to the experiment.
            expt.addPropertyChangeRequest (*pciter);
            ++pciter;
        }

        string model_dir(cmdOptions.expt_path);
        stripUnixFile (model_dir);
        model_dir += "/";

        // Fixme: Get path from the expt above and use below:
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
