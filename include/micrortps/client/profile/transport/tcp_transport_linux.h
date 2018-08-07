// Copyright 2018 Proyectos y Sistemas de Mantenimiento SL (eProsima).
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

#ifndef _MICRORTPS_CLIENT_TCP_TRANSPORT_LINUX_H_
#define _MICRORTPS_CLIENT_TCP_TRANSPORT_LINUX_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/poll.h>
#include <string.h>
#include <errno.h>

#define TCP_TRANSPORT_MTU 512

typedef struct TCPTransport TCPTransport;
struct TCPTransport
{
    uint8_t buffer[TCP_TRANSPORT_MTU];
    int socket_fd;
    struct sockaddr remote_addr;
    struct pollfd poll_fd;
};

int init_tcp_transport(TCPTransport* transport, const char* ip, uint16_t port);
intmax_t send_tcp_data(TCPTransport* transport, const void* buf, size_t len);
intmax_t recv_tcp_data(TCPTransport* transport, void** buf, size_t* len, int timeout);

#ifdef __cplusplus
}
#endif

#endif //_MICRORTPS_CLIENT_TCP_TRANSPORT_LINUX_H_
