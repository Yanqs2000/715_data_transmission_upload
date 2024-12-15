#include "CheckSum.h"
uint16_t calChecksum(SafeVector<uint8_t> &data, int size)
{
    uint16_t sum = 0;
    for(int i = 0; i < size; i ++)
    {
        sum += data[i];
    }

    return sum;
}


uint16_t calChecksum(std::vector<uint8_t> &data, int size)
{
    uint16_t sum = 0;
    for(int i = 0; i < size; i ++)
    {
        sum += data[i];
    }

    return sum;
}