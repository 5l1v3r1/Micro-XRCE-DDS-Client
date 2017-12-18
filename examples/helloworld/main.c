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
#include <unistd.h>

// ----------------------------------------------------
//    App client
// ----------------------------------------------------
void on_hello_topic(XRCEInfo info, const void *topic, void *args);
void on_status_received(XRCEInfo info, uint8_t operation, uint8_t status, void *args);

void printl_hello_topic(const HelloWorld *hello_topic);
void *listen_agent(void *args);
bool compute_command(const char *command, ClientState *state);
void list_commands();
void help();

String read_file(char *file_name);

int check_input()
{
    struct timeval tv;
    fd_set fds = {{0, 0}};
    FD_ZERO(&fds);
    FD_SET(0, &fds); //STDIN 0
    select(1, &fds, NULL, NULL, &tv);
    return FD_ISSET(0, &fds);
}

int main(int args, char **argv)
{
    printf("<< HELLO WORLD XRCE CLIENT >>\n");

    ClientState *state = NULL;
    if (args > 3)
    {
        if (strcmp(argv[1], "serial") == 0)
        {
            state = new_serial_client_state(MAX_MESSAGE_SIZE, argv[2]);
            printf("<< Serial mode => dev: %s >>\n", argv[2]);
        }
        else if (strcmp(argv[1], "udp") == 0 && args == 5)
        {
            uint16_t received_port = atoi(argv[3]);
            uint16_t send_port = atoi(argv[4]);
            state = new_udp_client_state(MAX_MESSAGE_SIZE, argv[2], received_port, send_port);
            printf("<< UDP mode => recv port: %u, send port: %u >>\n", received_port, send_port);
        }
    }
    if (!state)
    {
        printf("Help: program [serial | udp dest_ip recv_port send_port]\n");
        return 1;
    }

    // Waiting user commands
    printf(":>");
    char command_stdin_line[256];
    bool running = true;
    while (running)
    {
        if (!check_input())
        {
            receive_from_agent(state);
        }
        else if (fgets(command_stdin_line, 256, stdin))
        {
            if (!compute_command(command_stdin_line, state))
            {
                running = false;
                break;
            }
        }
        usleep(1000); // usleep takes sleep time in us (1 millionth of a second)
    }
}

bool compute_command(const char *command, ClientState *state)
{
    char name[128];
    static unsigned int hello_world_id = 0;
    int id = 0;
    int extra = 0;
    int length = sscanf(command, "%s %i %i", name, &id, &extra);

    if (strcmp(name, "create_client") == 0)
    {
        create_client(state, on_status_received, NULL);
    }
    else if (strcmp(name, "create_participant") == 0)
    {
        create_participant(state);
    }
    else if (strcmp(name, "create_topic") == 0 && length == 2)
    {
        String xml = read_file("hello_topic.xml");
        if (xml.length > 0)
        {
            create_topic(state, id, xml);
        }
    }
    else if (strcmp(name, "create_publisher") == 0 && length == 2)
    {
        create_publisher(state, id);
    }
    else if (strcmp(name, "create_subscriber") == 0 && length == 2)
    {
        create_subscriber(state, id);
    }
    else if (strcmp(name, "create_data_writer") == 0 && length == 3)
    {
        String xml = read_file("hello_data_writer_profile.xml");
        if (xml.length > 0)
        {
            create_data_writer(state, id, extra, xml);
        }
    }
    else if (strcmp(name, "create_data_reader") == 0 && length == 3)
    {
        String xml = read_file("hello_data_reader_profile.xml");
        if (xml.length > 0)
        {
            create_data_reader(state, id, extra, xml);
        }
    }
    else if (strcmp(name, "write_data") == 0 && length == 2)
    {
        char message[] = "Hello from client";
        HelloWorld hello_topic = (HelloWorld){hello_world_id++, message};
        write_data(state, id, serialize_HelloWorld_topic, &hello_topic);
        printl_hello_topic(&hello_topic);
    }
    else if (strcmp(name, "read_data") == 0 && length == 2)
    {
        read_data(state, id, deserialize_HelloWorld_topic, on_hello_topic, NULL);
    }
    else if (strcmp(name, "delete") == 0 && length == 2)
    {
        delete_resource(state, id);
    }
    else if (strcmp(name, "h") == 0 || strcmp(name, "help") == 0)
    {
        list_commands();
    }
    else if (strcmp(name, "exit") == 0)
    {
        return false;
    }
    else
    {
        help();
    }

    // only send data if there is.
    send_to_agent(state);

    return true;
}

void on_hello_topic(XRCEInfo info, const void *vtopic, void *args)
{
    (void)info;
    (void)args;
    HelloWorld *topic = (HelloWorld *)vtopic;
    printl_hello_topic(topic);

    free(topic->m_message);
    free(topic);
}

void on_status_received(XRCEInfo info, uint8_t operation, uint8_t status, void *args)
{
    (void)info;
    (void)operation;
    (void)status;
    (void)args;
    printf("User status callback\n");
}

void printl_hello_topic(const HelloWorld *hello_topic)
{
    printf("        %s[%s | index: %u]%s\n",
           "\x1B[1;34m",
           hello_topic->m_message,
           hello_topic->m_index,
           "\x1B[0m");
}

String read_file(char *file_name)
{
    printf("READ FILE\n");
    const size_t MAXBUFLEN = 4096;
    char data[MAXBUFLEN];
    String xml = {data, 0};
    FILE *fp = fopen(file_name, "r");
    if (fp != NULL)
    {
        xml.length = fread(xml.data, sizeof(char), MAXBUFLEN, fp);
        if (xml.length == 0)
        {
            printf("Error reading %s\n", file_name);
        }
        fclose(fp);
    }
    else
    {
        printf("Error opening %s\n", file_name);
    }

    return xml;
}

void help()
{
    printf("usage: <command> [<args>]\n");
    printf("    h, help: for command list\n");
}

void list_commands()
{
    printf("usage: <command> [<args>]\n");
    printf("    create_client:                                       Creates a Client\n");
    printf("    create_participant:                                  Creates a new Participant on the current Client\n");
    printf("    create_topic <participant id>:                       Register new Topic using <participant id> participant\n");
    printf("    create_publisher <participant id>:                   Creates a Publisher on <participant id> participant\n");
    printf("    create_subscriber <participant id>:                  Creates a Subscriber on <participant id> participant\n");
    printf("    create_data_writer <participant id> <publisher id>:  Creates a DataWriter on the publisher <publisher id> of the <participant id> participant\n");
    printf("    create_data_reader <participant id> <subscriber id>: Creates a DataReader on the subscriber <subscriber id> of the <participant id> participant\n");
    printf("    write_data <data writer id>:                         Write data using <data writer id> DataWriter\n");
    printf("    read_data <data reader id>:                          Read data using <data reader id> DataReader\n");
    printf("    delete <id>:                                         Removes object with <id> identifier\n");
    printf("    exit:                                                Close program\n");
    printf("    h, help:                                             Shows this message\n");
}
