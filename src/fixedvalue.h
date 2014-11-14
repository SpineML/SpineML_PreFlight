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
     * <Property name="m" dimension="?">
     *   <FixedValue value="1"/>
     * </Property>
     */
    class FixedValue : public PropertyContent
    {
    public:
        FixedValue(rapidxml::xml_node<>* fv_node, const unsigned int num_in_pop);
        FixedValue();

        void setValue (const double& v);

    protected:
        void writeVLBinaryData (std::ostream& f);
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
