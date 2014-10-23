/*
 * A small class to allocate space for some text and to then read it
 * from a file.
 */

#ifndef _ALLOCANDREAD_H_
#define _ALLOCANDREAD_H_

#include <string>
#include <sstream>
#include <fstream>
#include <stdexcept>
#include <stdlib.h>
#include "rapidxml.hpp"

namespace s2b
{
    /*!
     * Allocate storage and read in the data from the file at
     * filepath.
     */
    class AllocAndRead {
    public:
        AllocAndRead (const std::string& path)
            : filepath (path)
            , data_((char*)0)
        {
            this->read();
        }
        ~AllocAndRead()
        {
            if (this->data_) {
                free (this->data_);
            }
        }

        /*!
         * Get a pointer to the data.
         */
        char* data (void)
        {
            return this->data_;
        }

    private:
        /*!
         * Read the file, allocating memory as required.
         */
        void read (void)
        {
            std::ifstream f;
            f.open (this->filepath.c_str(), std::ios::in);
            if (!f.is_open()) {
                std::stringstream ee;
                ee << "AllocAndRead: Failed to open file " << this->filepath << " for reading";
                throw std::runtime_error (ee.str());
            }

            // Work out how much memory to allocate - seek to the end
            // of the file and find its size.
            f.seekg (0, std::ios::end);
            size_t sz = f.tellg();
            f.seekg (0);
            this->data_ = static_cast<char*>(calloc (++sz, sizeof(char))); // ++ for trailing null

            char* textpos = this->data_;
            std::string line("");
            size_t curmem = 0; // current allocated memory count (chars)
            size_t llen = 0;   // line length (chars)
            size_t curpos = 0;
            while (getline (f, line)) {
                // Restore the newline
                line += "\n";
                // Get line length
                llen = line.size();
                // Restore textpos pointer in the reallocated memory:
                textpos = this->data_ + curpos;
                // copy line to textpos:
                strncpy (textpos, line.c_str(), llen);
                // Update current character position
                curpos += llen;
            }
            f.close();
            // Note: text is already null terminated as we used calloc.
        }

        //! The path from which to read data.
        std::string filepath;

        //! The character data.
        char* data_;
    };

} // namespace s2b

#endif // _ALLOCANDREAD_H_
