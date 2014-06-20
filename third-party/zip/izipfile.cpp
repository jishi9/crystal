//------------------------------------------------------------------------------
// (c) 2001 Gottfried Chen
//------------------------------------------------------------------------------

#include "izipfile.h"

namespace zip
{

//------------------------------------------------------------------------------
izipfile::izipfile(const char* fileName) :
//------------------------------------------------------------------------------
mHandle(0),
mFileInZipIsOpen(false)
{
    mHandle = unzOpen(fileName);
}

//------------------------------------------------------------------------------
bool izipfile::isOk() const
//------------------------------------------------------------------------------
{
    return mHandle != 0;
}

//------------------------------------------------------------------------------
izipfile::~izipfile()
//------------------------------------------------------------------------------
{
    close();
}

//------------------------------------------------------------------------------
bool izipfile::close()
//------------------------------------------------------------------------------
{
    bool ret = false;
    closeInZip();
    if (isOk())
    {
        ret = unzClose(mHandle) == UNZ_OK;
        mHandle = 0;
    }

    return ret;
}

//------------------------------------------------------------------------------
bool izipfile::fileInZipIsOpen() const
//------------------------------------------------------------------------------
{
    return mFileInZipIsOpen;
}

//------------------------------------------------------------------------------
bool izipfile::openInZip(const char* fileName)
//------------------------------------------------------------------------------
{
    bool ret = false;
    if (isOk() &&
        !fileInZipIsOpen() &&
        unzLocateFile(mHandle, fileName, 0) == UNZ_OK &&
        unzOpenCurrentFile(mHandle) == UNZ_OK)
    {
        mFileInZipIsOpen = true;
        ret = true;
    }

    return ret;
}

//------------------------------------------------------------------------------
bool izipfile::closeInZip()
//------------------------------------------------------------------------------
{
    bool ret = false;
    if (mFileInZipIsOpen)
    {
        mFileInZipIsOpen = false;
        ret = unzCloseCurrentFile(mHandle) == UNZ_OK;
    }
    return ret;
}

//------------------------------------------------------------------------------
int izipfile::read(void* buffer, unsigned int size)
//------------------------------------------------------------------------------
{
    if (fileInZipIsOpen())
    {
        return unzReadCurrentFile(mHandle, buffer, size);
    }
    else
    {
        return -1;
    }
}

//------------------------------------------------------------------------------
izipfile::izipfile(izipfile& rhs)
//------------------------------------------------------------------------------
{
    *this = rhs;
}

//------------------------------------------------------------------------------
izipfile& izipfile::operator=(izipfile& rhs)
//------------------------------------------------------------------------------
{
    mHandle = rhs.mHandle;
    mFileInZipIsOpen = rhs.mFileInZipIsOpen;

    rhs.mHandle = 0;
    rhs.mFileInZipIsOpen = false;

    return *this;
}

//------------------------------------------------------------------------------
int izipfile::getCurrentFileSize() const
//------------------------------------------------------------------------------
{
    if (fileInZipIsOpen())
    {
        unz_file_info info;
        if (UNZ_OK == unzGetCurrentFileInfo(mHandle, &info, 0, 0, 0, 0, 0, 0))
        {
            return info.uncompressed_size;
        }
    }

    return -1;
}

} // namespace zip