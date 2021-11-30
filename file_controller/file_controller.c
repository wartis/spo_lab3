//
// Created by kirill
//
#include "file_controller.h"
#include "dirent.h"
#include "sys/types.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <openssl/md5.h>

void print_list(file_description_node *fd_node_head) {
    file_description_node *current = fd_node_head;

    int i = 0;
    while (current->file_desc_node_next != NULL) {
        i++;
        printf("file %d: %s/%d/%s\n", i, current->file_desc_entry->name, current->file_desc_entry->size, current->file_desc_entry->hash);
        current = current->file_desc_node_next;
    }

}

void push_fd(file_description_node **fd_node_current, file_description *fd) {
    (*fd_node_current)->file_desc_entry = fd;
    (*fd_node_current)->file_desc_node_next = (file_description_node *) malloc(sizeof(file_description_node));
    (*fd_node_current) = (*fd_node_current)->file_desc_node_next;
    (*fd_node_current)->file_desc_node_next = NULL;
}

void push_fd_by_head(file_description_node *list_fd_head, file_description *fd) {
    file_description_node *current = list_fd_head;

    while (current->file_desc_node_next != NULL) {
        current = current->file_desc_node_next;
    }

    push_fd(&current, fd);
}

void scan_all_directories(file_description_node *fd_node_head, char *root_path) {
    fd_node_head->file_desc_node_next = NULL;

    file_description_node *fd_node_current = fd_node_head;

    scan_dir(&fd_node_current, root_path);

}

void scan_dir(file_description_node **fd_node_current, char *path) {

    DIR *dir;
    struct dirent *entry;
    size_t len = strlen(path);

    if (!(dir = opendir(path))) {
        printf("Unable to open directory\n");
        exit(0);
    }

    while ((entry = readdir(dir)) != NULL) {
        char *name = entry->d_name;
        if (entry->d_type == DT_DIR) {
            if ((strcmp(name, ".") == 0) || (strcmp(name, "..") == 0))
                continue;

            if (strcmp(&path[len-1], "/") != 0) {
                strcat(path, "/");
            }

            strcat(path, name);
            scan_dir(fd_node_current, path);
            path[len-1] = '\0';
        } else if (entry->d_type == DT_REG) {
            file_description *fd = calloc(1, sizeof(file_description));
            fd->name = calloc(1, 256);
            fd->path = calloc(1, 512);
            strcpy(fd->name, name);

            char filepath[256];
            strcpy(filepath, path);

            if (strcmp(&filepath[len-1], "/") != 0) {
                strcat(filepath, "/");
            }

            strcat(filepath, name);

            fd->size = calculate_file_size(filepath);
            fd->hash = calculate_file_hash(filepath);
            strcpy(fd->path, filepath);
            push_fd(fd_node_current, fd);
        }
    }
    closedir(dir);
}

int calculate_file_size(char *path) {
    FILE *file = fopen(path, "r");
    fseek(file, 0, SEEK_END);
    int size_file = ftell(file);
    fclose(file);

    return size_file;
}

char* calculate_file_hash(char *path) {
    unsigned char *c = calloc(1, MD5_DIGEST_LENGTH);
    char *hash = calloc(1, MD5_DIGEST_LENGTH*2);
    char *buf = calloc(1, MD5_DIGEST_LENGTH);

    int i;
    FILE *inFile = fopen (path, "rb");
    MD5_CTX mdContext;
    int bytes;
    unsigned char data[1024];

    if (inFile == NULL) {
//        printf ("%s can't be opened.\n", path);
        return 0;
    }

    MD5_Init (&mdContext);
    while ((bytes = fread (data, 1, 1024, inFile)) != 0)
        MD5_Update (&mdContext, data, bytes);

    MD5_Final (c,&mdContext);

    for(i = 0; i < MD5_DIGEST_LENGTH; i++) {
        sprintf(buf, "%02x", (unsigned char)c[i]);
        strcat(hash, buf);
    }

    fclose (inFile);
    free(c);
    free(buf);

    return hash;
}

file_description *search_file_description_by_name(file_description_node *fd_list_head, char *search_file_desc) {
    file_description_node *current_node = fd_list_head;

    int i = 0;
    while (current_node->file_desc_node_next != NULL) {
        char *current_file_desc = calloc(1, 1024);

        file_description *fd = current_node->file_desc_entry;
        strcat(current_file_desc, fd->name);

        if (!strcmp(current_file_desc, search_file_desc)) {

            free(current_file_desc);
            return fd;
        }

        current_node = current_node->file_desc_node_next;
    }

    return NULL;
}

file_description *find_file_description(file_description_node *fd_list_head, char *search_file_desc) {
    file_description_node *current_node = fd_list_head;

    int i = 0;
    while (current_node->file_desc_node_next != NULL) {
        char *current_file_desc = calloc(1, 1024);

        file_description *fd = current_node->file_desc_entry;
        strcat(current_file_desc, fd->name);
        strcat(current_file_desc, "/");
        strcat(current_file_desc, fd->hash);
        char buf[30];
        sprintf(buf, "%d", fd->size);
        strcat(current_file_desc, "/");
        strcat(current_file_desc, buf);

        if (!strcmp(current_file_desc, search_file_desc)) {

            free(current_file_desc);
            return fd;
        }

        current_node = current_node->file_desc_node_next;
    }

    return NULL;
}