/*!
 * A UniformDistribution element object.
 *
 * Author: Seb James
 * Date: Nov 2014
 */

#ifndef _UNIFORMDISTRIBUTION_H_
#define _UNIFORMDISTRIBUTION_H_

#include <ostream>
#include "rapidxml.hpp"
#include "propertycontent.h"

namespace spineml
{
    /*!
     * A class to represent a UniformDistribution element, such as this one:
     *
     * \verbatim
     * <Property name="m" dimension="?">
     *   <Uniformdistribution minimum="1.1" maximum="1.4" seed="123"/>
     * </Property>
     * \endverbatim
     */
    class UniformDistribution : public PropertyContent
    {
    public:
        /*!
         * Construct a new UniformDistribution using the XML at @param
         * ud_node and the number in the population given by @param
         * num_in_pop
         */
        UniformDistribution(rapidxml::xml_node<>* ud_node, const unsigned int num_in_pop);

        /*!
         * Construct an empty UniformDistribution
         */
        UniformDistribution();

    protected:
        /*!
         * Write out the fixed values as an explicit binary file.
         */
        void writeVLBinaryData (std::ostream& f);

        /*!
         * Populates the Property node @param into_node with a
         * UniformDistribution XML node. Uses @param the_doc to
         * allocate member for the new node and its attributes.
         */
        void writeULPropertyValue (rapidxml::xml_document<>* the_doc,
                                   rapidxml::xml_node<>* into_node);

    public:
        /*!
         * Set minimum, maximum and seed from a string like
         * UNI(1,2,123)
         */
        void setFromString (const std::string& str);

        /*!
         * The minimum value for an output random number.
         */
        double minimum;

        /*!
         * The maximum value for an output random number.
         */
        double maximum;

        /*!
         * The RNG seed.
         */
        unsigned int seed;
    };

} // namespace

#endif // _UNIFORMDISTRIBUTION_H_
