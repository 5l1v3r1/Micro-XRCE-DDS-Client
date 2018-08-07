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

#ifndef _MICRORTPS_CLIENT_SESSION_H_
#define _MICRORTPS_CLIENT_SESSION_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <micrortps/client/session/session_info.h>
#include <micrortps/client/session/stream_storage.h>
#include <micrortps/client/communication/communication.h>

typedef struct Session
{
    SessionInfo info;
    StreamStorage streams;
    Communication* comm;
    int last_request_id;

} Session;

int create_session(Session* session, uint8_t session_id, const char* key, Communication* comm); //TODO
int delete_session(Session* session); //TODO
void run_session(Session* session, size_t max_attemps, uint32_t poll_ms);
int generate_request_id(Session* session); //TODO

StreamId create_output_best_effort_stream(Session* session, uint8_t* buffer, size_t size);
StreamId create_output_reliable_stream(Session* session, uint8_t* buffer, size_t size, size_t message_size);
StreamId create_input_best_effort_stream(Session* session);
StreamId create_input_reliable_stream(Session* session, uint8_t* buffer, size_t size);

#ifdef PROFILE_STATUS_ANSWER
void read_status_submessage(Session* session, MicroBuffer* submessage, StreamId id); //TODO
#endif

#ifdef PROFILE_DATA_ACCESS
void read_data_submessage(Session* session, MicroBuffer* submessage, StreamId id, uint8_t flags);
#endif

#ifdef __cplusplus
}
#endif

#endif // _MICRORTPS_CLIENT_SESSION_H
