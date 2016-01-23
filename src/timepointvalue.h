/*!
 * A TimePointValue element object.
 *
 * Author: Seb James
 * Date: June 2015
 */

#ifndef _TIMEPOINTVALUE_H_
#define _TIMEPOINTVALUE_H_

#include <ostream>
#include "rapidxml.hpp"

namespace spineml
{
    /*!
     * A class to represent a TimePointValue element, such as this one:
     *
     * \verbatim
     *  <TimeVaryingInput target="Sebtest1" port="I" name="I">
     *    <TimePointValue time="0" value="-60"/>
     *  </TimeVaryingInput>
     * \endverbatim
     */
    class TimePointValue
    {
    public:
        /*!
         * Construct a new TimePointValue using the XML at @param fv_node
         */
        TimePointValue(rapidxml::xml_node<>* tpv_node);

        /*!
         * Construct an empty TimePointValue.
         */
        TimePointValue();

        /*!
         * Simple setter for @see value.
         */
        void setValue (const double& v);

        /*!
         * Simple setter for @see time.
         */
        void setTime (const double& t);

        /*!
         * Populates the TimeVaryingInput node @param into_node with a
         * TimePointValue XML node. Uses @param the_doc to allocate member
         * for the new node and its attributes.
         */
        void writeXML (rapidxml::xml_document<>* the_doc, rapidxml::xml_node<>* into_node);

    public:
        /*!
         * The value as a number.
         */
        double value;

        /*!
         * The time as a number.
         */
        double time;
    };

} // namespace

#endif // _TIMEPOINTVALUE_H_
