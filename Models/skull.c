#include <windows.h>
#include <stdio.h>
#define _DEBUG
#define IS_DIGIT(c) (c >= '0'? (c <= '9'? 1: 0): 0)

typedef union Vertex
{
    float v[6];
    struct
    {
        float Position[3];
        float Normal[3];
    };
}Vertex;

int main()
{
    HANDLE hFile;
    DWORD dwByteRead;
    UINT iFileLength;
    char* pFileBuffer;
    char* pBegin, *pEnd, *pBlock;
    char pOutputBuffer[32] = {0};

    UINT iVertexCounter = 0, iIndexCounter = 0, j;
    Vertex vertex[31076];
    UINT16 indices[60339];

//**** Read File
    if((hFile = CreateFileA("skull.txt", GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL)) == INVALID_HANDLE_VALUE)
        return MessageBoxA(NULL, "Load Skull.txt failed", "error", MB_OK);
    
    iFileLength = GetFileSize(hFile, NULL);

    pFileBuffer = malloc(iFileLength + 2);
    ReadFile(hFile, pFileBuffer, iFileLength, NULL, NULL);
    CloseHandle(hFile);
    pFileBuffer[iFileLength] = '\0';
    pFileBuffer[iFileLength + 1] = '\0';

//*** Encoding File
    pBlock = pFileBuffer;

    // vertex
    while(*pBlock++ != '{');

    while(*pBlock != '}' || *pBlock)
    {
        while(!((*pBlock >= '0' && *pBlock <= '9') || *pBlock == '-')){++pBlock;}
        // 一组默认有 6 个浮点数
        for(j = 0; j < 6; ++j)
        {
            pBegin = pBlock;
            while(((*pBlock++ != ' ') && (*pBlock != '\r')));

            pEnd = pBlock - 1;      // ' '
            *pEnd = '\0';

            vertex[iVertexCounter].v[j] = atof(pBegin);
        }
        ++iVertexCounter;
        if(iVertexCounter == 31076)
            break;
    }

    // indices
    while(*pBlock++ != '{');

    while(*pBlock)
    {
        while(!((*pBlock >= '0' && *pBlock <= '9'))){++pBlock;}

        pBegin = pBlock;
        while(((*pBlock != ' ') && (*pBlock != '\r'))){++pBlock;}
        pEnd = pBlock++;      // ' '
        *pEnd = '\0';

        indices[iIndexCounter++] = atoi(pBegin);
        
        if(iIndexCounter == 60339)
            break;
    }
    free(pFileBuffer);
    return 0;
}