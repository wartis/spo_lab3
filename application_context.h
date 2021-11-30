//
// Created by kirill
//

#ifndef LR3_NET_APPLICATION_CONTEXT_H
#define LR3_NET_APPLICATION_CONTEXT_H

#include "file_controller/file_controller.h"

#define BUF_SIZE 1024

typedef struct application_context {
    file_description_node *list_fd_head;
    int instance_count;
    int exit_code;
    char *root_path;
    int ui_ready;
} application_context;

#endif //LR3_NET_APPLICATION_CONTEXT_H
