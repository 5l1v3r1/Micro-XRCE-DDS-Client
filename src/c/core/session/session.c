#include <uxr/client/core/session/session.h>
#include <uxr/client/util/time.h>
#include <uxr/client/core/communication/communication.h>
#include <uxr/client/config.h>

#include "submessage_internal.h"
#include "session_internal.h"
#include "session_info_internal.h"
#include "stream/stream_storage_internal.h"
#include "stream/common_reliable_stream_internal.h"
#include "stream/input_best_effort_stream_internal.h"
#include "stream/input_reliable_stream_internal.h"
#include "stream/output_best_effort_stream_internal.h"
#include "stream/output_reliable_stream_internal.h"
#include "stream/seq_num_internal.h"
#include "../serialization/xrce_protocol_internal.h"
#include "../log/log_internal.h"
#include "../../util/time_internal.h"

#define CREATE_SESSION_MAX_MSG_SIZE (MAX_HEADER_SIZE + SUBHEADER_SIZE + CREATE_CLIENT_PAYLOAD_SIZE)
#define DELETE_SESSION_MAX_MSG_SIZE (MAX_HEADER_SIZE + SUBHEADER_SIZE + DELETE_CLIENT_PAYLOAD_SIZE)
#define HEARTBEAT_MAX_MSG_SIZE      (MAX_HEADER_SIZE + SUBHEADER_SIZE + HEARTBEAT_PAYLOAD_SIZE)
#define ACKNACK_MAX_MSG_SIZE        (MAX_HEADER_SIZE + SUBHEADER_SIZE + ACKNACK_PAYLOAD_SIZE)
#define TIMESTAMP_PAYLOAD_SIZE      8
#define TIMESTAMP_MAX_MSG_SIZE      (MAX_HEADER_SIZE + SUBHEADER_SIZE + TIMESTAMP_PAYLOAD_SIZE)

static bool listen_message(
        uxrSession* session,
        int poll_ms);

static bool listen_message_reliably(
        uxrSession* session,
        int poll_ms);

static bool wait_session_status(
        uxrSession* session,
        uint8_t* buffer,
        size_t length,
        size_t attempts);

static bool send_message(
        const uxrSession* session,
        uint8_t* buffer,
        size_t length);

static bool recv_message(
        const uxrSession* session,
        uint8_t** buffer,
        size_t* length,
        int poll_ms);

static void write_submessage_heartbeat(
        const uxrSession* session,
        uxrStreamId stream);

static void write_submessage_acknack(
        const uxrSession* session,
        uxrStreamId stream);

static void read_message(
        uxrSession* session,
        ucdrStream* message);

static void read_stream(
        uxrSession* session,
        ucdrStream* message,
        uxrStreamId id,
        uxrSeqNum seq_num);

static void read_submessage_list(
        uxrSession* session,
        ucdrStream* submessages,
        uxrStreamId stream_id);

static void read_submessage(
        uxrSession* session,
        ucdrStream* submessage,
        uint8_t submessage_id,
        uxrStreamId stream_id,
        uint16_t length,
        uint8_t flags);

static void read_submessage_status(
        uxrSession* session,
        ucdrStream* submessage);

static void read_submessage_data(
        uxrSession* session,
        ucdrStream* submessage,
        uint16_t length,
        uxrStreamId stream_id,
        uint8_t format);

static void read_submessage_heartbeat(
        uxrSession* session,
        ucdrStream* submessage);

static void read_submessage_acknack(
        uxrSession* session,
        ucdrStream* submessage);

static void read_submessage_timestamp_reply(
        uxrSession* session,
        ucdrStream* submessage);

#ifdef PERFORMANCE_TESTING
static void read_submessage_performance(
        uxrSession* session,
        ucdrStream* submessage,
        uint16_t length);
#endif

static void process_status(
        uxrSession* session,
        uxrObjectId object_id,
        uint16_t request_id,
        uint8_t status);

static void process_timestamp_reply(
        uxrSession* session,
        TIMESTAMP_REPLY_Payload* timestamp);

static bool run_session_until_sync(
        uxrSession* session,
        int timeout);

//==================================================================
//                             PUBLIC
//==================================================================

void uxr_init_session(
        uxrSession* session,
        uxrCommunication* comm,
        uint32_t key)
{
    session->comm = comm;

    session->request_list = NULL;
    session->status_list = NULL;
    session->request_status_list_size = 0;

    session->on_status = NULL;
    session->on_status_args = NULL;
    session->on_topic = NULL;
    session->on_topic_args = NULL;

    session->on_time = NULL;
    session->on_time_args = NULL;
    session->time_offset = 0;
    session->synchronized = false;

    uxr_init_session_info(&session->info, 0x81, key);
    uxr_init_stream_storage(&session->streams);
}

void uxr_set_status_callback(
        uxrSession* session,
        uxrOnStatusFunc on_status_func,
        void* args)
{
    session->on_status = on_status_func;
    session->on_status_args = args;
}

void uxr_set_topic_callback(
        uxrSession* session,
        uxrOnTopicFunc on_topic_func,
        void* args)
{
    session->on_topic = on_topic_func;
    session->on_topic_args = args;
}

void uxr_set_time_callback(
        uxrSession* session,
        uxrOnTimeFunc on_time_func,
        void* args)
{
    session->on_time = on_time_func;
    session->on_time_args = args;
}

#ifdef PERFORMANCE_TESTING
void uxr_set_performance_callback(uxrSession* session, uxrOnPerformanceFunc on_echo_func, void* args)
{
    session->on_performance = on_echo_func;
    session->on_performance_args = args;
}
#endif

bool uxr_create_session(
        uxrSession* session)
{
    uxr_reset_stream_storage(&session->streams);

    uint8_t buffer[CREATE_SESSION_MAX_MSG_SIZE];
    ucdrStream us;
    ucdr_init_stream(&us, buffer, sizeof(buffer));

    uxr_serialize_message_header(&us, session->info.id & SESSION_ID_WITHOUT_CLIENT_KEY, 0, 0, session->info.key);
    uxr_buffer_create_session(&session->info, &us, (uint16_t)(session->comm->mtu - INTERNAL_RELIABLE_BUFFER_OFFSET));

    bool received = wait_session_status(session, buffer, ucdr_used_size(&us), UXR_CONFIG_MAX_SESSION_CONNECTION_ATTEMPTS);
    bool created = received && (UXR_STATUS_OK == session->info.last_requested_status);

    return created;
}

bool uxr_delete_session(
        uxrSession* session)
{
    uint8_t buffer[DELETE_SESSION_MAX_MSG_SIZE];
    ucdrStream us;
    ucdr_init_stream(&us, buffer, sizeof(buffer));

    uxr_serialize_message_header(&us, session->info.id, 0, 0, session->info.key);
    uxr_buffer_delete_session(&session->info, &us);

    bool received = wait_session_status(session, buffer, ucdr_used_size(&us), UXR_CONFIG_MAX_SESSION_CONNECTION_ATTEMPTS);
    return received && (UXR_STATUS_OK == session->info.last_requested_status);
}

uxrStreamId uxr_create_output_best_effort_stream(
        uxrSession* session,
        uint8_t* buffer,
        size_t size)
{
    uint8_t header_offset = uxr_session_header_offset(&session->info);
    return uxr_add_output_best_effort_buffer(&session->streams, buffer, size, header_offset);
}

uxrStreamId uxr_create_output_reliable_stream(
        uxrSession* session,
        uint8_t* buffer,
        size_t max_message_size,
        size_t max_fragment_size,
        uint16_t history)
{
    uint8_t header_offset = uxr_session_header_offset(&session->info);
    return uxr_add_output_reliable_buffer(&session->streams, buffer, max_message_size, max_fragment_size, history, header_offset);
}

uxrStreamId uxr_create_input_best_effort_stream(
        uxrSession* session)
{
    return uxr_add_input_best_effort_buffer(&session->streams);
}

uxrStreamId uxr_create_input_reliable_stream(
        uxrSession* session,
        uint8_t* buffer,
        size_t max_message_size,
        size_t max_fragment_size,
        uint16_t history)
{
    return uxr_add_input_reliable_buffer(&session->streams, buffer, max_message_size, max_fragment_size, history);
}

bool uxr_run_session_time(
        uxrSession* session,
        int timeout_ms)
{
    uxr_flash_output_streams(session);

    bool timeout = false;
    while(!timeout)
    {
         timeout = !listen_message_reliably(session, timeout_ms);
    }

    return uxr_output_streams_confirmed(&session->streams);
}

bool uxr_run_session_until_timeout(
        uxrSession* session,
        int timeout_ms)
{
    uxr_flash_output_streams(session);

    return listen_message_reliably(session, timeout_ms);
}

bool uxr_run_session_until_confirm_delivery(
        uxrSession* session,
        int timeout_ms)
{
    uxr_flash_output_streams(session);

    bool timeout = false;
    while(!uxr_output_streams_confirmed(&session->streams) && !timeout)
    {
        timeout = !listen_message_reliably(session, timeout_ms);
    }

    return uxr_output_streams_confirmed(&session->streams);
}

bool uxr_run_session_until_all_status(
        uxrSession* session,
        int timeout_ms,
        const uint16_t* request_list,
        uint8_t* status_list,
        size_t list_size)
{
    uxr_flash_output_streams(session);

    for(unsigned i = 0; i < list_size; ++i)
    {
        status_list[i] = UXR_STATUS_NONE;
    }

    session->request_list = request_list;
    session->status_list = status_list;
    session->request_status_list_size = list_size;

    bool timeout = false;
    bool status_confirmed = false;
    while(!timeout && !status_confirmed)
    {
        timeout = !listen_message_reliably(session, timeout_ms);
        status_confirmed = true;
        for(unsigned i = 0; i < list_size && status_confirmed; ++i)
        {
            status_confirmed = status_list[i] != UXR_STATUS_NONE
                            || request_list[i] == UXR_INVALID_REQUEST_ID; //CHECK: better give an error? an assert?
        }
    }

    session->request_status_list_size = 0;

    bool status_ok = true;
    for(unsigned i = 0; i < list_size && status_ok; ++i)
    {
        status_ok = status_list[i] == UXR_STATUS_OK || status_list[i] == UXR_STATUS_OK_MATCHED;
    }

    return status_ok;
}

bool uxr_run_session_until_one_status(
        uxrSession* session,
        int timeout_ms,
        const uint16_t* request_list,
        uint8_t* status_list,
        size_t list_size)
{
    uxr_flash_output_streams(session);

    for(unsigned i = 0; i < list_size; ++i)
    {
        status_list[i] = UXR_STATUS_NONE;
    }

    session->request_list = request_list;
    session->status_list = status_list;
    session->request_status_list_size = list_size;

    bool timeout = false;
    bool status_confirmed = false;
    while(!timeout && !status_confirmed)
    {
        timeout = !listen_message_reliably(session, timeout_ms);
        for(unsigned i = 0; i < list_size && !status_confirmed; ++i)
        {
            status_confirmed = status_list[i] != UXR_STATUS_NONE
                            || request_list[i] == UXR_INVALID_REQUEST_ID; //CHECK: better give an error? an assert?
        }
    }

    session->request_status_list_size = 0;

    return status_confirmed;
}

bool uxr_sync_session(
        uxrSession* session,
        int time)
{
    uint8_t buffer[TIMESTAMP_MAX_MSG_SIZE];
    ucdrStream us;
    ucdr_init_stream(&us, buffer, sizeof(buffer));

    uxr_serialize_message_header(&us, session->info.id, 0, 0, session->info.key);
    uxr_buffer_submessage_header(&us, SUBMESSAGE_ID_TIMESTAMP, TIMESTAMP_PAYLOAD_SIZE, 0);

    TIMESTAMP_Payload timestamp;
    int64_t nanos = uxr_nanos();
    timestamp.transmit_timestamp.seconds = (int32_t)(nanos / 1000000000);
    timestamp.transmit_timestamp.nanoseconds = (uint32_t)(nanos % 1000000000);
    (void) uxr_serialize_TIMESTAMP_Payload(&us, &timestamp);

    return send_message(session, buffer, ucdr_used_size(&us)) && run_session_until_sync(session, time);
}

int64_t uxr_epoch_millis(
        uxrSession* session)
{
    return uxr_epoch_nanos(session) / 1000000;
}

int64_t uxr_epoch_nanos(
        uxrSession* session)
{
    return uxr_nanos() - session->time_offset;
}

#ifdef PERFORMANCE_TESTING
bool uxr_buffer_performance(
        uxrSession *session,
        uxrStreamId stream_id,
        uint64_t epoch_time,
        uint8_t* buf,
        uint16_t len,
        bool echo)
{
    bool rv = false;
    PERFORMANCE_Payload payload;
    payload.epoch_time_lsb = (uint32_t)(epoch_time & UINT32_MAX);
    payload.epoch_time_msb = (uint32_t)(epoch_time >> 32);
    payload.buf = buf;
    payload.len = len;
    ucdrStream mb;
    const uint16_t payload_length = (uint16_t)(sizeof(payload.epoch_time_lsb) +
                                               sizeof(payload.epoch_time_msb) +
                                               len);

    uint8_t flags = (echo) ? UXR_ECHO : 0;
    if(uxr_prepare_stream_to_write_submessage(session, stream_id, payload_length, &mb, SUBMESSAGE_ID_PERFORMANCE, flags))
    {
        (void) uxr_serialize_PERFORMANCE_Payload(&mb, &payload);
        rv = true;
    }
    return rv;
}
#endif

void uxr_flash_output_streams(
        uxrSession* session)
{
    for(uint8_t i = 0; i < session->streams.output_best_effort_size; ++i)
    {
        uxrOutputBestEffortStream* stream = &session->streams.output_best_effort[i];
        uxrStreamId id = uxr_stream_id(i, UXR_BEST_EFFORT_STREAM, UXR_OUTPUT_STREAM);

        uint8_t* buffer; size_t length; uxrSeqNum seq_num;
        if(uxr_prepare_best_effort_buffer_to_send(stream, &buffer, &length, &seq_num))
        {
            uxr_stamp_session_header(&session->info, id.raw, seq_num, buffer);
            send_message(session, buffer, length);
        }
    }

    for(uint8_t i = 0; i < session->streams.output_reliable_size; ++i)
    {
        uxrOutputReliableStream* stream = &session->streams.output_reliable[i];
        uxrStreamId id = uxr_stream_id(i, UXR_RELIABLE_STREAM, UXR_OUTPUT_STREAM);

        uint8_t* buffer; size_t length; uxrSeqNum seq_num;
        while(uxr_prepare_next_reliable_buffer_to_send(stream, &buffer, &length, &seq_num))
        {
            uxr_stamp_session_header(&session->info, id.raw, seq_num, buffer);
            send_message(session, buffer, length);
        }
    }
}

//==================================================================
//                             PRIVATE
//==================================================================
bool listen_message(
        uxrSession* session,
        int poll_ms)
{
    uint8_t* data; size_t length;
    bool must_be_read = recv_message(session, &data, &length, poll_ms);
    if(must_be_read)
    {
        ucdrStream us;
        ucdr_init_stream(&us, data, (uint32_t)length);
        read_message(session, &us);
    }

    return must_be_read;
}

bool listen_message_reliably(
        uxrSession* session,
        int poll_ms)
{
    bool received = false;
    int32_t poll = (poll_ms >= 0) ? poll_ms : INT32_MAX;
    do
    {
        int64_t next_heartbeat_timestamp = INT64_MAX;
        int64_t timestamp = uxr_millis();
        for(uint8_t i = 0; i < session->streams.output_reliable_size; ++i)
        {
            uxrOutputReliableStream* stream = &session->streams.output_reliable[i];
            uxrStreamId id = uxr_stream_id(i, UXR_RELIABLE_STREAM, UXR_OUTPUT_STREAM);

            if(uxr_update_output_stream_heartbeat_timestamp(stream, timestamp))
            {
                write_submessage_heartbeat(session, id);
            }

            if(stream->next_heartbeat_timestamp < next_heartbeat_timestamp)
            {
                next_heartbeat_timestamp = stream->next_heartbeat_timestamp;
            }
        }

        int32_t poll_to_next_heartbeat = (next_heartbeat_timestamp != INT64_MAX) ? (int32_t)(next_heartbeat_timestamp - timestamp) : poll;
        if(0 == poll_to_next_heartbeat)
        {
            poll_to_next_heartbeat = 1;
        }

        int poll_chosen = (poll_to_next_heartbeat < poll) ? poll_to_next_heartbeat : poll;
        received = listen_message(session, poll_chosen);
        if(!received)
        {
            poll -= poll_chosen;
        }
    }
    while(!received && poll > 0);

    return received;
}

bool wait_session_status(
        uxrSession* session,
        uint8_t* buffer,
        size_t length,
        size_t attempts)
{
    session->info.last_requested_status = UXR_STATUS_NONE;

    int poll_ms = UXR_CONFIG_MIN_SESSION_CONNECTION_INTERVAL;
    for(size_t i = 0; i < attempts && session->info.last_requested_status == UXR_STATUS_NONE; ++i)
    {
        send_message(session, buffer, length);
        poll_ms = listen_message(session, poll_ms) ? UXR_CONFIG_MIN_SESSION_CONNECTION_INTERVAL : poll_ms * 2;
    }

    return session->info.last_requested_status != UXR_STATUS_NONE;
}

inline bool send_message(
        const uxrSession* session,
        uint8_t* buffer,
        size_t length)
{
    bool sent = session->comm->send_msg(session->comm->instance, buffer, length);
    UXR_DEBUG_PRINT_MESSAGE((sent) ? UXR_SEND : UXR_ERROR_SEND, buffer, length, session->info.key);
    return sent;
}

inline bool recv_message(
        const uxrSession* session,
        uint8_t**buffer,
        size_t* length,
        int poll_ms)
{
    bool received = session->comm->recv_msg(session->comm->instance, buffer, length, poll_ms);
    if(received)
    {
        UXR_DEBUG_PRINT_MESSAGE(UXR_RECV, *buffer, *length, session->info.key);
    }
    return received;
}

void write_submessage_heartbeat(
        const uxrSession* session,
        uxrStreamId id)
{
    uint8_t buffer[HEARTBEAT_MAX_MSG_SIZE];
    ucdrStream us;
    ucdr_init_stream(&us, buffer, sizeof(buffer));

    const uxrOutputReliableStream* stream = &session->streams.output_reliable[id.index];

    uxr_serialize_message_header(&us, session->info.id, 0, 0, session->info.key);
    uxr_buffer_submessage_header(&us, SUBMESSAGE_ID_HEARTBEAT, HEARTBEAT_PAYLOAD_SIZE, 0);

    HEARTBEAT_Payload payload;
    payload.first_unacked_seq_nr = uxr_seq_num_add(stream->last_acknown, 1);
    payload.last_unacked_seq_nr = stream->last_sent;
    payload.stream_id = id.raw;
    (void) uxr_serialize_HEARTBEAT_Payload(&us, &payload);

    send_message(session, buffer, ucdr_used_size(&us));
}

void write_submessage_acknack(
        const uxrSession* session,
        uxrStreamId id)
{
    uint8_t buffer[ACKNACK_MAX_MSG_SIZE];
    ucdrStream us;
    ucdr_init_stream(&us, buffer, sizeof(buffer));

    const uxrInputReliableStream* stream = &session->streams.input_reliable[id.index];

    uxr_serialize_message_header(&us, session->info.id, 0, 0, session->info.key);
    uxr_buffer_submessage_header(&us, SUBMESSAGE_ID_ACKNACK, ACKNACK_PAYLOAD_SIZE, 0);

    ACKNACK_Payload payload;
    uint16_t nack_bitmap = uxr_compute_acknack(stream, &payload.first_unacked_seq_num);
    payload.nack_bitmap[0] = (uint8_t)(nack_bitmap >> 8);
    payload.nack_bitmap[1] = (uint8_t)((nack_bitmap << 8) >> 8);
    payload.stream_id = id.raw;
    (void) uxr_serialize_ACKNACK_Payload(&us, &payload);

    send_message(session, buffer, ucdr_used_size(&us));
}

void read_message(uxrSession* session, ucdrStream* us)
{
    uint8_t stream_id_raw; uxrSeqNum seq_num;
    if(uxr_read_session_header(&session->info, us, &stream_id_raw, &seq_num))
    {
        uxrStreamId id = uxr_stream_id_from_raw(stream_id_raw, UXR_INPUT_STREAM);
        read_stream(session, us, id, seq_num);
    }
}

void read_stream(
        uxrSession* session,
        ucdrStream* us,
        uxrStreamId stream_id,
        uxrSeqNum seq_num)
{
    switch(stream_id.type)
    {
        case UXR_NONE_STREAM:
        {
            stream_id = uxr_stream_id_from_raw(0x00, UXR_INPUT_STREAM);
            read_submessage_list(session, us, stream_id);
            break;
        }
        case UXR_BEST_EFFORT_STREAM:
        {
            uxrInputBestEffortStream* stream = uxr_get_input_best_effort_stream(&session->streams, stream_id.index);
            if(stream && uxr_receive_best_effort_message(stream, seq_num))
            {
                read_submessage_list(session, us, stream_id);
            }
            break;
        }
        case UXR_RELIABLE_STREAM:
        {
            uxrInputReliableStream* stream = uxr_get_input_reliable_stream(&session->streams, stream_id.index);
            bool input_buffer_used;
            if(stream && uxr_receive_reliable_message(stream, seq_num, us->iterator, ucdr_remaining_size(us), &input_buffer_used))
            {
                if(!input_buffer_used)
                {
                    read_submessage_list(session, us, stream_id);
                }

                ucdrStream next_mb;
                while(uxr_next_input_reliable_buffer_available(stream, &next_mb))
                {
                    read_submessage_list(session, &next_mb, stream_id);
                }
            }
            write_submessage_acknack(session, stream_id);
            break;
        }
        default:
            break;
    }
}

void read_submessage_list(
        uxrSession* session,
        ucdrStream* submessages,
        uxrStreamId stream_id)
{
    uint8_t id; uint16_t length; uint8_t flags;
    while(uxr_read_submessage_header(submessages, &id, &length, &flags))
    {
        ucdrStream submsg;
        ucdr_clone_stream(&submsg, submessages);
        read_submessage(session, &submsg, id, stream_id, length, flags);
        ucdr_promote_stream(submessages, length);
    }
}

void read_submessage(
        uxrSession* session,
        ucdrStream* submessage,
        uint8_t submessage_id,
        uxrStreamId stream_id,
        uint16_t length,
        uint8_t flags)
{
    switch(submessage_id)
    {
        case SUBMESSAGE_ID_STATUS_AGENT:
            if(stream_id.type == UXR_NONE_STREAM)
            {
                uxr_read_create_session_status(&session->info, submessage);
            }
            break;

        case SUBMESSAGE_ID_STATUS:
            if(stream_id.type == UXR_NONE_STREAM)
            {
                uxr_read_delete_session_status(&session->info, submessage);
            }
            else
            {
                read_submessage_status(session, submessage);
            }
            break;

        case SUBMESSAGE_ID_DATA:
            read_submessage_data(session, submessage, length, stream_id, flags & FORMAT_MASK);
            break;

        case SUBMESSAGE_ID_HEARTBEAT:
            read_submessage_heartbeat(session, submessage);
            break;

        case SUBMESSAGE_ID_ACKNACK:
            read_submessage_acknack(session, submessage);
            break;

        case SUBMESSAGE_ID_TIMESTAMP_REPLY:
            read_submessage_timestamp_reply(session, submessage);
            break;

#ifdef PERFORMANCE_TESTING
        case SUBMESSAGE_ID_PERFORMANCE:
            read_submessage_performance(session, submessage, length);
            break;
#endif

        default:
            break;
    }
}

void read_submessage_status(
        uxrSession* session,
        ucdrStream* submessage)
{
    STATUS_Payload payload;
    uxr_deserialize_STATUS_Payload(submessage, &payload);

    uxrObjectId object_id; uint16_t request_id;
    uxr_parse_base_object_request(&payload.base.related_request, &object_id, &request_id);

    uint8_t status = payload.base.result.status;
    process_status(session, object_id, request_id, status);
}


extern void read_submessage_format(
        uxrSession* session,
        ucdrStream* data,
        uint16_t length,
        uint8_t format,
        uxrStreamId stream_id,
        uxrObjectId object_id,
        uint16_t request_id);

void read_submessage_data(
        uxrSession* session,
        ucdrStream* submessage,
        uint16_t length,
        uxrStreamId stream_id,
        uint8_t format)
{
    BaseObjectRequest base;
    uxr_deserialize_BaseObjectRequest(submessage, &base);
    length = (uint16_t)(length - 4); //CHANGE: by a future size_of_BaseObjectRequest

    uxrObjectId object_id; uint16_t request_id;
    uxr_parse_base_object_request(&base, &object_id, &request_id);

    process_status(session, object_id, request_id, UXR_STATUS_OK);

    if(session->on_topic != NULL)
    {
        read_submessage_format(session, submessage, length, format, stream_id, object_id, request_id);
    }
}

void read_submessage_heartbeat(
        uxrSession* session,
        ucdrStream* submessage)
{
    HEARTBEAT_Payload heartbeat;
    uxr_deserialize_HEARTBEAT_Payload(submessage, &heartbeat);
    uxrStreamId id = uxr_stream_id_from_raw(heartbeat.stream_id, UXR_INPUT_STREAM);

    uxrInputReliableStream* stream = uxr_get_input_reliable_stream(&session->streams, id.index);
    if(stream)
    {
        uxr_process_heartbeat(stream, heartbeat.first_unacked_seq_nr, heartbeat.last_unacked_seq_nr);
        write_submessage_acknack(session, id);
    }
}

void read_submessage_acknack(
        uxrSession* session,
        ucdrStream* submessage)
{
    ACKNACK_Payload acknack;
    uxr_deserialize_ACKNACK_Payload(submessage, &acknack);
    uxrStreamId id = uxr_stream_id_from_raw(acknack.stream_id, UXR_INPUT_STREAM);

    uxrOutputReliableStream* stream = uxr_get_output_reliable_stream(&session->streams, id.index);
    if(stream)
    {
        uint16_t nack_bitmap = (uint16_t)(((uint16_t)acknack.nack_bitmap[0] << 8) + acknack.nack_bitmap[1]);
        uxr_process_acknack(stream, nack_bitmap, acknack.first_unacked_seq_num);

        uint8_t* buffer; size_t length;
        uxrSeqNum seq_num_it = uxr_begin_output_nack_buffer_it(stream);
        while (uxr_next_reliable_nack_buffer_to_send(stream, &buffer, &length, &seq_num_it))
        {
            send_message(session, buffer, length);
        }
    }
}

void read_submessage_timestamp_reply(
        uxrSession* session,
        ucdrStream* submessage)
{
    TIMESTAMP_REPLY_Payload timestamp_reply;
    uxr_deserialize_TIMESTAMP_REPLY_Payload(submessage, &timestamp_reply);

    process_timestamp_reply(session, &timestamp_reply);
}

#ifdef PERFORMANCE_TESTING
void read_submessage_performance(uxrSession* session, ucdrStream* submessage, uint16_t length)
{
    ucdrStream mb_performance;
    ucdr_init_buffer(&mb_performance, submessage->iterator, length);
    session->on_performance(session, &mb_performance, session->on_performance_args);
}
#endif

void process_status(
        uxrSession* session,
        uxrObjectId object_id,
        uint16_t request_id,
        uint8_t status)
{
    if(session->on_status != NULL)
    {
        session->on_status(session, object_id, request_id, status, session->on_status_args);
    }

    for(unsigned i = 0; i < session->request_status_list_size; ++i)
    {
        if(request_id == session->request_list[i])
        {
            session->status_list[i] = status;
            break;
        }
    }
}

void process_timestamp_reply(
        uxrSession* session,
        TIMESTAMP_REPLY_Payload* timestamp)
{
    if(session->on_time != NULL)
    {
        session->on_time(session,
            uxr_nanos(),
            uxr_convert_to_nanos(timestamp->receive_timestamp.seconds, timestamp->receive_timestamp.nanoseconds),
            uxr_convert_to_nanos(timestamp->transmit_timestamp.seconds, timestamp->transmit_timestamp.nanoseconds),
            uxr_convert_to_nanos(timestamp->originate_timestamp.seconds, timestamp->originate_timestamp.nanoseconds),
            session->on_time_args);
    }
    else
    {
        int64_t t3 = uxr_nanos();
        int64_t t0 = uxr_convert_to_nanos(timestamp->originate_timestamp.seconds,
                                          timestamp->originate_timestamp.nanoseconds);
        int64_t t1 = uxr_convert_to_nanos(timestamp->receive_timestamp.seconds,
                                          timestamp->receive_timestamp.nanoseconds);
        int64_t t2 = uxr_convert_to_nanos(timestamp->transmit_timestamp.seconds,
                                          timestamp->transmit_timestamp.nanoseconds);
        session->time_offset = ((t0 + t3) - (t1 + t2)) / 2;
    }
    session->synchronized = true;
}

bool uxr_prepare_stream_to_write_submessage(
        uxrSession* session,
        uxrStreamId stream_id,
        size_t payload_size,
        ucdrStream* us,
        uint8_t submessage_id,
        uint8_t mode)
{
    bool available = false;
    size_t submessage_size = SUBHEADER_SIZE + payload_size;

    switch(stream_id.type)
    {
        case UXR_BEST_EFFORT_STREAM:
        {
            uxrOutputBestEffortStream* stream = uxr_get_output_best_effort_stream(&session->streams, stream_id.index);
            available = stream && uxr_prepare_best_effort_buffer_to_write(stream, submessage_size, us);
            break;
        }
        case UXR_RELIABLE_STREAM:
        {
            uxrOutputReliableStream* stream = uxr_get_output_reliable_stream(&session->streams, stream_id.index);
            available = stream && uxr_prepare_reliable_buffer_to_write(stream, submessage_size, us);
            break;
        }
        default:
            break;
    }

    if(available)
    {
        (void) uxr_buffer_submessage_header(us, submessage_id, (uint16_t)payload_size, mode);
    }

    return available;
}

bool run_session_until_sync(
        uxrSession* session,
        int timeout)
{
    session->synchronized = false;
    bool timeout_exceeded = false;
    while(!timeout_exceeded && !session->synchronized)
    {
        timeout_exceeded = !listen_message_reliably(session, timeout);
    }
    return session->synchronized;
}
