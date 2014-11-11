/*
 * Implementation of PropertyContent class.
 * Author: Seb James
 * Date: Nov 2014
 */

#include <string>
#include <sstream>
#include <fstream>
#include <stdexcept>
#include "propertycontent.h"
#include "rapidxml.hpp"

using namespace std;
using namespace spineml;
using namespace rapidxml;

PropertyContent::PropertyContent(xml_node<>* fv_node, const unsigned int num_in_pop)
    : alreadyBinary (false)
    , numInPopulation (num_in_pop)
{
}

bool
PropertyContent::writeAsBinaryValueList (rapidxml::xml_node<>* into_node,
                                         const std::string& model_root,
                                         const std::string& binary_file_name)
{
    if (this->alreadyBinary == true) {
        return false;
    }
    this->writeVLXml (into_node, model_root, binary_file_name);
    this->writeVLBinary (into_node, model_root, binary_file_name);
    return true;
}

void
PropertyContent::writeVLBinary (rapidxml::xml_node<>* into_node,
                                const std::string& model_root,
                                const std::string& binary_file_name)
{
    string path = model_root + binary_file_name;
    ofstream f;
    f.open (path.c_str(), ios::out|ios::trunc);
    if (!f.is_open()) {
        stringstream ee;
        ee << __FUNCTION__ << " Failed to open file '" << path << "' for writing.";
        throw runtime_error (ee.str());
    }

    this->writeVLBinaryData (f);

    f.close();
}

void
PropertyContent::writeVLXml (rapidxml::xml_node<>* into_node,
                             const std::string& model_root,
                             const std::string& binary_file_name)
{
    // into_node should be the PropertyContent node itself.
    xml_document<>* thedoc = into_node->document();
    // Remove any attributes and child nodes from into_node.
    into_node->remove_all_attributes();
    into_node->remove_all_nodes();
    // Change name of node from whatever
    // (e.g. FixedProbabilityConnection) to .
    into_node->name("ValueList");

    // Add the BinaryFile node
    xml_node<>* binfile_node = thedoc->allocate_node (node_element, "BinaryFile");
    char* bfn_alloced = thedoc->allocate_string (binary_file_name.c_str());
    xml_attribute<>* file_name_attr = thedoc->allocate_attribute ("file_name", bfn_alloced);

    stringstream num_elem_ss;
    num_elem_ss << this->numInPopulation;
    char* num_elem_alloced = thedoc->allocate_string (num_elem_ss.str().c_str());
    xml_attribute<>* num_elem_attr = thedoc->allocate_attribute ("num_elements", num_elem_alloced);

    binfile_node->append_attribute (file_name_attr);
    binfile_node->append_attribute (num_elem_attr);

    into_node->prepend_node (binfile_node);
}
