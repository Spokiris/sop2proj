#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#include "common/constants.h"
#include "common/io.h"
#include "operations.h"

#include <pthread.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <string.h>

#include <fcntl.h>
#include <sys/stat.h>

#define MAX_SESSIONS 1
#define PATH_MAX 256

typedef struct
{
    int client_id;
} client_t;

int sessions = 0;
int server_fd;

void create_named_pipe(const char *pipe_name)
{
    if (mkfifo(pipe_name, 0666) == -1)
    {
        perror("Erro ao criar o named pipe");
        exit(EXIT_FAILURE);
    }
}

void cli_init(int sv_fd)
{
    int op_code;

    const char con_setup_req[40];
    const char con_setup_resp[40];

    read(sv_fd, &op_code, 4);
    if (op_code == 1)
    {

        // Read the first string
        if (read(sv_fd, con_setup_req, 40) == -1)
        {
            perror("read setup request data");
            return;
        }

        if (read(sv_fd, con_setup_resp, 40) == -1)
        {
            perror("read setup request data");
            return;
        }

        int request_fd = open("../req", 0666);

        if (request_fd == -1)
        {
            perror("open request pipe");
            return;
        }

        int response_fd = open(&con_setup_resp, 0666);

        if (response_fd == -1)
        {
            perror("open response pipe");
            return;
        }
        sessions++;
        int client_id = sessions;

        if (write(server_fd, &sessions, sizeof(int)) == -1)
        {
            perror("write setup response");
            return;
        }

        while (1)
        {

            process_client_request(request_fd, response_fd);
        }
    }
}

int process_client_request(int request_fd, int response_fd)
{
    char op_code;
    unsigned int event_id;
    size_t num_rows, num_cols;
    size_t num_seats;

    while (read(request_fd, &op_code, sizeof(int)) != sizeof(int))
    {
        continue;
    }

    switch (op_code)
    {
    case 2: // Quit
        close(request_fd);
        close(response_fd);
        return 1;
    case 3: // Create

        if (read(request_fd, &event_id, sizeof(unsigned int)) == -1 ||
            read(request_fd, &num_rows, sizeof(size_t)) == -1 ||
            read(request_fd, &num_cols, sizeof(size_t)) == -1)
        {
            perror("read create request data");
            return 0;
        }
        // Realizar a operação de criação de evento e enviar a resposta
        int create_result = ems_create(event_id, num_rows, num_cols);
        if (write(response_fd, &create_result, sizeof(int)) == -1)
        {
            perror("write create response");
            return 0;
        }
        return 0;
    case 4: // Reserve

        if (read(request_fd, &event_id, sizeof(unsigned int)) == -1 ||
            read(request_fd, &num_seats, sizeof(size_t)) == -1)
        {
            perror("read reserve request data");
            return 0;
        }
        size_t *xs = malloc(num_seats * sizeof(size_t));
        size_t *ys = malloc(num_seats * sizeof(size_t));
        if (read(request_fd, xs, num_seats * sizeof(size_t)) == -1 ||
            read(request_fd, ys, num_seats * sizeof(size_t)) == -1)
        {
            perror("read reserve request data");
            return 0;
        }

        int reserve_result = ems_reserve(event_id, num_seats, xs, ys);
        if (write(response_fd, &reserve_result, sizeof(int)) == -1)
        {
            perror("write reserve response");
            return 0;
        }
        free(xs);
        free(ys);
        return 0;
    case 5: // Show

        if (read(request_fd, &event_id, sizeof(unsigned int)) == -1)
        {
            perror("read show request data");
            return 0;
        }

        int show_result = ems_show(response_fd, event_id);
        if (write(response_fd, &show_result, sizeof(int)) == -1)
        {
            perror("write show response");
            return 0;
        }

    case 6: // List Events

        if (read(request_fd, &event_id, sizeof(unsigned int)) == -1)
        {
            perror("read list events request data");
            return 0;
        }
        int list_result = ems_list_events(response_fd);
        if (write(response_fd, &list_result, sizeof(int)) == -1)
        {
            perror("write list events response");
            return 0;
        }
        return 0;
    default:
        perror("Invalid operation code");
        return 0;
    }
}

int main(int argc, char *argv[])
{
    if (argc < 2 || argc > 3)
    {
        fprintf(stderr, "Usage: %s\n <pipe_path> [delay]\n", argv[0]);
        return 1;
    }

    char *endptr;
    unsigned int state_access_delay_us = STATE_ACCESS_DELAY_US;
    if (argc == 3)
    {
        unsigned long int delay = strtoul(argv[2], &endptr, 10);

        if (*endptr != '\0' || delay > UINT_MAX)
        {
            fprintf(stderr, "Invalid delay value or value too large\n");
            return 1;
        }

        state_access_delay_us = (unsigned int)delay;
    }

    if (ems_init(state_access_delay_us))
    {
        fprintf(stderr, "Failed to initialize EMS\n");
        return 1;
    }

    const char *sv_pipe_name = argv[1];

    create_named_pipe(sv_pipe_name);

    server_fd = open(sv_pipe_name, 0666);

    if (server_fd == -1)
    {
        perror("Erro ao abrir o named pipe");
        exit(EXIT_FAILURE);
    }
    cli_init(server_fd);

    ems_terminate();
}