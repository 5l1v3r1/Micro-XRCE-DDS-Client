#ifndef _DDS_XRCE_XRCE_SPEC_H_
#define _DDS_XRCE_XRCE_SPEC_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>

// --------------------------------------------------------------------
//                               DEFINES
// --------------------------------------------------------------------
#define SUBMESSAGE_CREATE        0x01
#define SUBMESSAGE_DELETE        0x03
#define SUBMESSAGE_STATUS        0x04
#define SUBMESSAGE_WRITE_DATA    0x06
#define SUBMESSAGE_READ_DATA     0x07
#define SUBMESSAGE_DATA          0x08

#define OBJECT_ID_INVALID    0xFFFFFF
#define OBJECT_ID_CLIENT     0xFFFFF0
#define OBJECT_ID_SESSION    0xFFFFF1

#define OBJECT_KIND_PARTICIPANT   0x01
#define OBJECT_KIND_DATA_WRITER   0x03
#define OBJECT_KIND_DATA_READER   0x07
#define OBJECT_KIND_SUBSCRIBER    0x08
#define OBJECT_KIND_PUBLISHER     0x09

#define READ_MODE_DATA              0x00
#define READ_MODE_DATA_SEQ          0x01
#define READ_MODE_SAMPLE            0x02
#define READ_MODE_SAMPLE_SEQ        0x03
#define READ_MODE_PACKED_SAMPLE_SEQ 0x04

#define STATUS_OK                    0x00
#define STATUS_OK_MATCHED            0x01
#define STATUS_ERR_DDS_ERROR         0x80
#define STATUS_ERR_MISMATCH          0x81
#define STATUS_ERR_ALREADY_EXISTS    0x82
#define STATUS_ERR_DENIED            0x83
#define STATUS_ERR_UNKNOWN_REFERENCE 0x84
#define STATUS_ERR_INVALID_DATA      0x85
#define STATUS_ERR_INCOMPATIBLE      0x86
#define STATUS_ERR_RESOURCES         0x87

#define STATUS_LAST_OP_NONE      0
#define STATUS_LAST_OP_CREATE    1
#define STATUS_LAST_OP_UPDATE    2
#define STATUS_LAST_OP_DELETE    3
#define STATUS_LAST_OP_LOOKUP    4
#define STATUS_LAST_OP_READ      5
#define STATUS_LAST_OP_WRITE     6

typedef uint32_t uint_least24_t;

// --------------------------------------------------------------------
//                            OBJECT VARIANT
// --------------------------------------------------------------------

typedef struct DataWriterSpec
{
    uint_least24_t participant_id;
    uint_least24_t publisher_id;

} DataWriterSpec;

typedef struct DataReaderSpec
{
    uint_least24_t participant_id;
    uint_least24_t subscriber_id;

} DataReaderSpec;

typedef struct PublisherSpec
{
    uint_least24_t participant_id;

} PublisherSpec;

typedef struct SubscriberSpec
{
    uint_least24_t participant_id;

} SubscriberSpec;

typedef union ObjectVariantSpec
{
    DataWriterSpec data_writer;
    DataReaderSpec data_reader;
    PublisherSpec publisher;
    SubscriberSpec subscriber;

} ObjectVariantSpec;

typedef struct ObjectKindSpec
{
    uint8_t kind;
    uint32_t string_size;
    char* string;
    ObjectVariantSpec variant;

} ObjectKindSpec;

// --------------------------------------------------------------------
//                                STATUS
// --------------------------------------------------------------------
typedef struct ResultStatusSpec
{
    uint32_t request_id;
    uint8_t status;
    uint8_t last_operation;

} ResultStatusSpec;

// --------------------------------------------------------------------
//                               SAMPLES
// --------------------------------------------------------------------
typedef struct SampleInfoSpec
{
    uint8_t state;
    uint32_t sequence_number; //change to uint64_t
    uint32_t session_time_offset;
} SampleInfoSpec;

typedef struct SampleDataSpec
{
    uint32_t serialized_data_size;
    uint8_t* serialized_data;

} SampleDataSpec;

typedef struct SampleSpec
{
    SampleInfoSpec info;
    SampleDataSpec data;

} SampleSpec;

typedef union SampleKindSpec
{
    SampleDataSpec data;
    SampleSpec sample;

} SampleKindSpec;

typedef struct DataModeSpec
{
    uint8_t read_mode;
    SampleKindSpec sample_kind;

} DataModeSpec;

// --------------------------------------------------------------------
//                               PAYLOADS
// --------------------------------------------------------------------
typedef struct CreatePayloadSpec
{
    uint32_t request_id;
    uint_least24_t object_id;
    ObjectKindSpec object;

} CreatePayloadSpec;

typedef struct DeletePayloadSpec
{
    uint32_t request_id;
    uint_least24_t object_id;

} DeletePayloadSpec;

typedef struct StatusPayloadSpec
{
    ResultStatusSpec  result;
    uint_least24_t object_id;

} StatusPayloadSpec;

typedef struct WriteDataPayloadSpec
{
    uint32_t request_id;
    uint_least24_t object_id;
    DataModeSpec data_writer;

} WriteDataPayloadSpec;

typedef struct ReadDataPayloadSpec
{
    uint32_t request_id;
    uint_least24_t object_id;
    uint16_t max_messages;
    uint8_t read_mode;
    uint32_t max_elapsed_time;
    uint32_t max_rate;
    uint32_t expression_size;
    char* content_filter_expression;
    uint16_t max_samples;
    uint8_t include_sample_info;

} ReadDataPayloadSpec;

typedef struct DataPayloadSpec
{
    uint32_t request_id;
    uint_least24_t object_id;
    DataModeSpec data_reader;

} DataPayloadSpec;

typedef union PayloadSpec
{
    CreatePayloadSpec create_resource;
    DeletePayloadSpec delete_resource;
    StatusPayloadSpec status;
    WriteDataPayloadSpec write_data;
    ReadDataPayloadSpec read_data;
    DataPayloadSpec data;

} PayloadSpec;

// --------------------------------------------------------------------
//                               MESSAGE
// --------------------------------------------------------------------
typedef struct MessageHeaderSpec
{
    uint32_t client_key;
    uint8_t session_id;
    uint8_t stream_id;
    uint16_t sequence_number;

} MessageHeaderSpec;

typedef struct SubmessageHeaderSpec
{
    uint8_t id;
    uint8_t flags;
    uint16_t length;

} SubmessageHeaderSpec;


#ifdef __cplusplus
}
#endif

#endif //_DDS_XRCE_XRCE_SPEC_H_