#include "client.h"

#include "input_message.h"
#include "output_message.h"

#include "log/message.h"

#include <time.h>
#include <stdlib.h>
#include <transport/ddsxrce_transport.h>

typedef struct ClientState
{
    locator_id_t transport_id;

    uint32_t buffer_size;
    uint8_t* input_buffer;
    uint8_t* output_buffer;

    uint8_t session_id;
    uint8_t stream_id;
    uint16_t output_sequence_number;
    uint16_t input_sequence_number;
    ClientKey key;

    OutputMessage output_message;
    InputMessage input_message;

    InputMessageCallback input_callback;

} ClientState;

ClientState* new_client_state(uint32_t buffer_size, locator_id_t transport_id);

void on_initialize_message(MessageHeader* header, ClientKey* key, void* vstate);
int on_message_header(const MessageHeader* header, const ClientKey* key, void* vstate);

// ----------------------------------------------------------------------------------
//                                  CLIENT STATE
// ----------------------------------------------------------------------------------
ClientState* new_serial_client_state(uint32_t buffer_size, const char* device)
{
    return new_client_state(buffer_size, add_serial_locator(device));
}

ClientState* new_udp_client_state(uint32_t buffer_size, uint16_t recv_port, uint16_t send_port)
{
    return new_client_state(buffer_size, add_udp_locator(recv_port, send_port));
}

ClientState* new_client_state(uint32_t buffer_size, locator_id_t transport_id)
{
    ClientState* state = malloc(sizeof(ClientState));

    state->transport_id = transport_id;

    state->buffer_size = buffer_size;
    state->input_buffer = malloc(buffer_size);
    state->output_buffer = malloc(buffer_size);

    state->session_id = SESSIONID_NONE_WITH_CLIENT_KEY;
    state->stream_id = STREAMID_NONE;
    state->output_sequence_number = 0;
    state->input_sequence_number = 0;
    state->key = (ClientKey){0xFF, 0xFF, 0xFF, 0xFF};

    OutputMessageCallback output_callback;
    output_callback.object = state;
    output_callback.on_initialize_message = on_initialize_message;

    InputMessageCallback input_callback = {0};
    input_callback.object = state;
    input_callback.on_message_header = on_message_header;

    init_output_message(&state->output_message, output_callback, state->output_buffer, buffer_size);
    init_input_message(&state->input_message, input_callback, state->input_buffer, buffer_size);

    return state;
}

void free_client_state(ClientState* state)
{
    rm_locator(state->transport_id);
    free_input_message(&state->input_message);
    free(state->output_buffer);
    free(state->input_buffer);
    free(state);
}

// ----------------------------------------------------------------------------------
//                                    XRCE API
// ----------------------------------------------------------------------------------

void create_client(ClientState* state)
{
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);

    CreateClientPayload payload;
    payload.representation.xrce_cookie = XRCE_COOKIE;
    payload.representation.xrce_version = XRCE_VERSION;
    payload.representation.xrce_vendor_id = (XrceVendorId){0x01, 0x0F};
    payload.representation.client_timestamp.seconds = ts.tv_sec;
    payload.representation.client_timestamp.nanoseconds = ts.tv_nsec;
    payload.representation.client_key = (ClientKey){0xFF, 0xFF, 0xFF, 0xFF};
    payload.representation.session_id = 0x01;

    add_create_client_submessage(&state->output_message, &payload);
    PRINTL_CREATE_CLIENT_SUBMESSAGE(SEND, &payload);
}

uint16_t create_participant(ClientState* state)
{
    CreateResourcePayload payload;
    payload.request.base.request_id;
    payload.request.object_id;
    payload.representation.kind;
    payload.representation._.participant.base2.format;
    payload.representation._.participant.base2._.object_name;
    printf("%u\n", sizeof(ObjectId));
    add_create_resource_submessage(&state->output_message, &payload, 0);
    PRINTL_CREATE_RESOURCE_SUBMESSAGE(SEND, &payload);
}

// ----------------------------------------------------------------------------------
//                                 COMUNICATION
// ----------------------------------------------------------------------------------

void send_to_agent(ClientState* state)
{
    uint32_t length = get_message_length(&state->output_message);
    int output_length = send_data(state->output_buffer, length, state->transport_id);

    if(output_length > 0)
    {
        PRINT_SERIALIZATION(SEND, state->output_message.writer.init, output_length);
    }

    reset_buffer(&state->output_message.writer);
}

void received_from_agent(ClientState* state)
{
    uint32_t length = receive_data(state->input_buffer, state->buffer_size, state->transport_id);
    if(length > 0)
    {
        parse_message(&state->input_message, length);

        PRINT_SERIALIZATION(RECV, state->input_message.reader.init, length);
    }
}

// ----------------------------------------------------------------------------------
//                                   CALLBACKS
// ----------------------------------------------------------------------------------

void on_initialize_message(MessageHeader* header, ClientKey* key, void* vstate)
{
    ClientState* state = (ClientState*) vstate;

    header->session_id = state->session_id;
    header->stream_id = state->stream_id;
    header->sequence_nr = state->output_sequence_number;
    *key = state->key;

    state->output_sequence_number++;
}

int on_message_header(const MessageHeader* header, const ClientKey* key, void* vstate)
{
    ClientState* state = (ClientState*) vstate;

    if(header->stream_id != STREAMID_NONE)
        if(header->sequence_nr != state->input_sequence_number)
            return 0;

    state->input_sequence_number++;

    return 1;
}