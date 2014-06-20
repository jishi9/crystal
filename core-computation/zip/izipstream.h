//------------------------------------------------------------------------------
// (c) 2001 Gottfried Chen
//------------------------------------------------------------------------------

#ifndef IZIPSTREAM_H
#define IZIPSTREAM_H

#include "izipbuf.h"


namespace zip
{

template<class Character, class Traits = std::char_traits<Character> >
class basic_izipstream : public std::basic_istream<Character, Traits>
{
public:
    basic_izipstream(izipfile& zip, const char* fileName,
                     std::ios_base::openmode = std::ios_base::in);

    void open(izipfile& zip, const char* fileName,
              std::ios_base::openmode = std::ios_base::in);
    void close();

    // Returns the uncompressed size of the file or -1 if an error occurs.
    int getSize() const;

private:
    basic_izipbuf<Character, Traits> mBuffer;
};

typedef basic_izipstream<char> izipstream;

// Attention: No multibyte to wide character translation is done on input.
typedef basic_izipstream<wchar_t> wizipstream;

} // namespace zip

#include "izipstream.inl"
#endif // IZIPSTREAM_H