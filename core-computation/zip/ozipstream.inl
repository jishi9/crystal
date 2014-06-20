//------------------------------------------------------------------------------
// (c) 2001 Gottfried Chen
//------------------------------------------------------------------------------

namespace zip
{

template<class C, class T>
//------------------------------------------------------------------------------
basic_ozipstream<C, T>::basic_ozipstream(ozipfile& zip,
                                         const char* fileName,
                                         std::ios_base::openmode mode) :
//------------------------------------------------------------------------------
std::basic_ostream<C, T>(0)
{
    open(zip, fileName, mode);
}

template<class C, class T>
//------------------------------------------------------------------------------
void basic_ozipstream<C, T>::open(ozipfile& zip, const char* fileName,
                                  std::ios_base::openmode mode)
//------------------------------------------------------------------------------
{
    close();

    mBuffer.open(zip, fileName, mode);
    if (mBuffer.is_open())
        this->init(&mBuffer);
    else
        this->setstate(std::ios_base::failbit);
}

template<class C, class T>
//------------------------------------------------------------------------------
void basic_ozipstream<C, T>::close()
//------------------------------------------------------------------------------
{
    mBuffer.close();
    this->init(0);
}


} // namespace zip