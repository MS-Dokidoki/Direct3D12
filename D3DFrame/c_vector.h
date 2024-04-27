#ifndef _C_VECTOR_H
#define _C_VECTOR_H
#include "c_base.h"

#ifndef VECTOR_INIT_MAX_SIZE 
#define VECTOR_INIT_MAX_SIZE 2
#endif

typedef HANDLE HVECTOR;

UINT VectorStructSize();
BOOL VectorIsVaild(HVECTOR);
UINT VectorCapacity(HVECTOR);
UINT VectorSize(HVECTOR);
UINT VectorType(HVECTOR);
UNDEF_TYPE VectorAt(HVECTOR, UINT index);

HVECTOR VectorCreate(UINT cbType);
BOOL VectorPushBack(HVECTOR, UNDEF_TYPE);
BOOL VectorPushConstantBack(HVECTOR, UNDEF_TYPE);
BOOL VectorPopBack(HVECTOR);

BOOL VectorInsert(HVECTOR, UINT iBegin, UINT count, UNDEF_TYPE);
BOOL VectorRemove(HVECTOR, UINT iBegin, UINT count);
BOOL VectorResize(HVECTOR, UINT size);
BOOL VectorClear(HVECTOR);
BOOL VectorFree(HVECTOR);
BOOL VectorFreeEx(HVECTOR);

UNDEF_TYPE VectorFront(HVECTOR);
UNDEF_TYPE VectorBack(HVECTOR);

#define VectorCreateEx(type) VectorCreate(sizeof(type))
#define VectorPushBackEx(handle, element) VectorPushBack(handle, &element)
#define VectorIsEmpty(handle) (VectorSize(handle) == 0)

#define _VectorType_(type)
#define C_VECTOR(type) HVECTOR
#endif