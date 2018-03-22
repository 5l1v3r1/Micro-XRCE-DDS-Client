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

#include "HelloWorld.h"

#include <stdio.h>

size_t read_file(const char *file_name, char* data_file, size_t buf_size)
{
    printf("READ FILE\n");
    FILE *fp = fopen(file_name, "r");
    size_t length = 0;
    if (fp != NULL)
    {
        length = fread(data_file, sizeof(char), buf_size, fp);
        if (length == 0)
        {
            printf("Error reading %s\n", file_name);
        }
        fclose(fp);
    }
    else
    {
        printf("Error opening %s\n", file_name);
    }

    return length;
}

int main(int args, char** argv)
{
    (void)args;
    (void)argv;

    uint8_t result;
    uint8_t my_buffer[1024];
    Session my_session;

    /* Create udp session */
    result = new_udp_session(&my_session, my_buffer, sizeof(my_buffer),
                             4000, 2020, 2019, "127.0.0.1");
    if (result != SESSION_CREATED)
    {
        return 1;
    }

    result = init_session(&my_session, NULL, on_status, NULL);
    if (result != SESSION_INIT)
    {
        return 2;
    }

    // Creates a participant on the Agent.
    XRCEInfo participant_info = create_participant(&my_session);

    // Register a topic on the given participant. Uses a topic configuration read from xml file
    String topic_profile = {"<dds><topic><name>HelloWorldTopic</name><dataType>HelloWorld</dataType></topic></dds>", 86+1};
    create_topic(&my_session, participant_info.object_id, topic_profile);

    // Creates a publisher on the given participant
    XRCEInfo publisher_info = create_publisher(&my_session, participant_info.object_id);

    String data_writer_profile = {"<profiles><publisher profile_name=\"default_xrce_publisher_profile\"><topic><kind>NO_KEY</kind><name>HelloWorldTopic</name><dataType>HelloWorld</dataType><historyQos><kind>KEEP_LAST</kind><depth>5</depth></historyQos><durability><kind>TRANSIENT_LOCAL</kind></durability></topic></publisher></profiles>",
    300+1};
    XRCEInfo data_writer_info = create_data_writer(&my_session, participant_info.object_id, publisher_info.object_id, data_writer_profile); //read_file("hello_data_writer_profile.xml"));

    // Prepare and write the user data to be sent.
    char message[] = "Hello DDS world!";
    HelloWorld hello_topic = {1, message};

    // Write user type data.
    write_data(&my_session, data_writer_info.object_id, serialize_HelloWorld_topic, &hello_topic);

    // Send the data through the UDP transport.
    send_to_agent(&my_session);

    close_session(&my_session);

}
