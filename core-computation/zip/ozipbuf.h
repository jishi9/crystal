//------------------------------------------------------------------------------
// (c) 2001 Gottfried Chen
//------------------------------------------------------------------------------

#ifndef OZIPBUF_H
#define OZIPBUF_H

#include "ozipfile.h"
#include <streambuf>

namespace zip
{

template<class Character, class Traits = std::char_traits<Character> >
class basic_ozipbuf : public std::basic_streambuf<Character, Traits>
{
    typedef typename Traits::int_type int_type;
    typedef typename Traits::char_type char_type;
    typedef Traits traits_type;
public:
    enum
    {
        BUFFER_SIZE = 100
    };

    basic_ozipbuf();
    // Open <fileName> in <zip>
    basic_ozipbuf(ozipfile& zip, const char* fileName,
        std::ios_base::openmode = std::ios_base::out | std::ios_base::trunc);

    virtual ~basic_ozipbuf();

    // Open <fileName> in <zip>. Returns <this> if succesful otherwise 0.
    basic_ozipbuf* open(ozipfile& zip, const char* fileName,
                        std::ios_base::openmode);
    // If is_open, close the file and return <this>, otherwise 0.
    basic_ozipbuf* close();
    // Returns false, if the file couldn't be opened
    bool is_open();


protected:
    virtual int_type overflow(int_type c = traits_type::eof());
    virtual int sync();
    // ToDo: implement
    //virtual std::streamsize xsputn(char_type* buffer, std::streamsize size);


private:
    // Translates '\n' to "\r\n"
    static inline
    unsigned int toFileMode(char* buffer, unsigned int size);

    basic_ozipbuf(const basic_ozipbuf&);
    basic_ozipbuf& operator=(const basic_ozipbuf&);

    bool flushWriteBuffer(int_type);

    bool mOpen;
    // Need double size for worst case text mode translation
    char_type mWriteBuffer[2*BUFFER_SIZE];
    bool mBinary;
    ozipfile* mFile;
};

typedef basic_ozipbuf<char> ozipbuf;

// Attention: No wide to multibyte character translation is done on output.
typedef basic_ozipbuf<wchar_t> wozipbuf;


} // namespace zip

#include "ozipbuf.inl"

#endif // OZIPBUF_H