//------------------------------------------------------------------------------
// (c) 2001 Gottfried Chen
//------------------------------------------------------------------------------

#include "ozipfile.h"
#include <time.h>

namespace zip
{

//------------------------------------------------------------------------------
ozipfile::ozipfile(const char* fileName) :
//------------------------------------------------------------------------------
mFileInZipIsOpen(false)
{
    mHandle = zipOpen(fileName, 0);
}

//------------------------------------------------------------------------------
ozipfile::~ozipfile()
//------------------------------------------------------------------------------
{
    close();
}

//------------------------------------------------------------------------------
bool ozipfile::close()
//------------------------------------------------------------------------------
{
    bool ret = false;
    closeInZip();
    if (isOk())
    {
        ret = zipClose(mHandle, 0) == ZIP_OK;
        mHandle = 0;
    }

    return ret;
}

//------------------------------------------------------------------------------
bool ozipfile::isOk() const
//------------------------------------------------------------------------------
{
    return mHandle != 0;
}

//------------------------------------------------------------------------------
bool ozipfile::openInZip(const char* fileName)
//------------------------------------------------------------------------------
{
    bool ret = false;
    if (isOk() && !mFileInZipIsOpen)
    {
        // The file attributes were found out - by reading the attributes of
        // existing .zip files - to be 0x0 for internal and
        // 0x20 for external attributes. (These are the normal settings for
        // a read/write, visible file in a .zip)
        zip_fileinfo info = { 0 };
        info.external_fa = 0x20;

        // Set creation time.
        time_t sec;
        time(&sec);
        tm* local = localtime(&sec);
        info.tmz_date.tm_sec = local->tm_sec;
        info.tmz_date.tm_min = local->tm_min;
        info.tmz_date.tm_hour = local->tm_hour;
        info.tmz_date.tm_mday = local->tm_mday;
        info.tmz_date.tm_mon = local->tm_mon;
        info.tmz_date.tm_year = local->tm_year;

        if (zipOpenNewFileInZip(mHandle, fileName, &info,
                                0, 0, 0, 0, 0,
                                Z_DEFLATED, Z_DEFAULT_COMPRESSION) == ZIP_OK)
        {
            mFileInZipIsOpen = true;
            ret = true;
        }
    }
    
    return ret;
}

//------------------------------------------------------------------------------
bool ozipfile::closeInZip()
//------------------------------------------------------------------------------
{
    bool ret = false;
    if (mFileInZipIsOpen)
    {
        mFileInZipIsOpen = false;
        ret = zipCloseFileInZip(mHandle) == ZIP_OK;
    }

    return ret;
}

//------------------------------------------------------------------------------
bool ozipfile::write(void* buffer, unsigned int size)
//------------------------------------------------------------------------------
{
    bool ret = false;
    if (mFileInZipIsOpen)
    {
        ret = zipWriteInFileInZip(mHandle,
                                  static_cast<char*>(buffer), size) == ZIP_OK;
    }

    return ret;
}

//------------------------------------------------------------------------------
ozipfile::ozipfile(ozipfile& rhs)
//------------------------------------------------------------------------------
{
    *this = rhs;
}

//------------------------------------------------------------------------------
ozipfile& ozipfile::operator=(ozipfile& rhs)
//------------------------------------------------------------------------------
{
    mHandle = rhs.mHandle;
    mFileInZipIsOpen = rhs.mFileInZipIsOpen;

    rhs.mHandle = 0;
    rhs.mFileInZipIsOpen = false;

    return *this;
}

} // namespace zip
