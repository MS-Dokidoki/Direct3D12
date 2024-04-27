#include "BaseHelper_Scanner.h"
#include "BaseHelper_Memory.h"

using namespace BaseHelper;

ScannerA::ScannerA()
{}

ScannerA::ScannerA(const ScannerA& scanner)
{
    lpszBuffer = scanner.lpszBuffer;
    lpScanner = scanner.lpScanner;
}

ScannerA::ScannerA(LPSTR buffer)
{
    lpszBuffer = buffer;
    lpScanner = lpszBuffer;
}

ScannerA::ScannerA(LPCSTR buffer)
{
    ScannerA((LPSTR)buffer);
}

ScannerA& ScannerA::operator=(const ScannerA& scanner)
{
    lpszBuffer = scanner.lpszBuffer;
    lpScanner = scanner.lpScanner;

    return *this;
}

ScannerA& ScannerA::operator=(LPSTR buffer)
{
    lpszBuffer = buffer;
    lpScanner = lpszBuffer;

    return *this;
}

ScannerA& ScannerA::operator=(LPCSTR buffer)
{
    return operator=((LPSTR)buffer);
}

//***************************
// Scanner Functions
ScannerA& ScannerA::operator()(char cNext)
{
    while(*lpScanner++ != cNext);
    return *this;
}

ScannerA& ScannerA::operator>>(INT8& inum)
{
    INT32 i;
    operator>>(i);
    inum = (INT8)i;
    return *this;
}

ScannerA& ScannerA::operator>>(INT16& inum)
{
    INT32 i;
    operator>>(i);
    inum = (INT16)i;
    return *this;
}

ScannerA& ScannerA::operator>>(INT32& inum)
{
    LPSTR pBegin;
    char r;
    
    while(!(BASE_IsDigit(*lpScanner) ||
          (*lpScanner == '-' && BASE_IsDigit(*(lpScanner + 1))))) ++lpScanner;
    pBegin = lpScanner++;
    while(BASE_IsDigit(*lpScanner)) ++lpScanner;
    r = *lpScanner;
    *lpScanner = '\0';

    inum = atoi(pBegin);

    *lpScanner++ = r;
    return *this;
}

ScannerA& ScannerA::operator>>(INT64& inum)
{
    LPSTR pBegin;
    char r;
    
    while(!(BASE_IsDigit(*lpScanner) ||
          (*lpScanner == '-' && BASE_IsDigit(*(lpScanner + 1))))) ++lpScanner;
    pBegin = lpScanner++;
    while(BASE_IsDigit(*lpScanner)) ++lpScanner;
    r = *lpScanner;
    *lpScanner = '\0';

    inum = _atoi64(pBegin);

    *lpScanner++ = r;
    return *this;
}

ScannerA& ScannerA::operator>>(UINT8& uinum)
{
    INT32 i;
    operator>>(i);
    uinum = (UINT8)i;
    return *this;
}

ScannerA& ScannerA::operator>>(UINT16& uinum)
{
    INT32 i;
    operator>>(i);
    uinum = (UINT16)i;
    return *this;
}

ScannerA& ScannerA::operator>>(UINT32& uinum)
{
    INT32 i;
    operator>>(i);
    uinum = (UINT32)i;
    return *this;
}

ScannerA& ScannerA::operator>>(UINT64& uinum)
{
    INT64 i;
    operator>>(i);
    uinum = (UINT64)i;
    return *this;
}

ScannerA& ScannerA::operator>>(float& fnum)
{
    LPSTR pBegin;
    bool bPoint = 0;
    char r;
    
    while(!(BASE_IsDigit(*lpScanner) ||
          (*lpScanner == '-' && BASE_IsDigit(*(lpScanner + 1))))) ++lpScanner;
        
    pBegin = lpScanner++;
    while(1)
    {
        if(*lpScanner == '.' && !bPoint)
            bPoint = 1;
        else if(!BASE_IsDigit(*lpScanner) &&
           *lpScanner != 'E' && *lpScanner != 'e' &&
           *lpScanner != '+' && *lpScanner != '-')
            break;
        ++lpScanner;
    }
    r = *lpScanner;
    *lpScanner = '\0';
    
    fnum = atof(pBegin);

    *lpScanner++ = r;
    return *this;   
}

ScannerA& ScannerA::operator>>(std::string& str)
{
    LPSTR pBegin;
    char r;

    while(BASE_IsUnmeaning(*lpScanner))++lpScanner;
    pBegin = lpScanner++;
    while(!BASE_IsUnmeaning(*lpScanner))++lpScanner;
    r = *lpScanner;
    *lpScanner = '\0';
    
    str = pBegin;

    *lpScanner++ = r;

    return *this;
}

ScannerA& ScannerA::operator>>(std::wstring& wstr)
{
    std::string buffer;
    LPWSTR bufferW;
    UINT length;

    operator>>(buffer);
    length = buffer.size();

    bufferW = (LPWSTR)BASE_MALLOC(length * sizeof(WCHAR) + 2);
    bufferW[length] = L'\0';

    MultiByteToWideChar(CodePage, 0, buffer.c_str(), buffer.size(), bufferW, length * 2);

    wstr = bufferW;

    BASE_MFREE(bufferW);
    return *this;
}

ScannerA& ScannerA::operator>>(char& c)
{
    c = *lpScanner++;
    return *this;
}

/************************************/
// Wide

ScannerW::ScannerW()
{}

ScannerW::ScannerW(const ScannerW& scanner)
{
    lpszBuffer = scanner.lpszBuffer;
    lpScanner = scanner.lpScanner;
}

ScannerW::ScannerW(LPWSTR buffer)
{
    lpszBuffer = buffer;
    lpScanner = lpszBuffer;
}

ScannerW::ScannerW(LPCWSTR buffer)
{
    ScannerW((LPWSTR)buffer);
}

ScannerW& ScannerW::operator=(LPWSTR buffer)
{
    lpszBuffer = buffer;
    lpScanner = lpszBuffer;
    return *this;
}

ScannerW& ScannerW::operator=(const ScannerW& scanner)
{
    lpszBuffer = scanner.lpszBuffer;
    lpScanner = scanner.lpScanner;
    return *this;
}

ScannerW& ScannerW::operator=(LPCWSTR buffer)
{
    return operator=((LPWSTR)buffer);
}


/*******************/
// Scanner Functions
ScannerW& ScannerW::operator()(WCHAR cNext)
{
    while(*lpScanner++ != cNext);
    return *this;
}

ScannerW& ScannerW::operator>>(INT8& inum)
{
    INT32 i;
    operator>>(i);
    inum = (INT8)i;
    return *this;
}

ScannerW& ScannerW::operator>>(INT16& inum)
{
    INT32 i;
    operator>>(i);
    inum = (INT16)i;
    return *this;
}

ScannerW& ScannerW::operator>>(INT32& inum)
{
    LPWSTR pBegin;
    WCHAR r;

    while(!(BASE_IsDigitW(*lpScanner) ||
          (*lpScanner == L'-' && BASE_IsDigitW(*(lpScanner + 1))))) ++lpScanner;
    pBegin = lpScanner++;
    while(BASE_IsDigitW(*lpScanner)) ++lpScanner;
    r = *lpScanner;
    *lpScanner = L'\0';
    
    inum = _wtoi(pBegin);

    *lpScanner++ = r;
    return *this;
}

ScannerW& ScannerW::operator>>(INT64& inum)
{
    LPWSTR pBegin;
    WCHAR r;
    while(!(BASE_IsDigitW(*lpScanner) ||
          (*lpScanner == L'-' && BASE_IsDigitW(*(lpScanner + 1))))) ++lpScanner;
    pBegin = lpScanner++;
    while(BASE_IsDigitW(*lpScanner))++lpScanner;
    r = *lpScanner;
    *lpScanner = L'\0';

    inum = _wtoi64(pBegin);

    *lpScanner++ = r;
    return *this;
}

ScannerW& ScannerW::operator>>(UINT8& inum)
{
    INT32 i;
    operator>>(i);
    inum = (UINT8)i;
    return *this;
}

ScannerW& ScannerW::operator>>(UINT16& inum)
{
    INT32 i;
    operator>>(i);
    inum = (UINT16)i;
    return *this;
}

ScannerW& ScannerW::operator>>(UINT32& inum)
{
    INT32 i;
    operator>>(i);
    inum = (UINT32)i;
    return *this;
}

ScannerW& ScannerW::operator>>(UINT64& inum)
{
    INT64 i;
    operator>>(i);
    inum = (UINT64)i;
    return *this;
}
ScannerW& ScannerW::operator>>(float& fnum)
{
    LPWSTR pBegin;
    bool bPoint = 0;
    WCHAR r;
    
    while(!(BASE_IsDigitW(*lpScanner) ||
          (*lpScanner == L'-' && BASE_IsDigitW(*(lpScanner + 1))))) ++lpScanner;
    pBegin = lpScanner++;
    while(1)
    {
        if(*lpScanner == L'.' && !bPoint)
            bPoint = 1;
        else if(!BASE_IsDigitW(*lpScanner) &&
           *lpScanner != L'E' && *lpScanner != L'e' &&
           *lpScanner != L'+' && *lpScanner != L'-')
            break;
        ++lpScanner;
    }
    r = *lpScanner;
    *lpScanner = L'\0';

    fnum = _wtof(pBegin);

    *lpScanner++ = r;
    return *this;
}

ScannerW& ScannerW::operator>>(std::wstring& wstr)
{
    LPWSTR pBegin;
    WCHAR r;

    while(BASE_IsUnmeaningW(*lpScanner))++lpScanner;
    pBegin = lpScanner;
    while(!BASE_IsUnmeaningW(*lpScanner))++lpScanner;
    r = *lpScanner;
    *lpScanner = L'\0';
    
    wstr = pBegin;

    *lpScanner++ = r;

    return *this;
}

ScannerW& ScannerW::operator>>(std::string& str)
{
    std::wstring buffer;
    LPSTR bufferA;
    UINT length;

    operator>>(buffer);
    length = buffer.size();

    bufferA = (LPSTR)BASE_MALLOC(length + 1);
    bufferA[length] = '\0';
    
    WideCharToMultiByte(CodePage, 0, buffer.c_str(), buffer.size(), bufferA, length, 0, 0);

    str = bufferA;

    BASE_MFREE(bufferA);
    return *this;
}

ScannerW& ScannerW::operator>>(WCHAR& c)
{
    c = *lpScanner++;
    return *this;
}