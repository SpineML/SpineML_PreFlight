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
 * This program parses the SpineML model, updating explicit binary
 * data files from float to double format or back again.
 */

#include <exception>
#include <iostream>
#include <string>
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
    //! To hold the path to the model.xml file. The -m option
    char * model_path;
    //! To hold a flag to say whether we go backwards - from double to float. -b option
    int backwards;
};

/*!
 * Initializes a CmdOptions object via a @param copts pointer
 */
void zeroCmdOptions (CmdOptions* copts)
{
    copts->model_path = NULL;
    copts->backwards = 0;
}

/*!
 * main entry point for spineml_preflight
 */
int main (int argc, char * argv[])
{
    int rtn = 0;

    // popt command line argument processing setup
    CmdOptions cmdOptions;
    zeroCmdOptions (&cmdOptions);

    struct poptOption opt[] = {
        POPT_AUTOHELP

        {"model_path", 'm',
         POPT_ARG_STRING, &(cmdOptions.model_path), 0,
         "Provide the path to the model.xml file for the model you wish to update."},

        {"backwards", 'b',
         POPT_ARG_NONE, &(cmdOptions.backwards), 0,
         "If set, convert backwards from double to float, not forwards from float to double."},

        POPT_AUTOALIAS
        POPT_TABLEEND
    };
    poptContext con;
    con = poptGetContext (argv[0], argc, (const char**)argv, opt, 0);
    while (poptGetNextOpt(con) != -1) {}

    try {
        if (cmdOptions.model_path == NULL) {
            throw runtime_error ("Please supply the path to model xml file "
                                 "with the -m option.");
        }

        string model_dir(cmdOptions.model_path);
        Util::stripUnixFile (model_dir);
        model_dir += "/";
        string model_fname(cmdOptions.model_path);
        Util::stripUnixPath (model_fname);

        spineml::ModelPreflight model (model_dir, model_fname);
        if (cmdOptions.backwards) {
            model.binaryDataDoubleToFloat();
        } else {
            // Normal, forwards.
            model.binaryDataFloatToDouble();
        }
        cout << "Float2Double Finished.\n";
    } catch (const exception& e) {
        cout << "Float2Double Error: " << e.what() << endl;
        rtn = -1;
    }

    poptFreeContext(con);
    return rtn;
}
