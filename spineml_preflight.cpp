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

using namespace std;

void stripUnixFile (std::string& unixPath)
{
        string::size_type pos (unixPath.find_last_of ('/'));
        if (pos != string::npos) {
                unixPath = unixPath.substr (0, pos);
        }
}

int main()
{
    try {
        // Fixme: Use popt to get command line arg for the experiment:
        string expt_path ("./model/experiment.xml");
        spineml::Experiment expt (expt_path);

        string model_dir = expt_path;
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
        return -1;
    }
    return 0;
}
