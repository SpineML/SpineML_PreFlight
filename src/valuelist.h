/*!
 * A ValueList element object.
 *
 * Author: Seb James
 * Date: Nov 2014
 */

#ifndef _VALUELIST_H_
#define _VALUELIST_H_

#include <ostream>
#include <map>
#include "rapidxml.hpp"
#include "propertycontent.h"

namespace spineml
{
    /*!
     * A class to represent a Valuelist element, such as this one:
     *
     * <Property name="m" dimension="?">
     *   <Valuelist>
     *     <Value />
     *     <Value />
     *   </ValueList>
     * </Property>
     */
    class ValueList : public PropertyContent
    {
    public:
        ValueList(rapidxml::xml_node<>* ud_node, const unsigned int num_in_pop);
        ~ValueList() {}

    protected:
        /*!
         * Write out the fixed values as an explicit binary file.
         */
        void writeVLBinaryData (std::ostream& f);

    public:
        /*!
         * The values stored in the (XML only) value list.
         */
        std::map<int, double> values;
    };

} // namespace

#endif // _VALUELIST_H_
