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

#ifndef _UXR_CLIENT_CORE_SESSION_STREAM_OUTPUT_RELIABLE_STREAM_H_
#define _UXR_CLIENT_CORE_SESSION_STREAM_OUTPUT_RELIABLE_STREAM_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <uxr/client/core/session/stream/seq_num.h>

#include <stddef.h>
#include <stdbool.h>

struct ucdrStream;
struct uxrOutputReliableStream;

typedef void (*OnNewFragment)(struct ucdrStream* us, struct uxrOutputReliableStream* stream);

typedef struct uxrOutputReliableStream
{
    uint8_t* buffer;
    size_t size;
    uint16_t history;
    uint8_t offset;

    uxrSeqNum last_written;
    uxrSeqNum last_sent;
    uxrSeqNum last_acknown;

    int64_t next_heartbeat_timestamp;
    uint8_t next_heartbeat_tries;
    bool send_lost;

    OnNewFragment on_new_fragment;

} uxrOutputReliableStream;

#ifdef __cplusplus
}
#endif

#endif // _UXR_CLIENT_CORE_SESSION_STREAM_OUTPUT_RELIABLE_STREAM_H_
