//
// Created by kirill
//
#include <netinet/in.h>

#include "../application_context.h"
#include "../file_controller/file_controller.h"

#ifndef LR3_NET_TCP_H
#define LR3_NET_TCP_H

typedef struct tcp_description {
    int tcp_port;
    int sockfd;
    struct sockaddr_in server_address;
    struct sockaddr_in client_address;
} tcp_description;

typedef struct tcp_server_thread_description {
    application_context *app_context;
    file_description *file_desc;
    tcp_description *tcp_description;
} tcp_server_thread_description;

typedef struct tcp_client_thread_description {
    application_context *app_context;
    file_description_send file_desc_send;
    int port;
    int address;
    int file_offset;
    int download_size;
} tcp_client_thread_description;

typedef struct tcp_server_file_answer {
    int file_part[4096];
    int file_part_len;
} tcp_server_file_answer;

typedef struct tcp_client_progress_data {
    int current_size;
    int current_percents;
} tcp_client_progress_data;

typedef struct tcp_client_download_data {
    char cmd[3];
    int file_part_offset;
    int file_part_size;
} tcp_client_download_data;

void create_tcp_socket(tcp_description *td);

void start_tcp_server(tcp_server_thread_description *tcp_server_thread_description);

void start_tcp_client(tcp_client_thread_description *tcp_client_thread_description);

#endif //LR3_NET_TCP_H
