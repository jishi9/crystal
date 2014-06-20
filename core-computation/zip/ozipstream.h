//------------------------------------------------------------------------------
// (c) 2001 Gottfried Chen
//------------------------------------------------------------------------------

#ifndef OZIPSTREAM_H
#define OZIPSTREAM_H

#include "ozipbuf.h"


namespace zip
{

template<class Character, class Traits = std::char_traits<Character> >
class basic_ozipstream : public std::basic_ostream<Character, Traits>
{
public:
    basic_ozipstream(ozipfile& zip, const char* fileName,
                     std::ios_base::openmode = std::ios_base::out | std::ios_base::trunc);

    void open(ozipfile& zip, const char* fileName,
              std::ios_base::openmode = std::ios_base::out | std::ios_base::trunc);
    void close();

private:
    basic_ozipbuf<Character, Traits> mBuffer;
};

typedef basic_ozipstream<char> ozipstream;

// Attention: No wide to multibyte character translation is done on output.
typedef basic_ozipstream<wchar_t> wozipstream;

} // namespace zip

#include "ozipstream.inl"
#endif // OZIPSTREAM_H