/*!
 * A class to record information about delay changes requested by the
 * user (and validated and inserted into the experiment layer xml).
 *
 * Author: Seb James
 * Date: Nov 2016
 */

#ifndef _DELAYCHANGE_H_
#define _DELAYCHANGE_H_

#include <string>
#include <sstream>

namespace spineml
{
    enum DelayChangeType {
        DELAYCHANGE_UNSET,
        DELAYCHANGE_PROJECTION,
        DELAYCHANGE_GENERIC,
        DELAYCHANGE_N
    };

    /*!
     * A class to represent a delay change. This single class covers
     * both the SpineML delay change types "ProjectionDelayChange" and
     * "GenericInputDelayChange". The DelayChangeType is used to
     * distinguish which one is which.

     */
    class DelayChange
    {
    public:
        DelayChange() { this->type = DELAYCHANGE_UNSET; }
        DelayChange(DelayChangeType t) { this->type = t; }
        ~DelayChange() {};

        void setSynapseNumber (std::string& sn) {
            std::stringstream ss;
            ss << sn;
            ss >> this->synapseNumber;
        }

        void setDelay (std::string& del) {
            std::stringstream ss;
            ss << del;
            ss >> this->delay;
        }

        /*!
         * Does the candidate information for source population,
         * destination population and synapse number match this
         * DelayChange?
         */
        bool matches (const std::string& candSrc,
                      const std::string& candDst,
                      const std::string& candSynNum) const {
            // 3 args mean that this must be a projection delay change.
            if (this->type != DELAYCHANGE_PROJECTION) {
                return false;
            }
            if (this->src != candSrc || this->dst != candDst) {
                return false;
            }
            std::stringstream ss;
            ss << candSynNum;
            unsigned int csn = 0;
            ss >> csn;
            if (this->synapseNumber != csn) {
                return false;
            }
            return true;
        }

        /*!
         * Does the candidate information for source population,
         * destination population and src/dst port match this
         * DelayChange?
         */
        bool matches (const std::string& candSrc,
                      const std::string& candSrcPort,
                      const std::string& candDst,
                      const std::string& candDstPort) const {
            // 4 args mean that this must be a generic input delay change.
            if (this->type != DELAYCHANGE_GENERIC) {
                return false;
            }
            if (this->src != candSrc || this->dst != candDst) {
                return false;
            }
            if (this->srcPort != candSrcPort || this->dstPort != candDstPort) {
                return false;
            }
            return true;
        }

    private:
        DelayChangeType type;

    public:
        //! Used for both projection and generic:
        //@{
        std::string src;
        std::string dst;
        float delay;
        //@}

        //! Used for generic:
        //@{
        std::string srcPort;
        std::string dstPort;
        //@}

        //! Used for projection:
        //@{
        unsigned int synapseNumber;
        //@}
    };

} // namespace spineml

#endif // _DELAYCHANGE_H_
