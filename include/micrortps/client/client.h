#ifndef _MICRORTPS_CLIENT_CLIENT_H_
#define _MICRORTPS_CLIENT_CLIENT_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include <stdbool.h>

typedef struct ClientState ClientState;
typedef struct MicroBuffer MicroBuffer;


typedef enum CreateModeFlags
{
    REUSE_MODE =   0x01 << 0,
    REPLACE_MODE = 0x01 << 1

} CreateModeFlags;

typedef struct String
{
    char* data;
    uint32_t length;

} String;

typedef struct AbstractTopic
{
    void* topic;

} AbstractTopic;


typedef bool (*SerializeTopic)(MicroBuffer* writer, const AbstractTopic* topic_structure);
typedef bool (*DeserializeTopic)(MicroBuffer* reader, AbstractTopic* topic_structure);

typedef void (*OnTopic)(const void* topic, void* args);

// ----------------------------------------------------------------------------------------------
//    State funcions
// ----------------------------------------------------------------------------------------------
ClientState* new_serial_client_state(uint32_t buffer_size, const char* device);
ClientState* new_udp_client_state(uint32_t buffer_size, uint16_t recv_port, uint16_t send_port);
void free_client_state(ClientState* state);

// ----------------------------------------------------------------------------------------------
//    XRCE Client API funcions
// ----------------------------------------------------------------------------------------------
void create_client(ClientState* state);
uint16_t create_participant(ClientState* state);
uint16_t create_publisher(ClientState* state, uint16_t participant_id);
uint16_t create_subscriber(ClientState* state, uint16_t participant_id);
uint16_t create_topic(ClientState* state, uint16_t participant_id, String name);
uint16_t create_data_writer(ClientState* state, uint16_t participant_id, uint16_t publisher_id, String xml);
uint16_t create_data_reader(ClientState* state, uint16_t participant_id, uint16_t subscriber_id, String xml);

void delete_resource(ClientState* state, uint16_t resource_id);

void write_data(ClientState* state, uint16_t data_writer_id, SerializeTopic serialization, void* topic);
void read_data(ClientState* state, uint16_t data_reader_id, DeserializeTopic deserialization,
        OnTopic on_topic, void* on_topic_args);

// ----------------------------------------------------------------------------------------------
//    Comunication functions
// ----------------------------------------------------------------------------------------------
void send_to_agent(ClientState* state);
void receive_from_agent(ClientState* state);

#ifdef __cplusplus
}
#endif

#endif //_MICRORTPS_CLIENT_CLIENT_H_