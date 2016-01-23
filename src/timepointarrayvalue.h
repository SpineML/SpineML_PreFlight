/*!
 * A TimePointArrayValue element object.
 *
 * Author: Seb James
 * Date: January 2016
 */

#ifndef _TIMEPOINTARRAYVALUE_H_
#define _TIMEPOINTARRAYVALUE_H_

#include <ostream>
#include <string>
#include "rapidxml.hpp"

namespace spineml
{
    /*!
     * A class to represent a TimePointArrayValue element, such as this one:
     *
     * \verbatim
     *  <TimeVaryingArrayInput target="Sebtest1" port="I" name="I">
     *    <TimePointArrayValue index="0" array_time="0,100,300" array_value="0,2,0"/>
     *    <TimePointArrayValue index="1" array_time="0" array_value="0"/>
     *    <TimePointArrayValue index="2" array_time="0,200" array_value="0,1"/>
     *  </TimeVaryingArrayInput>
     *
     * (as compared to a TimePointValue:)
     *  <TimeVaryingInput target="Sebtest1" port="I" name="I">
     *    <TimePointValue time="0" value="-60"/>
     *  </TimeVaryingInput>
     * \endverbatim
     */
    class TimePointArrayValue
    {
    public:
        /*!
         * Construct a new TimePointArrayValue using the XML at @param tpav_node
         */
        TimePointArrayValue(rapidxml::xml_node<>* tpav_node);

        /*!
         * Construct an empty TimePointArrayValue.
         */
        TimePointArrayValue();

        /*!
         * Simple setter for @see array_value.
         */
        void setArrayValue (const std::string& vs);

        /*!
         * Simple setter for @see array_time.
         */
        void setArrayTime (const std::string& ts);

        /*!
         * Setter for @see index.
         */
        //@{
        void setIndex (const unsigned int& i);
        void setIndex (const std::string& s);
        //@}

        /*!
         * Populates the TimeVaryingInput node @param into_node with a
         * TimePointArrayValue XML node. Uses @param the_doc to allocate member
         * for the new node and its attributes.
         */
        void writeXML (rapidxml::xml_document<>* the_doc, rapidxml::xml_node<>* into_node);

    public:
        /*!
         * The index
         */
        unsigned int index;

        /*!
         * The value string - a comma separated list of values.
         */
        std::string array_value;

        /*!
         * The time string - a comma separated list of times.
         */
        std::string array_time;
    };

} // namespace

#endif // _TIMEPOINTVALUE_H_
