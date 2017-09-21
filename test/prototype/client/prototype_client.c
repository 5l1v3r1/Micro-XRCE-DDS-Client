#include <micrortps/client/client.h>

#include "../common/file_io.h"
#include "../common/serial_port_io.h"
#include "../common/shape_topic.h"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>


#define BUFFER_SIZE 1024

typedef struct UserData
{
    uint32_t recieved_topics;
    char command_file_name[256];

} UserData;


void compute_command(UserData* user, Topic* topic);

//callbacks for listening topics
void on_listener_shape_topic(const void* topic_data, void* callback_object);

int main(int args, char** argv)
{
    UserData user = {0, "command.io"};

    if(args == 2)
    {
        if(strcmp(argv[1], "file") == 0)
        {
            SharedFile* shared_file = malloc(sizeof(SharedFile));
            init_file_io(shared_file, "client_to_agent.bin", "agent_to_client.bin");
            fclose(fopen(user.command_file_name, "w"));

            init_client(BUFFER_SIZE, send_file_io, received_file_io, shared_file, &user);
        }
        else if(strcmp(argv[1], "serial_port") == 0)
        {
            SerialPort* serial_port = malloc(sizeof(SerialPort));
            init_serial_port_io(serial_port, "/dev/ttyACM0");

            init_client(BUFFER_SIZE, send_serial_port_io, received_serial_port_io, serial_port, &user);
        }
        else
        {
            printf(">> Write option: [file | serial_port]\n");
            return 0;
        }
    }
    else
    {
        printf(">> Write option: [file | serial_port]\n");
        return 0;
    }

    //init_client(1024, send_data_file, received_data_file, &shared_file, &user);

    Topic topic =
    {
        "SQUARE",
        serialize_shape_topic,
        deserialize_shape_topic,
        size_of_shape_topic
    };

    uint32_t dt = 2;
    char time_string[80];
    while(user.recieved_topics < 10)
    {
        time_t t = time(NULL);
        struct tm* timeinfo = localtime(&t);
        strftime(time_string, 80, "%T", timeinfo);
        printf("============================ CLIENT ========================> %s\n", time_string);

        // user code here
        compute_command(&user, &topic);

        // this function does all comunnications
        update_communication();

        usleep(1000000 * dt);
    }

    return 0;
}

void on_listener_shape_topic(const void* topic_data, void* callback_object)
{
    const ShapeTopic* shape_topic = (const ShapeTopic*)topic_data;

    print_shape_topic(shape_topic);

    UserData* user = (UserData*)callback_object;
    user->recieved_topics++;
}

/*
COMMANDS:
    > create_pub
    > create_sub
    > config_read sub_id
    > send pub_id color x y size
    > delete_pub pub_id
    > delete_sub sub_id
*/

void compute_command(UserData* user, Topic* topic)
{
    FILE* commands_file = fopen(user->command_file_name, "r");
    char command[256];
    uint32_t id;
    char color[256];
    uint32_t x;
    uint32_t y;
    uint32_t size;
    int valid_command = 0;
    int command_size = fscanf(commands_file, "%s %u %s %u %u %u", command, &id, color, &x, &y, &size);

    if(command_size == -1)
    {
        valid_command = 1;
    }
    else if(command_size == 1)
    {
        if(strcmp(command, "create_pub") == 0)
        {
            create_publisher(topic);
            valid_command = 1;
        }
        else if(strcmp(command, "create_sub") == 0)
        {
            Subscriber* subscriber = create_subscriber(topic);
            add_listener_topic(subscriber, on_listener_shape_topic);
            valid_command = 1;
        }
    }
    else if(command_size == 2)
    {
        if(strcmp(command, "delete") == 0)
        {
            void* object;
            int kind = get_xrce_object(id, &object);
            if(kind == OBJECT_PUBLISHER)
            {
                delete_publisher(object);
                valid_command = 1;
            }
            else if(kind == OBJECT_SUBSCRIBER)
            {
                delete_subscriber(object);
                valid_command = 1;
            }
        }
    }
    else if(command_size == 3)
    {
        if(strcmp(command, "read_data") == 0)
        {
            void* object;
            if(get_xrce_object(id, &object) == OBJECT_SUBSCRIBER)
            {
                read_data(object, atoi(color));
                valid_command = 1;
            }
        }
    }
    else if(command_size == 6)
    {
        if(strcmp(command, "send") == 0)
        {
            void* object;
            if(get_xrce_object(id, &object) == OBJECT_PUBLISHER)
            {
                ShapeTopic shape_topic = {strlen(color) + 1, color, x, y, size};
                if(send_topic(object, &shape_topic))
                    print_shape_topic(&shape_topic);
                valid_command = 1;
            }
        }
    }

    if(!valid_command)
        printf("Unknown command or incorrect input \n");

    fclose(commands_file);
    fclose(fopen(user->command_file_name, "w"));
}