//------------------------------------------------------------------------------
// (c) 2001 Gottfried Chen
//------------------------------------------------------------------------------

#ifndef OZIPFILE_H
#define OZIPFILE_H

#include "zip.h"

namespace zip
{

class ozipfile
{
public:
    // Con/Destruction
    ozipfile(const char* fileName);
    // Ownership is reassigned on copying. I.e.: <rhs> will be invalid afterwards.
    ozipfile(ozipfile& rhs);
    ozipfile& operator=(ozipfile& rhs);
    ~ozipfile();

    // Close the .zip file.
    bool close();
    // Returns true if the .zip is open and ok.
    bool isOk() const;

    // Open/Close a file in the .zip. Only one file can be open at
    // a time.
    bool openInZip(const char* fileName);
    bool closeInZip();
    
    // Returns false if an error occured.
    bool write(void* buffer, unsigned int size);

private:
    zipFile mHandle;
    bool mFileInZipIsOpen;
};

} // namespace zip

#endif // OZIPFILE_H
