//
// Created by kirill
//

#ifndef LR3_NET_UI_H
#define LR3_NET_UI_H

#include "termios.h"
#include "../application_context.h"
#include "esc.h"

typedef struct command_result {
    char cmd[30];
    char arg[256];
} command_result;

typedef struct string_count {
    int download_count;
    int upload_count;
    int log_count;
} string_count;

typedef struct log_frame_description {
    int logStartX;
    int logStartY;
    int max_string;
} log_frame_description;

typedef struct progress_frame_description {
    int frameStartX;
    int frameStartyY;
    int frameWidth;
    int max_string;
} progress_frame_description;

typedef struct cmd_enter_description {
    int cmdEnterStartX;
    int cmdEnterStartY;
} cmd_enter_description;

void start_ui(application_context *app_context);
void print_horizontal_line(char symbol[1], int count, int startX, int y);
void print_vertical_line(char symbol[1], int count, int x, int startY);
int get_pos(int *y, int *x);

command_result process_command();

void print_log(char *log_str, int format);
void clear_log();

void print_download(char *filename, int *file_index);
void update_download_progress(int current_size, int file_size, int percents, int file_index);
void clear_downloads();

void print_upload(char *filename, int *file_index);
void update_upload_progress(int current_size, int file_size, int percents, int file_index);
void clear_uploads();

#endif //LR3_NET_UI_H
