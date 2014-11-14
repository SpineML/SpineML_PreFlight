/*!
 * A NormalDistribution element object.
 *
 * Author: Seb James
 * Date: Nov 2014
 */

#ifndef _NORMALDISTRIBUTION_H_
#define _NORMALDISTRIBUTION_H_

#include <ostream>
#include "rapidxml.hpp"
#include "propertycontent.h"

namespace spineml
{
    /*!
     * A class to represent a NormalDistribution element, such as this one:
     *
     * \verbatim
     * <Property name="m" dimension="?">
     *   <Normaldistribution mean="1.1" variance="1.4" seed="123"/>
     * </Property>
     * \endverbatim
     */
    class NormalDistribution : public PropertyContent
    {
    public:
        /*!
         * Construct a new NormalDistribution using the XML at @param
         * nd_node and the number in the population given by @param
         * num_in_pop
         */
        NormalDistribution(rapidxml::xml_node<>* nd_node, const unsigned int num_in_pop);

    protected:
        /*!
         * Write out the fixed values as an explicit binary file.
         */
        void writeVLBinaryData (std::ostream& f);

    public:
        /*!
         * The mean of the Gaussian
         */
        double mean;

        /*!
         * The variance of the Gaussian
         */
        double variance;

        /*!
         * The RNG seed.
         */
        unsigned int seed;
    };

} // namespace

#endif // _NORMALDISTRIBUTION_H_
