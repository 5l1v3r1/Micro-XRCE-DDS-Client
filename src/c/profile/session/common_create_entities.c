#include <micrortps/client/profile/session/common_create_entities.h>
#include <micrortps/client/core/serialization/xrce_protocol.h>
#include <micrortps/client/core/session/submessage.h>

const uint8_t ENTITY_REPLACE = (uint8_t)FLAG_REPLACE;
const uint8_t ENTITY_REUSE = (uint8_t)FLAG_REUSE;

//==================================================================
//                              PUBLIC
//==================================================================
uint16_t write_delete_entity(Session* session, StreamId stream_id, mrObjectId object_id)
{
    uint16_t request_id = INVALID_REQUEST_ID;

    DELETE_Payload payload;

    // Change this when microcdr supports size_of function.
    int payload_length = 0; //DELETE_Payload_size(&payload);
    payload_length += 4; // delete payload (request id + object_id), no padding.

    MicroBuffer mb;
    bool available = prepare_stream_to_write(session, stream_id, payload_length + SUBHEADER_SIZE, &mb);
    if(available)
    {
        request_id = init_base_object_request(session, object_id, &payload.base);
        (void) write_submessage_header(&mb, SUBMESSAGE_ID_DELETE, payload_length, 0);
        (void) serialize_DELETE_Payload(&mb, &payload);
    }

    return request_id;
}

uint16_t common_create_entity(Session* session, StreamId stream_id,
                                  mrObjectId object_id, size_t xml_ref_size, uint8_t flags,
                                  CREATE_Payload* payload)
{
    uint16_t request_id = INVALID_REQUEST_ID;

    // Change this when microcdr supports size_of function. Currently, DOMAIN_ID is not supported.
    size_t payload_length = 0; //CREATE_Payload_size(&payload);
    payload_length += 4; // base
    payload_length += 1; // objk type
    payload_length += 1; // base3 type => xml
    payload_length += 2; // padding
    payload_length += 4; // xml length
    payload_length += xml_ref_size; // xml data (note: compiler executes strlen one time this function)
    payload_length += (object_id.type == OBJK_PARTICIPANT && payload_length % 2 != 0) ? 1 : 0; // necessary padding
    payload_length += (object_id.type == APPLICATION_ID || object_id.type == TYPE_ID ||
                       object_id.type  == DOMAIN_ID || object_id.type == QOS_PROFILE_ID) ? 0 : 2; //object id ref

    MicroBuffer mb;
    bool available = prepare_stream_to_write(session, stream_id, payload_length + SUBHEADER_SIZE, &mb);
    if(available)
    {
        request_id = init_base_object_request(session, object_id, &payload->base);
        (void) write_submessage_header(&mb, SUBMESSAGE_ID_CREATE, payload_length, flags);
        (void) serialize_CREATE_Payload(&mb, payload);
    }

    return request_id;
}

