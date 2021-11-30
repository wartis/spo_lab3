//
// Created by kirill
//

#include "tcp.h"
#include "../file_controller/file_controller.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <unistd.h>
#include "../ui/ui.h"

#define TCP_PORT 11111

void create_tcp_socket(tcp_description *td) {
    int sockfd;
    struct sockaddr_in server_address;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("error when creating tcp socket");
        return;
    }

    memset(&server_address, 0, sizeof(server_address));

    int tcp_port = TCP_PORT;

    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    server_address.sin_port = htons(tcp_port);

    while (bind(sockfd, (const struct sockaddr*) &server_address, sizeof(server_address)) < 0) {
        server_address.sin_port = htons(++tcp_port);
    }

    td->tcp_port = tcp_port;
    td->sockfd = sockfd;
    td->server_address = server_address;
}

void start_tcp_server(tcp_server_thread_description *tcp_server_thread_description) {
    int connfd, len;

    tcp_description *td = tcp_server_thread_description->tcp_description;
    file_description *fd = tcp_server_thread_description->file_desc;

    if ((listen(td->sockfd, 5)) < 0) {
        perror("error when listening");
        return;
    }

    char *str = malloc(BUF_SIZE);
    memset(str, 0, BUF_SIZE);
    sprintf(str, "Started TCP server with port %d", td->tcp_port);
    print_log(str, F_BLUE);
    free(str);

    len = sizeof(td->client_address);

    connfd = accept(td->sockfd, (struct sockaddr *) &td->client_address, (socklen_t *) &len);
    if (connfd < 0) {
        perror("accept fail");
        return;
    }


    char cmd[3];
    int file = open(fd->path, O_RDONLY);
    int offset = 0;
    int upload_file_index = 0;

    strncpy(cmd, "xxx", 3);
    tcp_server_file_answer file_answer;
    tcp_client_progress_data progress_data;
    tcp_client_download_data download_data;

    print_upload(fd->name, &upload_file_index);

    while (strncmp(cmd, "end", 3) != 0) {
        read(connfd, &download_data, sizeof(tcp_client_download_data));
        strncpy(cmd, download_data.cmd, 3);
        if (strncmp(cmd, "dwn", sizeof(cmd)) == 0) {
            int size = 4096;
            if (size*offset + 4096 > download_data.file_part_size) {
                size = download_data.file_part_size - size * offset;
            }

            file_answer.file_part_len = size;
            pread(file, file_answer.file_part, size, download_data.file_part_offset + 4096 * offset);
            offset++;
            write(connfd, &file_answer, sizeof(file_answer));

            read(connfd, &progress_data, sizeof(tcp_client_progress_data));
            update_upload_progress(progress_data.current_size, download_data.file_part_size, progress_data.current_percents, upload_file_index);
        } else if (strncmp(cmd, "end", 3) == 0) {
            update_upload_progress(download_data.file_part_size, download_data.file_part_size, 100, upload_file_index);
        }
    }

    str = malloc(BUF_SIZE);
    memset(str, 0, BUF_SIZE);
    sprintf(str, "Uploading file '%s' finished", fd->name);
    print_log(str, F_BLUE);
    free(str);

}

void start_tcp_client(tcp_client_thread_description *tcp_client_thread_description) {
    int sockfd;
    struct sockaddr_in server_address;
    application_context *app_context = tcp_client_thread_description->app_context;
    file_description *fd = calloc(1, sizeof(file_description));
    fd->size = tcp_client_thread_description->file_desc_send.size;
    fd->name = calloc(1, 256);
    fd->hash = calloc(1, 512);
    fd->path = calloc(1, 512);
    strcpy(fd->name, tcp_client_thread_description->file_desc_send.name);
    strcpy(fd->hash, tcp_client_thread_description->file_desc_send.hash);
    strcpy(fd->path, tcp_client_thread_description->file_desc_send.path);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
        perror("socket creation failed");

    memset(&server_address, 0, sizeof(server_address));

    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = tcp_client_thread_description->address;
    server_address.sin_port = htons(tcp_client_thread_description->port);

    if (connect(sockfd, (const struct sockaddr*) &server_address, sizeof(server_address)) < 0) {
        perror("\nconnection with server failed");
        return;
    } else {
//        printf("Connected to the server\n");
    }

    char *str = malloc(BUF_SIZE);
    memset(str, 0, BUF_SIZE);
    sprintf(str, "Connected to the server with TCP port %d", tcp_client_thread_description->port);
    print_log(str, F_BLUE);
    free(str);

    int download_file_index = 0;

    char *file_path = calloc(1, 1024);

    strcpy(file_path, app_context->root_path);
    strcat(file_path, "/");
    strcat(file_path, fd->name);

    int file = open(file_path, O_CREAT | O_WRONLY, 0777);

    if (tcp_client_thread_description->file_offset == 0) {
        print_download(fd->name, &download_file_index);
    } else {
        char *str = malloc(BUF_SIZE);
        memset(str, 0, BUF_SIZE);
        sprintf(str, "-- %s", fd->name);
        print_download(str, &download_file_index);
        free(str);
    }

    if (file >= 0) {
        if (tcp_client_thread_description->file_offset == 0) {
            push_fd_by_head(app_context->list_fd_head, fd);
        }

        int offset = 0;
        int current_size = 0;
        tcp_server_file_answer file_answer;
        tcp_client_progress_data progress_data;
        tcp_client_download_data download_data;
        int file_part_size = tcp_client_thread_description->download_size;

        strncpy(download_data.cmd, "dwn", 3);

        while (1) {
            int current_percents = (int) ((((float) current_size) / file_part_size) * 100);

            update_download_progress(current_size, file_part_size, current_percents, download_file_index);

            if (current_size < file_part_size) {
                strncpy(download_data.cmd, "dwn", 3);
                download_data.file_part_offset = tcp_client_thread_description->file_offset;
                download_data.file_part_size = file_part_size;

                write(sockfd, &download_data, sizeof(tcp_client_download_data));
                read(sockfd, &file_answer, sizeof(file_answer));
                pwrite(file, &file_answer.file_part, file_answer.file_part_len, tcp_client_thread_description->file_offset + offset * 4096);
                current_size += file_answer.file_part_len;
                offset++;
            } else {
                strncpy(download_data.cmd, "end", 3);
                write(sockfd, &download_data, sizeof(tcp_client_download_data));
                char *str = malloc(BUF_SIZE);
                memset(str, 0, BUF_SIZE);

                sprintf(str, "Downloading file '%s' finished", fd->name);
                print_log(str, F_BLUE);
                free(str);
                return;
            }

            progress_data.current_size = current_size;
            progress_data.current_percents = current_percents;
            write(sockfd, &progress_data, sizeof(tcp_client_progress_data));

        }
    } else {
//        perror("creating file failed");
    }


}
