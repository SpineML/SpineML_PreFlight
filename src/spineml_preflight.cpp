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

struct features {
    char * expt_path;
};

void zeroFeatures (struct features * f)
{
        f->expt_path = NULL;
}

int main (int argc, char * argv[])
{
    int rtn = 0;

    // popt command line argument processing setup
    struct features f;
    zeroFeatures (&f);
    struct poptOption opt[] = {
        POPT_AUTOHELP
        {"expt_path", 'e',
         POPT_ARG_STRING, &(f.expt_path), 0,
         "Provide the path to the experiment.xml file for the model you wish to preflight."},
        POPT_TABLEEND
    };
    poptContext con;
    con = poptGetContext (argv[0], argc, (const char**)argv, opt, 0);
    while (poptGetNextOpt(con) != -1) {}

    try {
        if (f.expt_path == NULL) {
            throw runtime_error ("Please supply the path to experiment xml file with the -e option.");
        }
        spineml::Experiment expt (f.expt_path);

        string model_dir(f.expt_path);
        stripUnixFile (model_dir);
        model_dir += "/";
        cout << "model_dir: " << model_dir << endl;

        // Fixme: Get path from the expt above and use below:
        spineml::ModelPreflight model (model_dir, expt.modelUrl());
        model.preflight();
        // Write out the now modified xml:
        model.write();
    } catch (const exception& e) {
        cerr << "Error thrown: " << e.what() << endl;
        rtn = -1;
    }

    poptFreeContext(con);
    return rtn;
}
