//
// Created by kirill
//

#include "ui.h"
#include <string.h>
#include <pthread.h>
#include <sys/select.h>
#include <unistd.h>
#include <termios.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <time.h>
#include "../file_controller/file_controller.h"
#include "../net/udp.h"

static struct termios stored_settings;

string_count *sc;
log_frame_description *log_desc;
progress_frame_description *df_desc;
progress_frame_description *uf_desc;
cmd_enter_description *cmd_desc;
pthread_mutex_t lock;

void set_keypress(void)
{
    struct termios new_settings;

    tcgetattr(0,&stored_settings);

    new_settings = stored_settings;

    new_settings.c_lflag &= (~ICANON & ~ECHO);
    new_settings.c_cc[VTIME] = 1;
    new_settings.c_cc[VMIN] = 1;

    tcsetattr(0,TCSANOW,&new_settings);

    return;
}

void reset_keypress(void)
{
    tcsetattr(0,TCSANOW,&stored_settings);
    return;
}

void start_ui(application_context *app_context) {

    sc = calloc(1, sizeof(string_count));
    log_desc = calloc(1, sizeof(log_frame_description));
    df_desc = calloc(1, sizeof(progress_frame_description));
    uf_desc = calloc(1, sizeof(progress_frame_description));
    cmd_desc = calloc(1, sizeof(cmd_desc));

    pthread_mutex_init(&lock, NULL);

    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);

    clrscr();
    home();

    set_display_atrib(BRIGHT);

    // Downloading and Uploading frame
    int subFrameWidth= (w.ws_col - 8) / 2;
    int subFrameHeight = (w.ws_row - 12) / 2;
    int downloadingXStart = 4;
    int downloadingYStart = 2;
    int uploadingXStart = downloadingXStart + subFrameWidth + 4;
    int uploadingYStart = downloadingYStart;

    gotoxy(downloadingXStart,downloadingYStart);
    printf("Downloading\n");
    print_horizontal_line("-", subFrameWidth, downloadingXStart, downloadingYStart + 1);

    gotoxy(uploadingXStart, uploadingYStart);
    printf("Uploading\n");
    print_horizontal_line("-", subFrameWidth, uploadingXStart, uploadingYStart + 1);

    print_vertical_line("|", subFrameHeight, downloadingXStart - 1, downloadingYStart + 2);
    print_vertical_line("|", subFrameHeight, downloadingXStart + subFrameWidth, downloadingYStart + 2);
    print_vertical_line("|", subFrameHeight, uploadingXStart - 1, uploadingYStart + 2);
    print_vertical_line("|", subFrameHeight, uploadingXStart + subFrameWidth, uploadingYStart + 2);

    print_horizontal_line("-", subFrameWidth, downloadingXStart, downloadingYStart + subFrameHeight + 1);
    print_horizontal_line("-", subFrameWidth, uploadingXStart, downloadingYStart + subFrameHeight + 1);

    df_desc->frameStartX = downloadingXStart + 2;
    df_desc->frameStartyY = downloadingYStart + 3;
    df_desc->max_string = subFrameHeight - 4;
    df_desc->frameWidth = subFrameWidth - 4;

    uf_desc->frameStartX = uploadingXStart + 2;
    uf_desc->frameStartyY = uploadingYStart + 3;
    uf_desc->max_string = subFrameHeight - 4;
    uf_desc->frameWidth = subFrameWidth - 4;

    //Action/events log frame
    int logXStart = downloadingXStart;
    int logYStart;
    int logFrameWidth = subFrameWidth * 2 + 4;
    int logFrameHeight = subFrameHeight + 2;
    cursor_down(2);
    set_cursor_column(logXStart);
    fflush(stdout);

    get_pos(&logYStart, &logXStart);
    printf("Action/events log\n");
    print_horizontal_line("-", logFrameWidth, logXStart, logYStart + 1);
    print_vertical_line("|", logFrameHeight , logXStart - 1, logYStart + 2);
    print_vertical_line("|", logFrameHeight , logXStart + logFrameWidth, logYStart + 2);
    print_horizontal_line("-", logFrameWidth, logXStart, logYStart + logFrameHeight + 1);

    log_desc->logStartX = logXStart + 2;
    log_desc->logStartY = logYStart + 3;
    log_desc->max_string = logFrameHeight - 4;

    //Command line frame
    int cmdXStart = logXStart;
    int cmdYStart;
    int cmdFrameWidth = logFrameWidth;
    int cmdFrameHeight = 2;
    cursor_down(2);
    set_cursor_column(cmdXStart);
    fflush(stdout);

    get_pos(&cmdYStart, &cmdXStart);
    printf("Command line\n");
    print_horizontal_line("-", cmdFrameWidth, cmdXStart, cmdYStart + 1);
    print_vertical_line("|", cmdFrameHeight , cmdXStart - 1, cmdYStart + 2);
    print_vertical_line("|", cmdFrameHeight , cmdXStart + cmdFrameWidth, cmdYStart + 2);
    print_horizontal_line("-", cmdFrameWidth, cmdXStart, cmdYStart + cmdFrameHeight + 1);

    cursor_up(1);
    set_cursor_column(cmdXStart);
    printf(" > ");
    fflush(stdout);

    get_pos(&cmd_desc->cmdEnterStartY, &cmd_desc->cmdEnterStartX);
    command_result cmd_res = {0};

    print_log(app_context->root_path, BRIGHT);

    app_context->ui_ready = 1;

    while (!app_context->exit_code) {
        resetcolor();
        cmd_res = process_command();
        if (strcmp(cmd_res.cmd, "show") == 0) {
            file_description *file_desc = search_file_description_by_name(app_context->list_fd_head, cmd_res.arg);
            char *str = malloc(BUF_SIZE);
            char *fd_str = malloc(BUF_SIZE);
            memset(str, 0, BUF_SIZE);
            memset(fd_str, 0, BUF_SIZE);
            if (file_desc != NULL) {
                strcpy(fd_str, "Found file description - ");
                sprintf(str, "%s/%s/%d", file_desc->name, file_desc->hash, file_desc->size);
                strcat(fd_str, str);
                print_log(fd_str, F_WHITE);
            } else if (strcmp(cmd_res.arg, "") == 0){
                print_log("Specify the filename", F_WHITE);
            } else {
                sprintf(str, "File '%s' not found", cmd_res.arg);
                print_log(str, F_WHITE);
            }
            free(str);
            free(fd_str);
        } else if (strcmp(cmd_res.cmd, "download") == 0) {
            if (strcmp(cmd_res.arg, "") == 0) {
                print_log("Specify the filename/hash/size", DIM);
            } else {
                udp_search_data *udp_sd = calloc(1, sizeof(udp_search_data));
                udp_sd->app_context = app_context;
                udp_sd->file_str = cmd_res.arg;

                pthread_t *udp_thread = malloc(sizeof(pthread_t));
                pthread_create(udp_thread, NULL, (void *) search_other_servers, udp_sd);
            }
        } else if (strcmp(cmd_res.cmd, "close") == 0) {
            app_context->exit_code = 1;
        } else if (strcmp(cmd_res.cmd, "") != 0) {
                char *str = malloc(BUF_SIZE);
                memset(str, 0, BUF_SIZE);
                sprintf(str, "Incorrect command '%s'. Try again", cmd_res.cmd);
                print_log(str, DIM);
                free(str);
        }
        gotoxy(cmd_desc->cmdEnterStartX, cmd_desc->cmdEnterStartY);
        clear_symbols(cmdFrameWidth - 4);
    }

}

command_result process_command() {
    char cmd[100];
    int count_space = 0;
    command_result cmd_res = {0};

    fgets(cmd, 100, stdin);
    cmd[strlen(cmd) - 1] = 0;

    for (int i = 0; i < strlen(cmd); i++) {
        if (cmd[i] == ' ') {
            count_space++;
        }
    }


    if (count_space > 1) {

    } else if (count_space == 0) {
        strcpy(cmd_res.cmd, cmd);
    } else {
        if (count_space == 1) {
            char *c;
            int index;
            c = strchr(cmd, ' ');
            index = (int) (c - cmd);
            char substr_cmd[30] = {0};
            char substr_arg[256] = {0};

            for (int i = 0; i < index; i++) {
                substr_cmd[i] = cmd[i];
            }

            int j = 0;
            for (int i = index + 1; i < strlen(cmd); i++) {
                substr_arg[j] = cmd[i];
                j++;
            }

            if (strcmp(substr_cmd, "download") || strcmp(substr_arg, "show")) {
                strcpy(cmd_res.cmd, substr_cmd);
                strcpy(cmd_res.arg, substr_arg);
            }
        } else if (strcmp(cmd, "close") == 0) {
            strcpy(cmd_res.cmd, "close");
        }

    }

    return cmd_res;
}

void print_download(char *filename, int *file_index) {
    if (sc->download_count + 1> df_desc->max_string) {
        sc->download_count = 0;
        clear_downloads();
    }

    save_cursor();
    gotoxy(df_desc->frameStartX, df_desc->frameStartyY + sc->download_count);
    char *str = malloc(100 * sizeof(char));
    memset(str, ' ', 100 * sizeof(char));
    strcpy(str, filename);
    for (int i = 0; i < df_desc->frameWidth * 0.4; i++) {
        printf("%c", str[i]);
    }
    restore_cursor();
    free(str);
    *file_index = sc->download_count;
    sc->download_count++;
    fflush(stdout);
}

void print_upload(char *filename, int *file_index) {
    if (sc->upload_count + 1> uf_desc->max_string) {
        sc->upload_count = 0;
        clear_uploads();
    }

    save_cursor();
    gotoxy(uf_desc->frameStartX, uf_desc->frameStartyY + sc->upload_count);
    char *str = malloc(100 * sizeof(char));
    memset(str, ' ', 100 * sizeof(char));
    strcpy(str, filename);
    for (int i = 0; i < uf_desc->frameWidth * 0.4; i++) {
        printf("%c", str[i]);
    }
    restore_cursor();
    free(str);
    *file_index = sc->upload_count;
    sc->upload_count++;
    fflush(stdout);
}

void update_download_progress(int size, int file_size, int percents, int file_index) {
    int offset = (int) df_desc->frameWidth * 0.8;
    char *str = malloc(BUF_SIZE);
    memset(str, 0, BUF_SIZE);
    sprintf(str, "|%d|%d|%d", size, percents, file_index);

    pthread_mutex_lock(&lock);
    save_cursor();
    gotoxy(df_desc->frameStartX + offset, df_desc->frameStartyY + file_index);
    clear_symbols(df_desc->frameWidth - offset);
    printf("%dM/%dM | %d%%/100%%", (int) size / 1024 / 1024, (int) file_size / 1024 / 1024, percents);
    restore_cursor();
    pthread_mutex_unlock(&lock);
    free(str);

    fflush(stdout);
}

void update_upload_progress(int current_size, int file_size, int percents, int file_index) {
    int offset = (int) uf_desc->frameWidth * 0.8;
    char *str = malloc(BUF_SIZE);
    memset(str, 0, BUF_SIZE);
    sprintf(str, "|%d|%d|%d", current_size, percents, file_index);

    save_cursor();
    gotoxy(uf_desc->frameStartX + offset, uf_desc->frameStartyY + file_index);
    clear_symbols(uf_desc->frameWidth - offset);
    printf("%dM/%dM | %d%%/100%%", (int) current_size / 1024 / 1024, (int) file_size / 1024 / 1024, percents);
    restore_cursor();
    free(str);

    fflush(stdout);
}

void print_log(char *log_str, int format) {
    if (sc->log_count + 1> log_desc->max_string) {
        sc->log_count = 0;
        clear_log();
    }

    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    set_display_atrib(format);

    if (format == F_BLUE || format == DIM) {
        set_display_atrib(BRIGHT);
    }

    pthread_mutex_lock(&lock);
    save_cursor();
    gotoxy(log_desc->logStartX, log_desc->logStartY + sc->log_count);
    printf("[%02d:%02d:%02d] %s", tm.tm_hour, tm.tm_min, tm.tm_sec, log_str);
    restore_cursor();
    resetcolor();
    pthread_mutex_unlock(&lock);
    sc->log_count++;
    fflush(stdout);
}

void clear_log() {
    for (int i = 0; i < log_desc->max_string + 1; i++) {
        gotoxy(log_desc->logStartX, log_desc->logStartY + i);
        clear_symbols(150);
    }
}

void clear_downloads() {
    for (int i = 0; i < df_desc->max_string + 1; i++) {
        gotoxy(df_desc->frameStartX, df_desc->frameStartyY + i);
        clear_symbols(df_desc->frameWidth);
    }
}

void clear_uploads() {
    for (int i = 0; i < uf_desc->max_string + 1; i++) {
        gotoxy(uf_desc->frameStartX, uf_desc->frameStartyY + i);
        clear_symbols(uf_desc->frameWidth);
    }
}

void print_horizontal_line(char symbol[1], int count, int startX, int y) {
    gotoxy(startX, y);
    for (int i = 0; i < count; i++) {
        printf("%c", symbol[0]);
    }
    fflush(stdout);
}

void print_vertical_line(char symbol[1], int count, int x, int startY) {
    int y = startY;
    for (int i = 0; i < count - 1; i++) {
        gotoxy(x, y);
        printf("%c\n", symbol[0]);
        y++;
    }
}

int get_pos(int *y, int *x) {

    char buf[30]={0};
    int ret, i, pow;
    char ch;

    *y = 0; *x = 0;

    struct termios term, restore;

    tcgetattr(0, &term);
    tcgetattr(0, &restore);
    term.c_lflag &= ~(ICANON|ECHO);
    tcsetattr(0, TCSANOW, &term);

    write(1, "\033[6n", 4);

    for( i = 0, ch = 0; ch != 'R'; i++ )
    {
        ret = read(0, &ch, 1);
        if ( !ret ) {
            tcsetattr(0, TCSANOW, &restore);
            return 1;
        }
        buf[i] = ch;
    }

    if (i < 2) {
        tcsetattr(0, TCSANOW, &restore);
        return(1);
    }

    for( i -= 2, pow = 1; buf[i] != ';'; i--, pow *= 10)
        *x = *x + ( buf[i] - '0' ) * pow;

    for( i-- , pow = 1; buf[i] != '['; i--, pow *= 10)
        *y = *y + ( buf[i] - '0' ) * pow;

    tcsetattr(0, TCSANOW, &restore);
    return 0;
}