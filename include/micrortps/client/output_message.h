#ifndef _MICRORTPS_CLIENT_OUTPUT_MESSAGE_H_
#define _MICRORTPS_CLIENT_OUTPUT_MESSAGE_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "micrortps/client/xrce_protocol_spec.h"

#include <microcdr/microcdr.h>

typedef struct OutputMessageCallback
{
    void (*on_initialize_message)(MessageHeader* header, ClientKey* key, void* callback_object);
    void* object;

} OutputMessageCallback;

typedef struct OutputMessage
{
    MicroBuffer writer;
    OutputMessageCallback callback;

} OutputMessage;

void init_output_message(OutputMessage* message, OutputMessageCallback callback,
                          uint8_t* out_buffer, uint32_t out_buffer_size);

bool add_create_client_submessage  (OutputMessage* message, const CreateClientPayload* payload);
bool add_create_resource_submessage(OutputMessage* message, const CreateResourcePayload* payload, uint8_t creation_mode);
bool add_delete_resource_submessage(OutputMessage* message, const DeleteResourcePayload* payload);
bool add_get_info_submessage       (OutputMessage* message, const GetInfoPayload* payload);
bool add_read_data_submessage      (OutputMessage* message, const ReadDataPayload* payload);
bool add_write_data_submessage     (OutputMessage* message, const WriteDataPayload* payload);

#ifdef __cplusplus
}
#endif

#endif //_MICRORTPS_CLIENT_OUTPUT_MESSAGE_H_