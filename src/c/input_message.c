// Copyright 2017 Proyectos y Sistemas de Mantenimiento SL (eProsima).
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <micrortps/client/input_message.h>
#include "xrce_protocol_serialization.h"

#include <stdlib.h>

#ifndef NDEBUG
    #include <string.h>
    #include <stdio.h>
    #define ERROR_PRINT(fmt, ...) fprintf(stderr, fmt, ## __VA_ARGS__)
#else
    #define ERROR_PRINT(fmt, ...);
#endif

void init_input_message(InputMessage* message, InputMessageCallback callback, uint8_t* in_buffer, uint32_t in_buffer_size)
{
    init_micro_buffer(&message->reader, in_buffer, 0);

    message->callback = callback;
    message->buffer = in_buffer;
    message->buffer_size = in_buffer_size;
}

void parse_data_payload(InputMessage* message, DataFormat format, const BaseObjectReply* reply)
{
    MicroBuffer* reader = &message->reader;
    InputMessageCallback* callback = &message->callback;

    switch(format)
    {
        case FORMAT_DATA: ;
            SampleData data;
            deserialize_SampleData(reader, &data);
            if(callback->on_data_payload)
            {
                callback->on_data_payload(reply, &data, callback->args, reader->endianness);
            }
            break;

        case FORMAT_SAMPLE: ;
            Sample sample;
            deserialize_Sample(reader, &sample);
            if(callback->on_sample_payload)
            {
                callback->on_sample_payload(reply, &sample, callback->args, reader->endianness);
            }
            break;

        case FORMAT_DATA_SEQ: ;
            SampleDataSeq data_sequence;
            deserialize_SampleDataSeq(reader, &data_sequence);
            if(callback->on_data_sequence_payload)
            {
                callback->on_data_sequence_payload(reply, &data_sequence, callback->args, reader->endianness);
            }
            break;

        case FORMAT_SAMPLE_SEQ: ;
            SampleSeq sample_sequence;
            deserialize_SampleSeq(reader, &sample_sequence);
            if(callback->on_sample_sequence_payload)
            {
                callback->on_sample_sequence_payload(reply, &sample_sequence, callback->args, reader->endianness);
            }
            break;

        case FORMAT_PACKED_SAMPLES: ;
            PackedSamples packed_samples;
            deserialize_PackedSamples(reader, &packed_samples);
            if(callback->on_packed_samples_payload)
                callback->on_packed_samples_payload(reply, &packed_samples, callback->args, reader->endianness);
             break;

        default:
            break;
    }
}

int parse_submessage(InputMessage* message)
{
    MicroBuffer* reader = &message->reader;
    InputMessageCallback* callback = &message->callback;

    // All subsessages begin with a 4 bytes alignment.
    align_to(reader, 4);

    // Submessage message header.
    SubmessageHeader submessage_header;
    deserialize_SubmessageHeader(reader, &submessage_header);
    if(callback->on_submessage_header)
    {
        callback->on_submessage_header(&submessage_header, callback->args);
    }

    // Configure the reader for the specific message endianness
    reader->endianness = submessage_header.flags & FLAG_ENDIANNESS;

    // Submessage is not complete.
    if(reader->iterator + submessage_header.length > reader->final)
    {
        ERROR_PRINT("Parse message error: Submessage not complete.\n", 0);
        return 0;
    }

    // Parse payload and call the corresponding callback.
    switch(submessage_header.id)
    {
        case SUBMESSAGE_ID_STATUS:
        {
            STATUS_Payload payload;
            deserialize_STATUS_Payload(reader, &payload);
            if(callback->on_status_submessage)
                callback->on_status_submessage(&payload, callback->args);
        }
        break;

        case SUBMESSAGE_ID_INFO:
        {
            INFO_Payload payload;
            deserialize_INFO_Payload(reader, &payload);
            if(callback->on_info_submessage)
                callback->on_info_submessage(&payload, callback->args);
        }
        break;

        case SUBMESSAGE_ID_DATA:
        {
            BaseObjectReply data_reply;
            deserialize_BaseObjectReply(reader, &data_reply);
            if(callback->on_data_submessage)
            {
                DataFormat format = callback->on_data_submessage(&data_reply, callback->args);
                parse_data_payload(message, format, &data_reply);
            }
            else
            {
                // skip submessage.
                reader->iterator += submessage_header.length;
                reader->last_data_size = 0;
            }
        }
        break;

        default:
            ERROR_PRINT("Parse message error: Unknown submessage id 0x%02X\n", submessage_header.id);
            return 0;
    }

    return 1;
}

int parse_message(InputMessage* message, uint32_t length)
{
    InputMessageCallback* callback = &message->callback;

    // Init the reader
    MicroBuffer* reader = &message->reader;
    if(length <= message->buffer_size)
    {
        init_micro_buffer(reader, message->buffer, length);
    }
    else
    {
        ERROR_PRINT("Parse message error: Message length greater than input buffer.\n", 0);
        return 0;
    }

    // Parse message header.
    MessageHeader message_header;
    ClientKey key;
    deserialize_MessageHeader(reader, &message_header);
    if(message_header.session_id < 128)
    {
        deserialize_ClientKey(reader, &key);
    }

    // If the callback implementation fails, the message will not be parse
    if(callback->on_message_header &&
        !callback->on_message_header(&message_header, &key, callback->args))
    {
        ERROR_PRINT("Parse message error: Message header not undestood.\n", 0);
        return 0;
    }

    // Parse all submessages of the message
    do
    {
        if(!parse_submessage(message))
            return 0;
    }
    while(reader->iterator < reader->final); // while another submessage fix...

    return 1;
}
