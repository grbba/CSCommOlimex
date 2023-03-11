
#ifndef Pool_h
#define Pool_h

#include <bit.h>
#include <DCSIlog.h>

template <typename T, int S>
class Pool
{
private:
    T pool[S];
    T nil;
    int16_t use = 0; // int 16 i.e. no more than 16 elements in the pool
                     // every bit resperesnts if one entry is used or not

    T *_iAllocate()
    {
        for (int idx = 0; idx < S; idx++)
        {
            if (!checkBit(use, idx))
            {
                setBit(use, idx);
                // TRC(F("Allocated [%x]" CR), &pool[idx]);
                return &pool[idx];
            }
        }
        // no element in the pool is available
        return &nil;
    }

public:
    T *allocate()
    {
        return _iAllocate();
    };

    void release(T *obj)
    {
        for (int idx = 0; idx < S; idx++)
        {
            if (&pool[idx] == obj)
            {
                unsetBit(use, idx);
                // TRC(F("Released [%x]" CR), &pool[idx]);
                memset(obj, 0, sizeof(T));
                return;
            }
        }
        ERR(F("Object does not exist in the pool [%x]" CR), obj);
    };

    bool isNil(T *obj)
    {
        return (obj == &nil);
    }
};

#endif