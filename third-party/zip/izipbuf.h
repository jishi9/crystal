//------------------------------------------------------------------------------
// (c) 2001 Gottfried Chen
//------------------------------------------------------------------------------

#ifndef IZIPBUF_H
#define IZIPBUF_H

#include "izipfile.h"
#include <streambuf>

namespace zip
{

// ATTENTION: Only one concurrent open file possible -> synchronize via a static variable

template<class Character, class Traits = std::char_traits<Character> >
class basic_izipbuf : public std::basic_streambuf<Character, Traits>
{
    typedef typename Traits::int_type int_type;
    typedef typename Traits::char_type char_type;
    typedef Traits traits_type;
public:
    enum
    {
        BUFFER_SIZE = 100,
        PUT_BACK_SIZE = 10
    };

    basic_izipbuf();
    // Open <fileName> in <zip>
    basic_izipbuf(izipfile& zip, const char* fileName,
        std::ios_base::openmode);

    virtual ~basic_izipbuf();

    // Open <fileName> in <zip>. Returns <this> if succesful otherwise 0.
    basic_izipbuf* open(izipfile& zip, const char* fileName,
                        std::ios_base::openmode);
    // If is_open, close the file and return <this>, otherwise 0.
    basic_izipbuf* close();
    // Returns false, if the file couldn't be opened
    bool is_open();

    const izipfile& getZipFile() const;

protected:
    virtual int_type pbackfail(int_type character);
    virtual int_type underflow();
    // ToDo: Optimize it for binary files.
    virtual std::streamsize xsgetn(char_type* buffer, std::streamsize size);

private:
    // Translates "\r\n" to '\n'
    static inline
    unsigned int toTextMode(char* buffer, unsigned int size);

    basic_izipbuf(const basic_izipbuf&);
    basic_izipbuf& operator=(const basic_izipbuf&);

    int fillReadBuffer();

    bool mOpen;
    izipfile* mFile;
    char_type mReadBuffer[BUFFER_SIZE];
    bool mBinary;
};

typedef basic_izipbuf<char> izipbuf;

// Attention: No multibyte to wide character translation is done on input.
typedef basic_izipbuf<wchar_t> wizipbuf;

} // namespace zip


#include "izipbuf.inl"
#endif // IZIPBUF_H