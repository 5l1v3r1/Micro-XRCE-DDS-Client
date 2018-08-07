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

#ifndef _MICRORTPS_CLIENT_INPUT_RELIABLE_STREAM_H_
#define _MICRORTPS_CLIENT_INPUT_RELIABLE_STREAM_H_

#ifdef __cplusplus
extern "C"
{
#endif


#include <stdint.h>
#include <stdbool.h>

typedef struct MicroBuffer MicroBuffer;

typedef struct InputReliableStream
{
    //FIFOStructureHere fifo;
    uint16_t last_handle;
    uint16_t last_announced;

} InputReliableStream;

void init_input_reliable_stream(InputReliableStream* stream, uint8_t* buffer);
bool receive_reliable_message(InputReliableStream* stream, uint16_t seq_num, MicroBuffer* mb);
bool next_input_reliable_buffer_available(InputReliableStream* stream, MicroBuffer* mb);

bool input_reliable_stream_must_confirm(InputReliableStream* stream);
int write_acknack(InputReliableStream* stream, MicroBuffer* mb);
void process_heartbeat(InputReliableStream* stream, uint16_t* first_seq_num, uint16_t last_seq_num);

#ifdef __cplusplus
}
#endif

#endif // _MICRORTPS_CLIENT_INPUT_RELIABLE_STREAM_H_
