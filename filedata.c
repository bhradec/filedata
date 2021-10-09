#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <argp.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <linux/kdev_t.h>

#define OUTPUT_COLOR_RED "\033[0;31m"
#define OUTPUT_COLOR_NORMAL "\033[0m"

#define MAX_PATH_CHARS 4096

const char *argp_program_version = "filedata 1.0";
const char *argp_program_bug_address = "<bhradec@gmail.com>";

static char doc[] ="filedata -- outputs data about filesystem and innodes";
static char args_doc[] = "INPUT_FILE_PATH";

static struct argp_option options[] = {{0}};

struct arguments {
    char *args[1]; /* Input file path is the only arg. */
};

static int parse_opt(int key, char *arg, struct argp_state *state) {
    struct arguments *arguments = state->input;

    switch (key) {
        case ARGP_KEY_ARG:
            /* Check if too many args without a flag. */
            if (state->arg_num > 1) { 
                argp_usage(state); 
            }
            /* Input file path is the only arg. It is 
             * stored in the args array for the args 
             * without flags. */
            arguments->args[state->arg_num] = arg;
            break;
        case ARGP_KEY_END:
            /* Check if not enough args. */
            if (state->arg_num < 1) { 
                argp_usage(state); 
            }
            break;
        default:
            return ARGP_ERR_UNKNOWN;
    }

    return 0;
}

static struct argp argp = {options, parse_opt, args_doc, doc};

void print_error(char *message, int print_errno) {
    fprintf(stderr, OUTPUT_COLOR_RED);
    fprintf(stderr, "Error: %s\n", message);

    if (print_errno != 0) {
        fprintf(stderr, "Descr: %s (errno: %d)\n", strerror(errno), errno);
    }

    fprintf(stderr, OUTPUT_COLOR_NORMAL);
}

struct partitions_row {
    int major;
    int minor;
    int blocks;
    char path[MAX_PATH_CHARS + 1];
};

char *get_filesystem_path(int device_id) {
    int device_major;
    int device_minor;
    char* filesystem_path = NULL;
    
    FILE* partitions_file;
    struct partitions_row row;

    filesystem_path = malloc(MAX_PATH_CHARS + 1);

    partitions_file = fopen("/proc/partitions", "r");

    device_major = MAJOR(device_id);
    device_minor = MINOR(device_id);

    /* Skipping first line of the partitions file. */
    fscanf(partitions_file, "%*[^\n]\n");

    while(!feof(partitions_file)) {
        fscanf(partitions_file, "%d", &(row.major));
        fscanf(partitions_file, "%d", &(row.minor));
        fscanf(partitions_file, "%d", &(row.blocks));
        fscanf(partitions_file, "%s\n", row.path);

        if (device_major == row.major && device_minor == row.minor) {
            filesystem_path = malloc(MAX_PATH_CHARS + 1);
            strcpy(filesystem_path, row.path);
        }
    }

    return filesystem_path;
}

int main(int argc, char **argv) {
    int fd;
    int device_id;
    int inode_number;
    int file_size;
    int block_size;
    char *file_path;
    char *filesystem_path;

    struct arguments arguments;
    struct stat file_stat;
    
    argp_parse(&argp, argc, argv, 0, 0, &arguments);

    file_path = arguments.args[0];

    fd = open(file_path, O_RDONLY);

    if (fd < 0) {
        print_error("Can't open file", 1); 
        return -1;
    }

    if (fstat(fd, &file_stat) < 0) {
        print_error("Can't get file stats", 1);
        return -1;
    }

    device_id = file_stat.st_dev;
    inode_number = file_stat.st_ino;
    file_size = file_stat.st_size;
    block_size = file_stat.st_blksize;
    filesystem_path = get_filesystem_path(device_id);

    if (filesystem_path == NULL) {
        print_error("Can't get filesystem path", 0);
        return -1;
    }

    printf("File path: %s\n", file_path);
    printf("Device id: %d\n", device_id);
    printf("File size : %dB\n", file_size);
    printf("Device block size: %dB\n", block_size);
    printf("Inode number: %d\n", inode_number);
    printf("Filesystem path: %s\n", filesystem_path);

    free(filesystem_path);

    close(fd);

    return 0;
}