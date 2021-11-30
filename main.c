#include <stdio.h>
#include <stdlib.h>
#include "file_controller/file_controller.h"
#include "net/udp.h"
#include <pthread.h>
#include "string.h"
#include "application_context.h"
#include "ui/ui.h"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Please, enter just path as program argument");
    }

    else {

        if (argv[1][strlen(argv[1]) - 1] != '/') {
            strcat(argv[1], "/");
        }

        application_context *app_context = calloc(1, sizeof(application_context));
        file_description_node *list_fd_head = malloc(sizeof(file_description_node));
        
        app_context->list_fd_head = list_fd_head;
        app_context->exit_code = 0;
        app_context->root_path = argv[1];
        scan_all_directories(list_fd_head, argv[1]);


        pthread_t *udp_thread = malloc(sizeof(pthread_t));
        pthread_create(udp_thread, NULL, (void *) start_udp_listener, app_context);

        start_ui(app_context);

        pthread_join(*udp_thread, NULL);

        home();
        clrscr();
        free(list_fd_head);
    }
    return 0;
}
