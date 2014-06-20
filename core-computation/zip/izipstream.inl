//------------------------------------------------------------------------------
// (c) 2001 Gottfried Chen
//------------------------------------------------------------------------------
namespace zip
{

template<class C, class T>
//------------------------------------------------------------------------------
basic_izipstream<C, T>::basic_izipstream(izipfile& zip,
                                         const char* fileName,
                                         std::ios_base::openmode mode) :
//------------------------------------------------------------------------------
std::basic_istream<C, T>(0)
{
    open(zip, fileName, mode);
}

template<class C, class T>
//------------------------------------------------------------------------------
void basic_izipstream<C, T>::open(izipfile& zip, const char* fileName,
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
void basic_izipstream<C, T>::close()
//------------------------------------------------------------------------------
{
    mBuffer.close();
    this->init(0);
}

template<class C, class T>
//------------------------------------------------------------------------------
int basic_izipstream<C, T>::getSize() const
//------------------------------------------------------------------------------
{
    return mBuffer.getZipFile().getCurrentFileSize();
}

} // namespace zip