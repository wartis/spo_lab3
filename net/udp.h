//
// Created by kirill
//

#ifndef LR3_NET_UDP_H
#define LR3_NET_UDP_H

#include "../application_context.h"

typedef struct udp_search_data {
    application_context *app_context;
    char *file_str;
} udp_search_data;

typedef struct udp_answer {
    int success_result;
    int port;
    file_description_send file_desc_send;
} udp_answer;

typedef struct checking_servers_data {
    int found_count;
    char *string_ports;
} checking_servers_data;

void start_udp_listener(application_context *app_context);

void download_from_server(application_context *app_context, char *search_file_description, int offset, int download_size, int port);

void check_server(int port, char *search_file_string, checking_servers_data *checking_servers_data);

void search_other_servers(udp_search_data *udp_sd);

void inform_instance_count(int count, int port);

#endif //LR3_NET_UDP_H
