#include "micrortps/client/mini_cdr.h"

#include <string.h>

void init_serialized_buffer(SerializedBufferHandle* buffer, uint8_t* buffer_input, uint32_t size)
{
    //if endian_mode contains 1 => LITTLE_ENDIAN, if contains 0 => BIG_ENDIAN
    uint32_t local = 1;
    buffer->endian_machine = *((uint8_t*) &local);
    buffer->endian_mode = LITTLE_ENDIAN_MODE;
    buffer->data = buffer_input;
    buffer->final = buffer_input + size;
    buffer->iterator = buffer_input;
}

void reset_buffer_iterator(SerializedBufferHandle* buffer)
{
    buffer->iterator = buffer->data;
}

void serialize_byte(SerializedBufferHandle* buffer, uint8_t byte)
{
    *buffer->iterator = byte;
    buffer->iterator++;
}

void serialize_byte_2(SerializedBufferHandle* buffer, uint16_t bytes)
{
    uint8_t* bytes_pointer = (uint8_t*)&bytes;
    if(buffer->endian_machine == buffer->endian_mode)
        *((uint16_t*)buffer->iterator) = bytes;
    else
    {
        *buffer->iterator = *(bytes_pointer + 1);
        *(buffer->iterator + 1) = *bytes_pointer;
    }

    buffer->iterator += sizeof(uint16_t);
}

void serialize_byte_2_endian(SerializedBufferHandle* buffer, uint16_t bytes, uint8_t endian)
{
    uint8_t* bytes_pointer = (uint8_t*)&bytes;
    if(buffer->endian_machine == endian)
        *((uint16_t*)buffer->iterator) = bytes;
    else
    {
        *buffer->iterator = *(bytes_pointer + 1);
        *(buffer->iterator + 1) = *bytes_pointer;
    }

    buffer->iterator += sizeof(uint16_t);
}

void serialize_byte_4(SerializedBufferHandle* buffer, uint32_t bytes)
{
    uint8_t* bytes_pointer = (uint8_t*)&bytes;
    if(buffer->endian_machine == buffer->endian_mode)
        *((uint32_t*)buffer->iterator) = bytes;
    else
    {
        *buffer->iterator = *(bytes_pointer + 3);
        *(buffer->iterator + 1) = *(bytes_pointer + 2);
        *(buffer->iterator + 2) = *(bytes_pointer + 1);
        *(buffer->iterator + 3) = *bytes_pointer;
    }

    buffer->iterator += sizeof(uint32_t);
}

void serialize_byte_4_endian(SerializedBufferHandle* buffer, uint32_t bytes, uint8_t endian)
{
    uint8_t* bytes_pointer = (uint8_t*)&bytes;
    if(buffer->endian_machine == endian)
        *((uint32_t*)buffer->iterator) = bytes;
    else
    {
        *buffer->iterator = *(bytes_pointer + 3);
        *(buffer->iterator + 1) = *(bytes_pointer + 2);
        *(buffer->iterator + 2) = *(bytes_pointer + 1);
        *(buffer->iterator + 3) = *bytes_pointer;
    }

    buffer->iterator += sizeof(uint32_t);
}

void serialize_byte_8(SerializedBufferHandle* buffer, uint64_t bytes)
{
    uint8_t* bytes_pointer = (uint8_t*)&bytes;
    if(buffer->endian_machine == buffer->endian_mode)
        *((uint64_t*)buffer->iterator) = bytes;
    else
    {
        *buffer->iterator = *(bytes_pointer + 7);
        *(buffer->iterator + 1) = *(bytes_pointer + 6);
        *(buffer->iterator + 2) = *(bytes_pointer + 5);
        *(buffer->iterator + 3) = *(bytes_pointer + 4);
        *(buffer->iterator + 4) = *(bytes_pointer + 3);
        *(buffer->iterator + 5) = *(bytes_pointer + 2);
        *(buffer->iterator + 6) = *(bytes_pointer + 1);
        *(buffer->iterator + 7) = *bytes_pointer;
    }

    buffer->iterator += sizeof(uint64_t);
}

void serialize_byte_8_endian(SerializedBufferHandle* buffer, uint64_t bytes, uint8_t endian)
{
    uint8_t* bytes_pointer = (uint8_t*)&bytes;
    if(buffer->endian_machine == endian)
        *((uint64_t*)buffer->iterator) = bytes;
    else
    {
        *buffer->iterator = *(bytes_pointer + 7);
        *(buffer->iterator + 1) = *(bytes_pointer + 6);
        *(buffer->iterator + 2) = *(bytes_pointer + 5);
        *(buffer->iterator + 3) = *(bytes_pointer + 4);
        *(buffer->iterator + 4) = *(bytes_pointer + 3);
        *(buffer->iterator + 5) = *(bytes_pointer + 2);
        *(buffer->iterator + 6) = *(bytes_pointer + 1);
        *(buffer->iterator + 7) = *bytes_pointer;
    }

    buffer->iterator += sizeof(uint64_t);
}


void serialize_array(SerializedBufferHandle* buffer, const uint8_t* array, uint32_t size)
{
    memcpy(buffer->iterator, array, size);
    buffer->iterator += size;
}


void deserialize_byte(SerializedBufferHandle* buffer, uint8_t* byte)
{
    if(buffer->iterator >= buffer->final)
        return;

    *byte = *buffer->iterator;
    buffer->iterator++;
}

void deserialize_byte_2(SerializedBufferHandle* buffer, uint16_t* bytes)
{
    if(buffer->iterator + sizeof(uint16_t) > buffer->final)
        return;

    uint8_t* bytes_pointer = (uint8_t*)bytes;
    if(buffer->endian_machine == buffer->endian_mode)
        *bytes = *(uint16_t*)buffer->iterator;
    else
    {
        *bytes_pointer = *(buffer->iterator + 1);
        *(bytes_pointer + 1) = *buffer->iterator;
    }

    buffer->iterator += sizeof(uint16_t);
}

void deserialize_byte_2_endian(SerializedBufferHandle* buffer, uint16_t* bytes, uint8_t endian)
{
    if(buffer->iterator + sizeof(uint16_t) > buffer->final)
        return;

    uint8_t* bytes_pointer = (uint8_t*)bytes;
    if(buffer->endian_machine == endian)
        *bytes = *(uint16_t*)buffer->iterator;
    else
    {
        *bytes_pointer = *(buffer->iterator + 1);
        *(bytes_pointer + 1) = *buffer->iterator;
    }

    buffer->iterator += sizeof(uint16_t);
}

void deserialize_byte_4(SerializedBufferHandle* buffer, uint32_t* bytes)
{
    if(buffer->iterator + sizeof(uint32_t) > buffer->final)
        return;

    uint8_t* bytes_pointer = (uint8_t*)bytes;
    if(buffer->endian_machine == buffer->endian_mode)
        *bytes = *(uint32_t*)buffer->iterator;
    else
    {
        *bytes_pointer = *(buffer->iterator + 3);
        *(bytes_pointer + 1) = *(buffer->iterator + 2);
        *(bytes_pointer + 2) = *(buffer->iterator + 1);
        *(bytes_pointer + 3) = *buffer->iterator;
    }

    buffer->iterator += sizeof(uint32_t);
}

void deserialize_byte_4_endian(SerializedBufferHandle* buffer, uint32_t* bytes, uint8_t endian)
{
    if(buffer->iterator + sizeof(uint32_t) > buffer->final)
        return;

    uint8_t* bytes_pointer = (uint8_t*)bytes;
    if(buffer->endian_machine == endian)
        *bytes = *(uint32_t*)buffer->iterator;
    else
    {
        *bytes_pointer = *(buffer->iterator + 3);
        *(bytes_pointer + 1) = *(buffer->iterator + 2);
        *(bytes_pointer + 2) = *(buffer->iterator + 1);
        *(bytes_pointer + 3) = *buffer->iterator;
    }

    buffer->iterator += sizeof(uint32_t);
}

void deserialize_byte_8(SerializedBufferHandle* buffer, uint64_t* bytes)
{
    if(buffer->iterator + sizeof(uint64_t) > buffer->final)
        return;

    uint8_t* bytes_pointer = (uint8_t*)bytes;
    if(buffer->endian_machine == buffer->endian_mode)
        *bytes = *(uint64_t*)buffer->iterator;
    else
    {
        *bytes_pointer = *(buffer->iterator + 7);
        *(bytes_pointer + 1) = *(buffer->iterator + 6);
        *(bytes_pointer + 2) = *(buffer->iterator + 5);
        *(bytes_pointer + 3) = *(buffer->iterator + 4);
        *(bytes_pointer + 4) = *(buffer->iterator + 3);
        *(bytes_pointer + 5) = *(buffer->iterator + 2);
        *(bytes_pointer + 6) = *(buffer->iterator + 1);
        *(bytes_pointer + 7) = *buffer->iterator;
    }

    buffer->iterator += sizeof(uint64_t);
}

void deserialize_byte_8_endian(SerializedBufferHandle* buffer, uint64_t* bytes, uint8_t endian)
{
    if(buffer->iterator + sizeof(uint64_t) > buffer->final)
        return;

    uint8_t* bytes_pointer = (uint8_t*)bytes;
    if(buffer->endian_machine == endian)
        *bytes = *(uint64_t*)buffer->iterator;
    else
    {
        *bytes_pointer = *(buffer->iterator + 7);
        *(bytes_pointer + 1) = *(buffer->iterator + 6);
        *(bytes_pointer + 2) = *(buffer->iterator + 5);
        *(bytes_pointer + 3) = *(buffer->iterator + 4);
        *(bytes_pointer + 4) = *(buffer->iterator + 3);
        *(bytes_pointer + 5) = *(buffer->iterator + 2);
        *(bytes_pointer + 6) = *(buffer->iterator + 1);
        *(bytes_pointer + 7) = *buffer->iterator;
    }

    buffer->iterator += sizeof(uint64_t);
}

void deserialize_array(SerializedBufferHandle* buffer, uint8_t* array, uint32_t size)
{
    if(buffer->iterator + size > buffer->final)
        return;

    memcpy(array, buffer->iterator, size);
    buffer->iterator += size;
}

void align_to(SerializedBufferHandle* buffer, uint32_t alignment)
{
    int offset = (buffer->iterator - buffer->data) % alignment;
    if(offset)
        buffer->iterator += alignment - offset;
}
