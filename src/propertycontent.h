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
     * \verbatim
     * <Property name="m" dimension="?">
     *   <!-- some sort of content element here -->
     * </Property>
     * \endverbatim
     *
     * This class and its children can also be used to generate
     * property change elements like this:
     *
     * \verbatim
     * <UL:Property name="m" dimension="?">
     *   <!-- content element here -->
     * </UL:Property>
     * \endverbatim
     */
    class PropertyContent
    {
    public:
        /*!
         * Construct PropertyContent object using the @param
         * content_node and @param num_in_pop to create it.
         */
        PropertyContent(rapidxml::xml_node<>* content_node, const unsigned int num_in_pop);

        /*!
         * Construct empty PropertyContent object
         */
        PropertyContent();

        /*!
         * Writes out this PropertyContent as a ValueList, with explicit
         * binary data in a file.
         *
         * @param into_node The (e.g.) FixedValue node, which will be
         * renamed as a ValueList node.
         *
         * @param model_root The root path of the model
         *
         * @param binary_file_name The file name of the binary file
         * into which the actual value list data will be written.
         *
         * @return true if a binary value list was written in place of
         * the existing property content, false if not (this may be
         * because the property content is already a binary
         * ValueList).
         */
        bool writeAsBinaryValueList (rapidxml::xml_node<>* into_node,
                                     const std::string& model_root,
                                     const std::string& binary_file_name);

        /*!
         * Setter. Sets @see propertyName to @param name
         */
        void setPropertyName (const std::string& name);

        /*!
         * Setter. Sets @see propertyDim to @param dim
         */
        void setPropertyDim (const std::string& dim);

        /*!
         * Populate \verbatim<UL:Property>\endverbatim element with
         * this \verbatim<FixedValue>\endverbatim
         *
         * @param the_doc The document for allocation of new nodes/attributes.
         *
         * @param into_node The node into which the UL:Property will
         * be written. This will be a Configuration node.
         */
        void writeULProperty (rapidxml::xml_document<>* the_doc,
                              rapidxml::xml_node<>* into_node);

    protected:
        /*!
         * Write out the fixed values as an explicit binary file.
         *
         * @param into_node The (e.g.) FixedValue node, which will be
         * renamed as a ValueList node.
         *
         * @param model_root The root path of the model
         *
         * @param binary_file_name The file name of the binary file
         * into which the actual value list data will be written.
         */
        void writeVLBinary (rapidxml::xml_node<>* into_node,
                            const std::string& model_root,
                            const std::string& binary_file_name);

        /*!
         * Write out the actual data to the binary file stream, which
         * will have been opened by @see writeVLBinary
         *
         * This is expected to be implemented in a derived class.
         *
         * The format for the data is: unsigned int index, double
         * value for each member of the population.
         *
         * @param f The output stream to which the binary data should
         * be written.
         */
        virtual void writeVLBinaryData (std::ostream& f) = 0;

        /*!
         * Re-writes the ConnectionList node's XML, in preparation for
         * writing out the connection list as an explicit binary file.
         *
         * @param into_node The (e.g.) FixedValue node, which will be
         * renamed as a ValueList node.
         *
         * @param model_root The root path of the model
         *
         * @param binary_file_name The file name of the binary file
         * into which the actual value list data will be written.
         */
        void writeVLXml (rapidxml::xml_node<>* into_node,
                         const std::string& model_root,
                         const std::string& binary_file_name);

        /*!
         * The content-specific write property value thing.
         *
         * Populates the Property node @param into_node with a (e.g.)
         * FixedValue XML node. Uses @param the_doc to allocate member
         * for the new node and its attributes.
         *
         * This will be implemented in a child class, not in
         * PropertyContent.
         */
        virtual void writeULPropertyValue (rapidxml::xml_document<>* the_doc,
                                           rapidxml::xml_node<>* into_node);

        /*!
         * Set to true if this Property is already a binary ValueList.
         */
        bool alreadyBinary;

        /*!
         * The "name" attribute of the enclosing property.
         */
        std::string propertyName;

        /*!
         * The "dimension" attribute of the enclosing property.
         */
        std::string propertyDim;

    public:
        /*!
         * The number of neurons for the population to which the
         * property pertains. Used when writing out as value list.
         */
        unsigned int numInPopulation;
    };

} // namespace

#endif // _PROPERTYCONTENT_H_
