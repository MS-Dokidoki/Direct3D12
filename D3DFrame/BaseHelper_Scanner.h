#pragma once
#include "Base.h"

namespace BaseHelper
{
    class ScannerA
    {
    public:
        ScannerA();
        ScannerA(const ScannerA&);
        ScannerA(LPCSTR lpszBuffer);
        ScannerA(LPSTR lpszBuffer);

        ScannerA& operator=(LPCSTR lpszBuffer);
        ScannerA& operator=(LPSTR lpszBuffer);
        ScannerA& operator=(const ScannerA& );

        ScannerA& operator>>(char&);
        ScannerA& operator>>(float&);
        ScannerA& operator>>(std::string&);
        ScannerA& operator>>(std::wstring&);
        
        ScannerA& operator>>(INT8&);
        ScannerA& operator>>(INT16&);
        ScannerA& operator>>(INT32&);
        ScannerA& operator>>(INT64&);
        ScannerA& operator>>(UINT8&);
        ScannerA& operator>>(UINT16&);
        ScannerA& operator>>(UINT32&);
        ScannerA& operator>>(UINT64&);

        ScannerA& operator()(char);

    private:
        LPSTR lpszBuffer = NULL;
        LPSTR lpScanner = NULL;

        UINT CodePage = CP_ACP;
    };

    class ScannerW
    {
        ScannerW();
        ScannerW(const ScannerW&);
        ScannerW(LPCWSTR lpszBuffer);
        ScannerW(LPWSTR lpszBuffer);

        ScannerW& operator=(LPCWSTR lpszBuffer);
        ScannerW& operator=(LPWSTR lpszBuffer);
        ScannerW& operator=(const ScannerW& );

        ScannerW& operator>>(WCHAR& c);
        ScannerW& operator>>(float& fnum);
        ScannerW& operator>>(std::string& str);
        ScannerW& operator>>(std::wstring& wstr);

        ScannerW& operator>>(INT8&);
        ScannerW& operator>>(INT16&);
        ScannerW& operator>>(INT32&);
        ScannerW& operator>>(INT64&);
        ScannerW& operator>>(UINT8&);
        ScannerW& operator>>(UINT16&);
        ScannerW& operator>>(UINT32&);
        ScannerW& operator>>(UINT64&);

        ScannerW& operator()(WCHAR);
    private:
        LPWSTR lpszBuffer = NULL;
        LPWSTR lpScanner = NULL;

        UINT CodePage = CP_ACP;
    };
};