//
// Created by kirill
//

#include "udp.h"
#include "tcp.h"
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include "../ui/ui.h"

#include "../file_controller/file_controller.h"

#define UDP_PORT 8888

int udp_port = 0;

void inform_instance_count(int count, int port) {
    int sockfd;
    struct sockaddr_in server_address;

    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
        perror("socket creation failed");
        return;
    }

    memset(&server_address, 0, sizeof(server_address));

    int broadcast = 1;

    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;

    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        perror("socket timer failed");
        return;
    }

    if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast)) < 0) {
        perror("socket broadcast failed");
        return;
    }

    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = htonl(INADDR_BROADCAST);
    server_address.sin_port = htons(port);

    char count_str[10];

    sprintf(count_str, "%d", count);
    strcat(count_str, " instance");

    sendto(sockfd, count_str, sizeof(count_str), MSG_CONFIRM,
           (const struct sockaddr *) &server_address, sizeof(server_address));
}

void start_udp_listener(application_context *app_context) {

    int sockfd;
    struct sockaddr_in server_address, foreign_address;

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
        perror("socket creation failed");
        return;
    }

    memset(&server_address, 0, sizeof(server_address));
    memset(&foreign_address, 0, sizeof(foreign_address));

    udp_port = UDP_PORT;
    int broadcast = 1;

    if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast)) < 0) {
        perror("socket broadcast failed");
        return;
    }

    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;

    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        perror("socket timer failed");
        return;
    }

    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(udp_port);

    while (bind(sockfd, (const struct sockaddr*) &server_address, sizeof(server_address)) < 0) {
        server_address.sin_port = htons(++udp_port);
    }


    if (udp_port > UDP_PORT) {
        for (int i = 0; i < udp_port - UDP_PORT; i++) {
            inform_instance_count(udp_port + 1 - UDP_PORT, i + UDP_PORT);
        }
        app_context->instance_count = udp_port - UDP_PORT + 1;
    } else {
        app_context->instance_count = 1;
    }

    while (!app_context->ui_ready) {

    }

    char *buf = calloc(1, BUF_SIZE);

    sprintf(buf, "UDP started on port %d", udp_port);
    print_log(buf, F_YELLOW);

    memset(buf, 0, BUF_SIZE);

    while (!app_context->exit_code) {

        int len, n;
        memset(buf, 0, BUF_SIZE);

        len = sizeof(foreign_address);
        n = recvfrom(sockfd, buf, BUF_SIZE, MSG_WAITALL, (struct sockaddr*) &foreign_address, (socklen_t *) &len);

        if (strstr(buf, " instance")) {
            app_context->instance_count = buf[0] - '0';
        } else if (strcmp(buf, "check") == 0) {
            n = recvfrom(sockfd, buf, BUF_SIZE, MSG_WAITALL, (struct sockaddr*) &foreign_address, (socklen_t *) &len);
            file_description *fd = find_file_description(app_context->list_fd_head, buf);

            if (fd != NULL) {
                char str[10];
                strcpy(str, "found");

                sendto(sockfd, str, sizeof(str), MSG_CONFIRM,
                       (const struct sockaddr *) &foreign_address, len);
            }
        } else {
            file_description *fd = find_file_description(app_context->list_fd_head, buf);

            if (fd != NULL) {
                char *str = malloc(BUF_SIZE);
                memset(str, 0, BUF_SIZE);
                sprintf(str, "Starting uploading file '%s'", fd->name);
                print_log(str, F_BLUE);
                free(str);
                udp_answer udp_answ = {0};
                tcp_description *tcp_desc = calloc(1, sizeof(tcp_description));
                tcp_server_thread_description *tcp_server_thread_description = calloc(1, sizeof(tcp_server_thread_description));

                create_tcp_socket(tcp_desc);

                tcp_server_thread_description->app_context = app_context;
                tcp_server_thread_description->file_desc = fd;
                tcp_server_thread_description->tcp_description = tcp_desc;

                udp_answ.port = tcp_desc->tcp_port;
                udp_answ.success_result = 1;
                strcpy(udp_answ.file_desc_send.name, fd->name);
                strcpy(udp_answ.file_desc_send.hash, fd->hash);
                strcpy(udp_answ.file_desc_send.path, fd->path);
                udp_answ.file_desc_send.size = fd->size;

                pthread_t *tcp_server_thread = (pthread_t *) malloc(sizeof(pthread_t));
                pthread_create(tcp_server_thread, NULL, (void *) start_tcp_server, tcp_server_thread_description);

                sendto(sockfd, &udp_answ, sizeof(udp_answer), MSG_CONFIRM,
                       (const struct sockaddr *) &foreign_address, len);
            } else {

            }
        }
    }

}

void search_other_servers(udp_search_data *udp_sd) {
    file_description *fd = find_file_description(udp_sd->app_context->list_fd_head, udp_sd->file_str);

    if (fd != NULL) {
        char *str = malloc(BUF_SIZE);
        memset(str, 0, BUF_SIZE);
        sprintf(str, "File '%s/%s/%d' exists here", fd->name, fd->hash, fd->size);
        print_log(str, F_CYAN);
        free(str);
        return;
    }

    checking_servers_data *checking_servers_data = malloc(sizeof(checking_servers_data));
    checking_servers_data->found_count = 0;
    checking_servers_data->string_ports = malloc(BUF_SIZE);
    memset(checking_servers_data->string_ports, 0, BUF_SIZE);
    strcpy(checking_servers_data->string_ports, "/");

    for (int i = 0; i < udp_sd->app_context->instance_count; i++) {
        if ((UDP_PORT + i) != udp_port)
            check_server(UDP_PORT + i, udp_sd->file_str, checking_servers_data);
    }

    if (checking_servers_data->found_count == 0) {
        char *str = malloc(BUF_SIZE);
        memset(str, 0, BUF_SIZE);
        sprintf(str, "File '%s' not found", udp_sd->file_str);
        print_log(str, F_CYAN);
        free(str);
    } else {
        char *str = malloc(BUF_SIZE);
        memset(str, 0, BUF_SIZE);
        sprintf(str, "File found on %d servers with ports %s", checking_servers_data->found_count, checking_servers_data->string_ports);
        print_log(str, F_BLUE);

        for (int i = 0; i < checking_servers_data->found_count; i++) {
            char *str = malloc(BUF_SIZE);
            memset(str, 0, BUF_SIZE);
            strncpy(str, checking_servers_data->string_ports + i * 5 + 1, 4);
            int port = atoi(str);
            free(str);

            char *size_str = malloc(BUF_SIZE);

            int j = 0;

            for (int i = 1; i < strlen(udp_sd->file_str); i++) {
                if (strstr(&udp_sd->file_str[strlen(udp_sd->file_str) - i], "/")) {
                    j = i - 1;
                    break;
                }
            }

            strcpy(size_str, &udp_sd->file_str[strlen(udp_sd->file_str) - j]);

            int full_file_size = atoi(size_str);
            free(size_str);

            int file_part_size = (int) full_file_size / checking_servers_data->found_count;

            if (i != checking_servers_data->found_count - 1) {
                download_from_server(udp_sd->app_context, udp_sd->file_str, file_part_size * i, file_part_size, port);
            } else {
                download_from_server(udp_sd->app_context, udp_sd->file_str, file_part_size * i, full_file_size - file_part_size * i, port);
            }
        }
    }
}

void check_server(int port, char *search_file_string, checking_servers_data *checking_servers_data) {

    int sockfd;
    struct sockaddr_in server_address, foreign_address;

    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
        perror("socket creation failed");
        return;
    }

    memset(&server_address, 0, sizeof(server_address));
    memset(&foreign_address, 0, sizeof(foreign_address));

//    int udp_port = UDP_PORT;
    int broadcast = 1;

    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;

    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        perror("socket timer failed");
        return;
    }

    if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast)) < 0) {
        perror("socket broadcast failed");
        return;
    }

    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = htonl(INADDR_BROADCAST);
    server_address.sin_port = htons(port);

    char str[10];
    strcpy(str, "check");

    sendto(sockfd, str, strlen(str), MSG_CONFIRM,
           (const struct sockaddr *) &server_address, sizeof(server_address));

    sendto(sockfd, search_file_string, strlen(search_file_string), MSG_CONFIRM,
           (const struct sockaddr *) &server_address, sizeof(server_address));

    int n, len;
    len = sizeof(foreign_address);

    int buffer[BUF_SIZE] = {0};
    memset(buffer, 0, BUF_SIZE);

    n = recvfrom(sockfd, (char *) buffer, BUF_SIZE, MSG_WAITALL, (struct sockaddr *) &foreign_address, &len);

    if (strcmp((char *) buffer, "found") == 0) {
        char *str = malloc(BUF_SIZE);
        checking_servers_data->found_count++;
        memset(str, 0 ,BUF_SIZE);
        sprintf(str, "%d", port);
        strcat(checking_servers_data->string_ports, str);
        strcat(checking_servers_data->string_ports, "/");
        free(str);
    }

}

void download_from_server(application_context *app_context, char *search_file_description, int offset, int download_size, int port) {

    int sockfd;
    struct sockaddr_in server_address, foreign_address;

    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
        perror("socket creation failed");
        return;
    }

    memset(&server_address, 0, sizeof(server_address));
    memset(&foreign_address, 0, sizeof(foreign_address));

//    int udp_port = UDP_PORT;
    int broadcast = 1;

    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;

    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        perror("socket timer failed");
        return;
    }

    if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast)) < 0) {
        perror("socket broadcast failed");
        return;
    }

    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = htonl(INADDR_BROADCAST);
    server_address.sin_port = htons(port);

    sendto(sockfd, search_file_description, strlen(search_file_description), MSG_CONFIRM,
           (const struct sockaddr *) &server_address, sizeof(server_address));

    int n, len;
    len = sizeof(foreign_address);

    int buffer[BUF_SIZE] = {0};
    memset(buffer, 0, BUF_SIZE);

    n = recvfrom(sockfd, (char *) buffer, BUF_SIZE, MSG_WAITALL, (struct sockaddr *) &foreign_address, &len);
    udp_answer *answer = (udp_answer *) buffer;

    if (answer->success_result) {
        char *str = malloc(BUF_SIZE);
        memset(str, 0, BUF_SIZE);
        sprintf(str, "File '%s' found on UDP server with port %d", answer->file_desc_send.name, htons(foreign_address.sin_port));
        print_log(str, F_BLUE);
        free(str);

        pthread_t *tcp_client_thread = (pthread_t *) malloc(sizeof(pthread_t));
        tcp_client_thread_description *tcp_client_td = malloc(sizeof(tcp_client_thread_description));
        tcp_client_td->app_context = app_context;
        tcp_client_td->file_desc_send = answer->file_desc_send;
        tcp_client_td->port = answer->port;
        tcp_client_td->address = foreign_address.sin_addr.s_addr;
        tcp_client_td->file_offset = offset;
        tcp_client_td->download_size = download_size;
        pthread_create(tcp_client_thread, NULL, (void *) start_tcp_client, tcp_client_td);
    } else {
    }

}