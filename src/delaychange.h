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
     * A class to represent a delay change
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
