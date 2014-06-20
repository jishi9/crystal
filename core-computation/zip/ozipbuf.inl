//------------------------------------------------------------------------------
// (c) 2001 Gottfried Chen
//------------------------------------------------------------------------------

#include <algorithm>
#include <streambuf>
#include <time.h>

namespace zip
{

template<class Character, class Traits>
//------------------------------------------------------------------------------
basic_ozipbuf<Character, Traits>::basic_ozipbuf(
    ozipfile& zip, const char* fileName, std::ios_base::openmode mode) :
//------------------------------------------------------------------------------
mOpen(false),
mBinary(false),
mFile(0)
{
    open(zip, fileName, mode);
}

template<class C, class T>
//------------------------------------------------------------------------------
basic_ozipbuf<C, T>::basic_ozipbuf() :
//------------------------------------------------------------------------------
mOpen(false),
mBinary(false),
mFile(0)
{
}


template<class Character, class Traits>
//------------------------------------------------------------------------------
basic_ozipbuf<Character, Traits>::~basic_ozipbuf()
//------------------------------------------------------------------------------
{
    close();
}


template<class C, class T>
//------------------------------------------------------------------------------
basic_ozipbuf<C, T>* basic_ozipbuf<C, T>::close()
//------------------------------------------------------------------------------
{
    if (is_open())
        sync();

    if (is_open() && mFile->closeInZip())
    {
        mOpen = false;
        return this;
    }
    else
        return 0;
}


template<class C, class T>
//------------------------------------------------------------------------------
basic_ozipbuf<C, T>* basic_ozipbuf<C, T>::open(ozipfile& zip,
    const char* fileName, std::ios_base::openmode mode)
//------------------------------------------------------------------------------
{
    mFile = &zip;
    mBinary = (mode & std::ios_base::binary) != 0;
    if (mFile->isOk() && mFile->openInZip(fileName))
    {
        mOpen = true;
        this->setp(mWriteBuffer, mWriteBuffer + BUFFER_SIZE);
    }
    else
    {
        mOpen = false;
        this->setp(0, 0);
    }

    return mOpen ? this : 0;
}



template<class C, class T>
//------------------------------------------------------------------------------
bool basic_ozipbuf<C, T>::is_open()
//------------------------------------------------------------------------------
{
    return mOpen;
}



template<class Character, class Traits>
//------------------------------------------------------------------------------
int basic_ozipbuf<Character, Traits>::sync()
//------------------------------------------------------------------------------
{
    return flushWriteBuffer(traits_type::eof()) ? 0 : -1;
}

template<class C, class T>
//------------------------------------------------------------------------------
typename basic_ozipbuf<C, T>::int_type basic_ozipbuf<C, T>::overflow(basic_ozipbuf<C, T>::int_type c)
//------------------------------------------------------------------------------
{
    if (flushWriteBuffer(c))
        return traits_type::not_eof(c);
    else
        return traits_type::eof();
}


template<class Character, class Traits>
//------------------------------------------------------------------------------
bool basic_ozipbuf<Character, Traits>::flushWriteBuffer(basic_ozipbuf<Character, Traits>::int_type c)
//------------------------------------------------------------------------------
{
    bool ret = true;
    if (is_open())
    {
        unsigned int putSize((this->pptr()-this->pbase())*sizeof(char_type));
        if (!mBinary)
            putSize = toFileMode(reinterpret_cast<char*>(this->pbase()), putSize);

        if (mFile->write(mWriteBuffer, putSize))
        {
            // Reset write pointers.
            this->setp(mWriteBuffer, mWriteBuffer + BUFFER_SIZE);
            // Put <c> into write buffer if c != eof.
            if (!traits_type::eq_int_type(c, traits_type::eof()))
                this->sputc(traits_type::to_char_type(c));
        }
        else
            ret = false;
    }
    else
        ret = false;

    return ret;
}

template<class Character, class Traits> inline
//------------------------------------------------------------------------------
unsigned int basic_ozipbuf<Character, Traits>::toFileMode(char* buffer, unsigned int size)
//------------------------------------------------------------------------------
{
    unsigned int newSize(size);

#ifdef _WIN32
    // Count number of \n
    unsigned int newLineCount(std::count(buffer, buffer+size, '\n'));
    newSize += newLineCount;

    char* source = buffer+size-1;
    char* destination = buffer+newSize-1;
    char* end = buffer-1;

    while (source > end)
    {
        if (*source == '\n')
        {
            *destination = '\n';
            --destination;
            *destination = '\r';
        }
        else
        {
            *destination = *source;
        }

        --source;
        --destination;
    }
#endif // _WIN32

    return newSize;
}

} // namespace zip