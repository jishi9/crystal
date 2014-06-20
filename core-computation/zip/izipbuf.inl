//------------------------------------------------------------------------------
// (c) 2001 Gottfried Chen
//------------------------------------------------------------------------------

#include <algorithm>
#include <cstring>
#include <iostream>

namespace zip
{

template<class Character, class Traits>
//------------------------------------------------------------------------------
basic_izipbuf<Character, Traits>::basic_izipbuf(izipfile& zip,
    const char* fileName, std::ios_base::openmode mode)
//------------------------------------------------------------------------------
{
    this->setg(0, 0, 0);
    if (open(zip, fileName, mode) != 0)
        fillReadBuffer();
}

template<class C, class T>
//------------------------------------------------------------------------------
basic_izipbuf<C, T>::basic_izipbuf() :
//------------------------------------------------------------------------------
mOpen(false),
mFile(0),
mBinary(false)
{
    this->setg(0, 0, 0);
}

template<class Character, class Traits>
//------------------------------------------------------------------------------
basic_izipbuf<Character, Traits>::~basic_izipbuf()
//------------------------------------------------------------------------------
{
    close();
}

template<class C, class T>
//------------------------------------------------------------------------------
basic_izipbuf<C, T>* basic_izipbuf<C, T>::close()
//------------------------------------------------------------------------------
{
    if (is_open() && mFile->closeInZip())
    {
        mOpen = false;
        return this;
    }
    else {
        return 0;
    }
}


template<class C, class T>
//------------------------------------------------------------------------------
basic_izipbuf<C, T>* basic_izipbuf<C, T>::open(izipfile& zip,
    const char* fileName, std::ios_base::openmode mode)
//------------------------------------------------------------------------------
{
    mFile = &zip;
    mBinary = (mode & std::ios_base::binary) != 0;
    if (mFile->isOk() && mFile->openInZip(fileName))
    {
        mOpen = true;
    }
    else
    {
        mOpen = false;
        this->setg(0, 0, 0);
    }

    return mOpen ? this : 0;
}



template<class C, class T>
//------------------------------------------------------------------------------
bool basic_izipbuf<C, T>::is_open()
//------------------------------------------------------------------------------
{
    return mOpen;
}



template<class Character, class Traits>
//------------------------------------------------------------------------------
typename basic_izipbuf<Character, Traits>::int_type basic_izipbuf<Character, Traits>::underflow()
//------------------------------------------------------------------------------
{
    if (fillReadBuffer() > 0)
        return this->sgetc();
    else
        return traits_type::eof();
}

template<class Character, class Traits>
//------------------------------------------------------------------------------
std::streamsize basic_izipbuf<Character, Traits>::xsgetn(char_type* buffer, std::streamsize size)
//------------------------------------------------------------------------------
{
    // This can be optimized for binary reading, so that reading
    // is not done via the read buffer.
    std::streamsize readCount(0);
    if (is_open())
    {
        using namespace std;
        readCount = basic_streambuf<Character, Traits>::xsgetn(buffer, size);

        // The implementation below basically is the default implementation.
        /*
        while (size > 0)
        {
            if (!in_avail())
            {
                if (underflow() == traits_type::eof())
                    break;
            }

            int avail = in_avail();
            if (avail > size)
                avail = size;

            memcpy(buffer+readCount, this->gptr(), avail);
            gbump(avail);

            readCount += avail;
            size -= avail;
        }
        */
    }

    return readCount;
}

template<class Character, class Traits>
//------------------------------------------------------------------------------
int basic_izipbuf<Character, Traits>::fillReadBuffer()
//------------------------------------------------------------------------------
{
    typedef unsigned int unsigned_int;
    if (is_open())
    {
        unsigned int putBackSize = 0;
        if (this->eback() != 0)
        {
            putBackSize =
#ifdef _WIN32
            std::_MIN
#else
            std::min
#endif
                (unsigned_int(PUT_BACK_SIZE), unsigned_int(this->gptr()-this->eback()));

            memmove(mReadBuffer, this->gptr()-putBackSize, putBackSize);
        }

        char_type* begin = mReadBuffer+putBackSize;

        int readBytes = mFile->read(begin, (BUFFER_SIZE-putBackSize)*sizeof(char_type));

        if (readBytes > 0)
        {
            if (!mBinary)
                readBytes = toTextMode(begin, readBytes);

            this->setg(begin, begin, begin + readBytes);
        }

        return readBytes;
    }
    else
        return -1;
}

template<class C, class T>
//------------------------------------------------------------------------------
typename basic_izipbuf<C, T>::int_type
basic_izipbuf<C, T>::pbackfail(basic_izipbuf<C, T>::int_type character)
//------------------------------------------------------------------------------
{
    if (this->eback() != this->gptr())
    {
        this->gbump(-1);
        if (!traits_type::eq_int_type(traits_type::eof(), character))
        {
            *(this->gptr()) = traits_type::to_char_type(character);
        }

        return traits_type::not_eof(character);
    }
    else
        return traits_type::eof();
}

template<class Character, class Traits> inline
//------------------------------------------------------------------------------
unsigned int basic_izipbuf<Character, Traits>::toTextMode(char* buffer, unsigned int size)
//------------------------------------------------------------------------------
{
    unsigned int newSize(size);

#ifdef _WIN32
    char* source = buffer;
    char* destination = buffer;
    char* end = buffer+size;

    while (source < end)
    {
        if (*source == '\r' && *(source+1) == '\n')
        {
            ++source;
            --newSize;
        }

        if (destination != source)
            *destination = *source;

        ++destination;
        ++source;
    }
#endif // _WIN32

    return newSize;
}

template<class Character, class Traits> inline
//------------------------------------------------------------------------------
const izipfile& basic_izipbuf<Character, Traits>::getZipFile() const
//------------------------------------------------------------------------------
{
    return *mFile;
}

} // namespace zip