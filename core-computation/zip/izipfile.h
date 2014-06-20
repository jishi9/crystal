//------------------------------------------------------------------------------
// (c) 2001 Gottfried Chen
//------------------------------------------------------------------------------

#ifndef IZIPFILE_H
#define IZIPFILE_H

#include "unzip.h"

namespace zip
{

class izipfile
{
public:
    // Con/Destruction
    izipfile(const char* fileName);
    // Ownership is reassigned on copying. I.e.: <rhs> will be invalid afterwards.
    izipfile(izipfile& rhs);
    izipfile& operator=(izipfile& rhs);
    ~izipfile();

    // Close the .zip file.
    bool close();
    // Was opening the file succesful?
    bool isOk() const;

    // Open/Close a file in the .zip. Only one file can be open at
    // a time.
    bool openInZip(const char* fileName);
    bool closeInZip();

    // Returns the uncompressed size of the current open file in the .zip
    // or -1 if an error occurs.
    int getCurrentFileSize() const;

    // Returns number of read bytes or a negative number on error.
    int read(void* buffer, unsigned int size);

private:
    bool fileInZipIsOpen() const;

    unzFile mHandle;
    bool mFileInZipIsOpen;
};

} // namespace zip

#endif // IZIPFILE_H