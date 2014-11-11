/*!
 * A PropertyContent element object.
 *
 * Author: Seb James
 * Date: Nov 2014
 */

#ifndef _PROPERTYCONTENT_H_
#define _PROPERTYCONTENT_H_

#include <string>
#include "rapidxml.hpp"

namespace spineml
{
    /*!
     * A base class for FixedValue, UniformDistribution, etc elements
     * which can be the contents of properties.
     *
     * <Property name="m" dimension="?">
     *   <!-- some sort of content element here -->
     * </Property>
     */
    class PropertyContent
    {
    public:
        PropertyContent(rapidxml::xml_node<>* content_node, const unsigned int num_in_pop);
        ~PropertyContent() {}

        /*!
         * Writes out this PropertyContent as a ValueList, with explicit
         * binary data in a file.
         *
         * @return true if a binary value list was written in place of
         * the existing property content, false if not (this may be
         * because the property content is already a binary
         * ValueList). Return false otherwise.
         */
        bool writeAsBinaryValueList (rapidxml::xml_node<>* into_node,
                                     const std::string& model_root,
                                     const std::string& binary_file_name);

    protected:
        /*!
         * Write out the fixed values as an explicit binary file.
         */
        void writeVLBinary (rapidxml::xml_node<>* into_node,
                            const std::string& model_root,
                            const std::string& binary_file_name);

        /*!
         * Write out the actual data to the binary file stream.
         */
        virtual void writeVLBinaryData (std::ostream& f) = 0;

        /*!
         * Re-writes the ConnectionList node's XML, in preparation for
         * writing out the connection list as an explicit binary file.
         */
        void writeVLXml (rapidxml::xml_node<>* into_node,
                         const std::string& model_root,
                         const std::string& binary_file_name);

        /*!
         * Set to true if this Property is already a binary ValueList.
         */
        bool alreadyBinary;

    public:
        /*!
         * The number of neurons for the population to which the
         * property pertains. Used when writing out as value list.
         */
        unsigned int numInPopulation;
    };

} // namespace

#endif // _PROPERTYCONTENT_H_
