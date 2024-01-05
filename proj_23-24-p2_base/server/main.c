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

#define PIPE_BUF 40
#define MAX_SESSIONS 1

void create_named_pipe(const char *pipe_name) {
    if (mkfifo(pipe_name, 0666) == -1) {
        perror("Erro ao criar o named pipe");
        exit(EXIT_FAILURE);
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
  
  
  int client_fd = open(sv_pipe_name, 0666);
  char request[PIPE_BUF];



  // Código para lidar com as solicitações dos clientes
  int num_sessions = 0;


  //TODO: Intialize server, create worker threads
 
 int rx = open(sv_pipe_name, 0444);
  if (rx == -1) {
    perror("Erro ao abrir o named pipe");
    exit(EXIT_FAILURE);
  }
 
 
 while (1) {
    //TODO: Read from pipe
    char buffer[PIPE_BUF];
    ssize_t ret = read(rx, buffer, PIPE_BUF - 1);

    if(ret == 0) {
      //EOF
      break;
    } else if (ret == -1) {
      perror("Erro ao ler do named pipe");
      exit(EXIT_FAILURE);
    }



    //TODO: Write new client to the producer-consumer buffer


  }

  //TODO: Close Server

  ems_terminate();
}