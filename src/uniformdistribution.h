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

    protected:
        /*!
         * Write out the fixed values as an explicit binary file.
         */
        void writeVLBinaryData (std::ostream& f);

    public:
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
