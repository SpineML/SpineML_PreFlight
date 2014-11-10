/*!
 * A FixedValue element object.
 *
 * Author: Seb James
 * Date: Nov 2014
 */

#ifndef _FIXEDVALUE_H_
#define _FIXEDVALUE_H_

#include <string>
#include "rapidxml.hpp"

namespace spineml
{
    /*!
     * A class to represent a FixedValue element, such as this one:
     *
     * <Property name="m" dimension="?">
     *   <FixedValue value="1"/>
     * </Property>
     */
    class FixedValue
    {
    public:
        FixedValue(rapidxml::xml_node<>* fv_node, const unsigned int num_in_pop);
        ~FixedValue() {}

        /*!
         * Writes out this FixedValue as a ValueList, with explicit
         * binary data in a file. Replaces this:
         *
         * <Property name="m" dimension="?">
         *   <FixedValue value="1"/>
         * </Property>
         *
         * with something like this:
         *
         * <Property name="m" dimension="?">
         *   <ValueList>
         *     <BinaryFile file_name="pf_explicitDataBinaryFile5.bin" num_elements="2500"/>
         *   </ValueList>
         * </Property>
         *
         * As well as writing out the data file into @binary_file_name.
         */
        void writeAsValueList (rapidxml::xml_node<>* into_node,
                               const std::string& model_root,
                               const std::string& binary_file_name);

    private:
        /*!
         * Write out the fixed values as an explicit binary file.
         */
        void writeVLBinary (rapidxml::xml_node<>* into_node,
                            const std::string& model_root,
                            const std::string& binary_file_name);

        /*!
         * Re-writes the ConnectionList node's XML, in preparation for
         * writing out the connection list as an explicit binary file.
         */
        void writeVLXml (rapidxml::xml_node<>* into_node,
                         const std::string& model_root,
                         const std::string& binary_file_name);

    public:
        /*!
         * The fixed value as a number.
         */
        double value;

        /*!
         * The number of neurons for the population to which this
         * fixed value pertains. Used when writing out as value list.
         */
        unsigned int numInPopulation;
    };

} // namespace

#endif // _FIXEDVALUE_H_
