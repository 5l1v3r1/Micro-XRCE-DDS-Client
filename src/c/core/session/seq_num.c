#include <micrortps/client/core/session/seq_num.h>

#define UINT16_SIZE (1 << 16)
#define UINT16_MIDSIZE (1 << 15)

//==================================================================
//                             PUBLIC
//==================================================================
SeqNum seq_num_add(SeqNum seq_num, SeqNum increment)
{
    return (seq_num + increment) % UINT16_SIZE;
}

SeqNum seq_num_sub(SeqNum seq_num, SeqNum decrement)
{
    return (decrement > seq_num)
        ? (seq_num + (UINT16_SIZE - decrement)) % UINT16_SIZE
        : seq_num - decrement;
}

int seq_num_cmp(SeqNum seq_num_1, SeqNum seq_num_2)
{
    int result;
    if(seq_num_1 == seq_num_2)
    {
        result = 0;
    }
    else if((seq_num_1 < seq_num_2 && (seq_num_2 - seq_num_1) < UINT16_MIDSIZE) ||
            (seq_num_1 > seq_num_2 && (seq_num_1 - seq_num_2) > UINT16_MIDSIZE))
    {
        result = -1;
    }
    else
    {
        result = 1;
    }
    return result;
}
