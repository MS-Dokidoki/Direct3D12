#include "c_vector.h"
#define VECTOR_FLAG *(unsigned int*)"VECT"

#define VECTOR_AT(vector, index) ((void*)((ADDRESS)vector->Begin + vector->ElementTypeBytesSize * index))
typedef struct Vector
{
    UINT Flag;  // = *(unsigned int*)"VECT"
    UINT ElementsMaxNum;// 元素最大可用长度
    UINT ElementsNum;   // 元素有效长度    
    UINT ElementTypeBytesSize; // 元素类型
    void* Begin;
}VECTOR;

BOOL VectorIsVaild(HVECTOR hvector)
{
    VECTOR* vec = hvector;
    
    if(vec == NULL)
        return 0;
    if(IsBadReadPtr(vec, sizeof(VECTOR)))
        return 0;
    if(vec->Flag != VECTOR_FLAG)
        return 0;
    return 1;
}

/*************************/
/* Information functions */
/*************************/
UINT VectorStructSize()
{
    return sizeof(VECTOR);
}

UINT VectorCapacity(HVECTOR hvector)
{
    if(!VectorIsVaild(hvector))
        return 0;
    return ((VECTOR*)hvector)->ElementsMaxNum;   
}

UINT VectorSize(HVECTOR hvector)
{
    if(!VectorIsVaild(hvector))
        return 0;
    return ((VECTOR*)hvector)->ElementsNum;
}

UINT VectorType(HVECTOR hvector)
{
    if(!VectorIsVaild(hvector))
        return 0;
    return ((VECTOR*)hvector)->ElementTypeBytesSize;
}

UNDEF_TYPE VectorAt(HVECTOR hvector, UINT index)
{
    VECTOR* vector = hvector;
    if(!VectorIsVaild(hvector))
        return NULL;
    if(index >= vector->ElementsNum)
        return NULL;

    return (UNDEF_TYPE*)(VECTOR_AT(vector, index));
}

/******************************/
/*       Main Functions       */
/******************************/

HVECTOR VectorCreate(UINT size)
{
    VECTOR* vector = NULL;

    vector = malloc(sizeof(VECTOR));
    if(vector)
    {
        vector->ElementsNum = 0;
        vector->ElementsMaxNum = VECTOR_INIT_MAX_SIZE;
        vector->ElementTypeBytesSize = size;
        vector->Flag = VECTOR_FLAG;

        vector->Begin = malloc(vector->ElementsMaxNum * vector->ElementTypeBytesSize);
        if(!vector->Begin)
        {
            free(vector);
            vector = NULL;
        }
        ZeroMemory(vector->Begin, vector->ElementTypeBytesSize * vector->ElementsMaxNum);
    }
    return vector;
}

BOOL VectorExtension(VECTOR* vector)
{
    void* newBegin;
    UINT newSize;

    newSize = vector->ElementsMaxNum * 2;
    newBegin = malloc(newSize * vector->ElementTypeBytesSize);
    ZeroMemory(newBegin, newSize * vector->ElementTypeBytesSize);

    if(!newBegin)
        return 0;
    
    CopyMemory(newBegin, vector->Begin, vector->ElementsMaxNum * vector->ElementTypeBytesSize);
    free(vector->Begin);
    vector->Begin = newBegin;

    vector->ElementsMaxNum = newSize;
    return 1;
}

BOOL VectorPushBack(HVECTOR handle, UNDEF_TYPE element)
{
    VECTOR* vector = handle;
    if(!VectorIsVaild(handle))
        return 0;
    
    if(vector->ElementsNum + 1 >= vector->ElementsMaxNum)
        if(!VectorExtension(vector))
            return 0;
    
    CopyMemory(VECTOR_AT(vector, vector->ElementsNum++), element, vector->ElementTypeBytesSize);
    return 1;
}

BOOL VectorPushConstantBack(HVECTOR handle, UNDEF_TYPE element)
{
    VECTOR* vector = handle;
    if(!VectorIsVaild(handle))
        return 0;
    
    if(vector->ElementsNum + 1 >= vector->ElementsMaxNum)
        if(!VectorExtension(vector))
            return 0;
    
    CopyMemory(VECTOR_AT(vector, vector->ElementsNum++), &element, vector->ElementTypeBytesSize);
    return 1;
}

BOOL VectorPopBack(HVECTOR handle)
{
    VECTOR* vector = handle;
    if(!VectorIsVaild(handle))
        return 0;

    vector->ElementsNum = max(vector->ElementsNum - 1, 0);
    return 1;
}

BOOL VectorInsert(HVECTOR handle, UINT begin, UINT count, UNDEF_TYPE elements)
{
    VECTOR* vector = handle;
    if(!VectorIsVaild(handle))
        return 0;
    
    assert(begin <= vector->ElementsNum);

    if(vector->ElementsNum + count >= vector->ElementsMaxNum)
        do
        {
            if(!VectorExtension(vector))
                return 0;
        }while(vector->ElementsMaxNum < vector->ElementsNum + count);

    MoveMemory(VECTOR_AT(vector, begin + count), VECTOR_AT(vector, begin), (vector->ElementsNum - begin) * vector->ElementTypeBytesSize);
    CopyMemory(VECTOR_AT(vector, begin), elements, count * vector->ElementTypeBytesSize);

    vector->ElementsNum += count;
    return 1;
}

BOOL VectorRemove(HVECTOR handle, UINT begin, UINT count)
{
    VECTOR* vector = handle;
    if(!VectorIsVaild(handle))
        return 0;
    
    assert(begin <= vector->ElementsNum);

    if(begin + count >= vector->ElementsNum)
        count = vector->ElementsNum - begin;
    
    ZeroMemory(VECTOR_AT(vector, begin), count * vector->ElementTypeBytesSize);
    vector->ElementsNum -= count;

    return 1;
}

BOOL VectorResize(HVECTOR handle, UINT size)
{
    VECTOR* vector = handle;
    void* newBegin;
    if(!VectorIsVaild(handle))
        return 0;
    newBegin = malloc(size * vector->ElementTypeBytesSize);
    
    if(newBegin == NULL)
        return 0;
    ZeroMemory(newBegin, size * vector->ElementTypeBytesSize);
    free(vector->Begin);
    vector->Begin = newBegin;
    return 1;
}

BOOL VectorClear(HVECTOR handle)
{
    VECTOR* vector = handle;
    if(!VectorIsVaild(handle))
        return 0;
    ZeroMemory(vector->Begin, vector->ElementTypeBytesSize * vector->ElementsMaxNum);
    return 1;
}

BOOL VectorFree(HVECTOR handle)
{
    VECTOR* vector = handle;
    if(!VectorIsVaild(handle))
        return 0;
    
    free(vector->Begin);
    free(vector);
    return 1;
}

BOOL VectorFreeEx(HVECTOR handle)
{
    if(!VectorIsVaild(handle))
        return 0;
    free(handle);
    return 1;
}

UNDEF_TYPE VectorFront(HVECTOR handle)
{
    VECTOR* vector = handle;
    if(!VectorIsVaild(handle))
        return 0;
    return vector->Begin;
}

UNDEF_TYPE VectorBack(HVECTOR handle)
{
    VECTOR* vector = handle;
    if(!VectorIsVaild(handle))
        return 0;
    return VECTOR_AT(vector, vector->ElementsNum);
}
