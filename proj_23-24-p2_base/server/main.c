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

#define PIPE_BUF 40
#define MAX_SESSIONS 1

typedef struct {
    int client_id;
    int request_fd;
    int response_fd;
    const char *req_pipe_name;
    const char *resp_pipe_name;
} client_t;


client_t sessions[MAX_SESSIONS];
int num_sessions = sizeof(sessions) / sizeof(client_t);

int server_fd;




void create_named_pipe(const char *pipe_name) {
    if (mkfifo(pipe_name, 0666) == -1) {
        perror("Erro ao criar o named pipe");
        exit(EXIT_FAILURE);
    }
}

void cli_init(int sv_fd) {
  client_t client;
  if(read(sv_fd, &client.req_pipe_name, sizeof(char)) == -1 || read(sv_fd, &client.resp_pipe_name, sizeof(char)) == -1){
      perror("read setup request data");
      return;
  }

    printf("Request pipe name: %s\n", client.req_pipe_name);
    printf("Response pipe name: %s\n", client.resp_pipe_name);
  if(mkfifo(client.req_pipe_name, O_WRONLY) == -1 || mkfifo(client.resp_pipe_name, O_RDONLY) == -1){
      perror("Erro ao criar o named pipe");
      exit(EXIT_FAILURE);
  }

  client.request_fd = open(client.req_pipe_name, O_WRONLY);
  if (client.request_fd == -1) {
      perror("open request pipe");
      return;
  }

  client.response_fd = open(client.resp_pipe_name, O_RDONLY);
  if (client.response_fd == -1) {
      perror("open response pipe");
      return;
  }

  client.client_id = num_sessions;
  sessions[num_sessions] = client; //MAYBEFIXME

  if (write(client.response_fd, &client.client_id, sizeof(int)) == -1) {
      perror("write setup response");
      return;
  }
  num_sessions++;
  while (1) {
    if(process_client_request(client.request_fd, client.response_fd)){
      exit(EXIT_SUCCESS);
    }
  }
}



int process_client_request(int request_fd, int response_fd) {
    char op_code;
    unsigned int event_id;
    size_t num_rows, num_cols;
    size_t num_seats;
    // Ler o código da operação
    if (read(request_fd, &op_code, sizeof(char)) == -1) {
        perror("read op_code");
        return 0;
    }

    // Dependendo do código da operação, ler os dados relevantes do pedido do cliente
    switch (op_code) {
        case 2: // Quit
          close(request_fd);
          close(response_fd);
            return 1;
        case 3: // Create
            // Ler os dados do pedido de create
            if (read(request_fd, &event_id, sizeof(unsigned int)) == -1 ||
                read(request_fd, &num_rows, sizeof(size_t)) == -1 ||
                read(request_fd, &num_cols, sizeof(size_t)) == -1) {
                perror("read create request data");
                return 0;
            }
            // Realizar a operação de criação de evento e enviar a resposta
            // Exemplo:
            int create_result = ems_create(event_id, num_rows, num_cols); // Use a lógica da sua função ems_create
            if (write(response_fd, &create_result, sizeof(int)) == -1) {
                perror("write create response");
                return 0;
            }
            return 0;
        case 4: // Reserve

            if (read(request_fd, &event_id, sizeof(unsigned int)) == -1 ||
                read(request_fd, &num_seats, sizeof(size_t)) == -1) {
                perror("read reserve request data");
                return 0;
            }
            size_t *xs = malloc(num_seats * sizeof(size_t));
            size_t *ys = malloc(num_seats * sizeof(size_t));
            if (read(request_fd, xs, num_seats * sizeof(size_t)) == -1 ||
                read(request_fd, ys, num_seats * sizeof(size_t)) == -1) {
                perror("read reserve request data");
                return 0;
            }

            int reserve_result = ems_reserve(event_id, num_seats, xs, ys); 
            if (write(response_fd, &reserve_result, sizeof(int)) == -1) {
                perror("write reserve response");
                return 0;
            }
            return 0;
        case 5: // Show

            if (read(request_fd, &event_id, sizeof(unsigned int)) == -1) {
                perror("read show request data");
                return 0;
            }

            int show_result = ems_show(response_fd, event_id); // Use a lógica da sua função ems_show
            if (write(response_fd, &show_result, sizeof(int)) == -1) {
                perror("write show response");
                return 0;
            }

        case 6: // List Events
            // Implementar lógica para lidar com o pedido de listagem de eventos
            if(read(request_fd, &event_id, sizeof(unsigned int)) == -1){
                perror("read list events request data");
                return 0;
            }
            int list_result = ems_list_events(response_fd);
            if(write(response_fd, &list_result, sizeof(int)) == -1){
                perror("write list events response");
                return 0;
            }
            return 0;
        default:
            perror("Invalid operation code");
            return 0;
    }
}


int main(int argc, char* argv[]) {
  if (argc < 2 || argc > 3) {
    fprintf(stderr, "Usage: %s\n <pipe_path> [delay]\n", argv[0]);
    return 1;
  }

  char* endptr;
  unsigned int state_access_delay_us = STATE_ACCESS_DELAY_US;
  if (argc == 3) {
    unsigned long int delay = strtoul(argv[2], &endptr, 10);

    if (*endptr != '\0' || delay > UINT_MAX) {
      fprintf(stderr, "Invalid delay value or value too large\n");
      return 1;
    }

    state_access_delay_us = (unsigned int)delay;
  }

  if (ems_init(state_access_delay_us)) {
    fprintf(stderr, "Failed to initialize EMS\n");
    return 1;
  }

  const char *sv_pipe_name = argv[1];

  // Criando o named pipe
  create_named_pipe(sv_pipe_name);
  

  server_fd = open(sv_pipe_name, 0666);
  char request[PIPE_BUF];
  memset(request, '0', PIPE_BUF);
  
  while (read(server_fd, request, sizeof(int)) != sizeof(int)) {
    continue;
    }
    printf("Received request\n");
  cli_init(server_fd);


    //TODO: Read from pipe
    
    //TODO: Write new client to the producer-consumer buffer



  //TODO: Close Server

  ems_terminate();
}