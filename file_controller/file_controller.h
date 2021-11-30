//
// Created by kirill
//

#ifndef LR3_NET_FILE_CONTROLLER_H
#define LR3_NET_FILE_CONTROLLER_H

#include "stddef.h"

typedef struct file_description_node file_description_node;

typedef struct file_description {
    char *name;
    int size;
    char *hash;
    char *path;
} file_description;

typedef struct file_description_send {
    char name[256];
    char hash[512];
    int size;
    char path[512];
} file_description_send;

struct file_description_node {
    file_description *file_desc_entry;
    file_description_node *file_desc_node_next;
};

void push_fd_by_head(file_description_node *list_fd_head, file_description *fd);
void push_fd(file_description_node **fd_node_current, file_description *fd);
void print_list(file_description_node *fd_node_head);

void scan_all_directories(file_description_node *fd_node_head, char *root_path);
void scan_dir(file_description_node **fd_node_current, char *path);
int calculate_file_size(char *path);
char* calculate_file_hash(char *path);
file_description* find_file_description(file_description_node *fd_list_head, char *search_file_desc);
file_description* search_file_description_by_name(file_description_node *fd_list_head, char *search_file_desc);

#endif //LR3_NET_FILE_CONTROLLER_H
