/*!
 * A FixedValue element object.
 *
 * Author: Seb James
 * Date: Nov 2014
 */

#ifndef _FIXEDVALUE_H_
#define _FIXEDVALUE_H_

#include <ostream>
#include "rapidxml.hpp"
#include "propertycontent.h"

namespace spineml
{
    /*!
     * A class to represent a FixedValue element, such as this one:
     *
     * \verbatim
     * <Property name="m" dimension="?">
     *   <FixedValue value="1"/>
     * </Property>
     * \endverbatim
     */
    class FixedValue : public PropertyContent
    {
    public:
        /*!
         * Construct a new FixedValue using the XML at @param fv_node
         * and the number in the population given by @param
         * num_in_pop
         */
        FixedValue(rapidxml::xml_node<>* fv_node, const unsigned int num_in_pop);

        /*!
         * Construct an empty FixedValue.
         */
        FixedValue();

        /*!
         * Simple setter for @see value.
         */
        void setValue (const double& v);

    protected:
        /*!
         * Writes the equivalent explicit binary data ValueList
         * binary data into the already-open filestream @param f
         */
        void writeVLBinaryData (std::ostream& f);

        /*!
         * Populates the Property node @param into_node with a
         * FixedValue XML node. Uses @param the_doc to allocate member
         * for the new node and its attributes.
         */
        void writeULPropertyValue (rapidxml::xml_document<>* the_doc,
                                   rapidxml::xml_node<>* into_node);

    public:
        /*!
         * The fixed value as a number.
         */
        double value;
    };

} // namespace

#endif // _FIXEDVALUE_H_
