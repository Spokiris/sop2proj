#include "api.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

int request_fd, response_fd, server_fd;
int session_id;

int ems_setup(char const* req_pipe_path, char const* resp_pipe_path, char const* server_pipe_path) {
  // Open the server pipe for writing
  server_fd = open(server_pipe_path , 0666);
  if (server_fd == -1) {
    perror("open server pipe");
    return 1;
  }

  // Create the request and response pipes
  if(mkfifo(req_pipe_path, 0666) == -1 || mkfifo(resp_pipe_path, 0666) == -1){
      perror("Erro ao criar o named pipe");
      exit(1);
  }
  // Send a setup request to the server
  int op_code = 1;
  if (write(server_fd, &op_code, 4) == -1 || write(server_fd, req_pipe_path, 40) == -1 ||
      write(server_fd, resp_pipe_path, 40) == -1) {
    perror("write setup request");
    return 1;
  }



  // Read the session_id from the response pipe
  if (read(server_fd, &session_id, sizeof(int)) == -1) {
    perror("read session_id");
    return 1;
  }
 
  request_fd = open(req_pipe_path, 0666);
  response_fd = open(resp_pipe_path, 0666);
  return 0;
}

int ems_quit(void) {
  
  int op_code = 2;

  if (write(request_fd, &op_code, sizeof(int)) == -1) {
    perror("write setup request");
    return 0;
  } else if (close(request_fd) == -1 || close(response_fd) == -1) {
       
    perror("Error closing the pipe");
    return 0;
    
  }

  return 1;
}

int ems_create(unsigned int event_id, size_t num_rows, size_t num_cols) {
  //TODO: send create request to the server (through the request pipe) and wait for the response (through the response pipe)

  int op_code = 3;  

  if (write(request_fd, &op_code, sizeof(int)) == -1 || write(request_fd, &event_id, sizeof(unsigned int)) == -1 ||
      write(request_fd, &num_rows, sizeof(size_t)) == -1 || write(request_fd, &num_cols, sizeof(size_t)) == -1) {
    perror("write create request");
    return 1;
  }
  return 0;
}

int ems_reserve(unsigned int event_id, size_t num_seats, size_t* xs, size_t* ys) {
  //TODO: send reserve request to the server (through the request pipe) and wait for the response (through the response pipe)
  int ack = 0;
  int op_code = 4;  
  

  if (write(request_fd, &op_code, sizeof(int)) == -1 || write(request_fd, &event_id, sizeof(unsigned int)) == -1 ||
      write(request_fd, &num_seats, sizeof(size_t)) == -1 || write(request_fd, xs, sizeof(size_t) * num_seats) == -1 ||
      write(request_fd, ys, sizeof(size_t) * num_seats) == -1) {
    perror("write create request");
    return 1;
  }

  if (read(response_fd, &ack, sizeof(int)) == -1) {
    perror("connection failed");
    return 0;
  } else if (ack) {
    return 1;
  } else {
    perror("connection error");
    return 0;
  } 
  return 1;
}

int ems_show(int out_fd, unsigned int event_id) {
  //TODO: send show request to the server (through the request pipe) and wait for the response (through the response pipe)
  int ack = 0;
  int op_code = 5;  
  size_t num_rows, num_cols;

  if (write(request_fd, &op_code, sizeof(int)) == -1 || write(request_fd, &event_id, sizeof(unsigned int)) == -1) {
    perror("write create request");
    return 0;
  }
  
  if (read(response_fd, &ack, sizeof(int)) == -1 ||
      read(response_fd, &num_rows, sizeof(size_t)) == -1 ||
      read(response_fd, &num_cols, sizeof(size_t)) == -1) {
      perror("read show response");
      return 0;
  }
  // Prepare to receive seat data based on num_rows and num_cols
  unsigned int seats[num_rows * num_cols];
  
  if (read(response_fd, &seats, sizeof(size_t[num_rows * num_cols])) == -1) {
      perror("read seats data");
      return 0;
  }

 
  
  // Output the received data to the specified out_fd
  for (size_t i = 1; i <= num_rows; i++) {
        for (size_t j = 1; j <= num_cols; j++) {
            unsigned int seat = seats[(i - 1) * num_cols + (j - 1)];
            dprintf(out_fd, "%u", seat);

            if (j < num_cols) {
                dprintf(out_fd, " ");
            }
        }

        dprintf(out_fd, "\n");
  }

  if(ack){
    return 1;
  } else {
    return 0;
  }
}

int ems_list_events(int out_fd) {
 
  int ack = 0;
  int op_code = 6;
  size_t num_events;

  if (write(request_fd, &op_code, sizeof(int)) == -1) {
    perror("write create request");
    return 0;
  }
  
  if (read(response_fd, &ack, sizeof(int)) == -1 ||
      read(response_fd, &num_events, sizeof(size_t)) == -1 ) {
      perror("read list events response");
      return 0;
  }
  // Prepare to receive seat data based on num_rows and num_cols
  
  unsigned int event_ids[num_events]; 
  if (read(response_fd, event_ids, sizeof(unsigned int) *num_events) == -1) {
      perror("read seats data");
      return 0;
  }

  // Output the received data to the specified out_fd

  for (size_t i = 1; i <= num_events; ++i) {
      if (write(out_fd, event_ids[i], sizeof(event_ids[1])) < 0) {
          perror("write to output fd");
          return 0;
      }
  }

  if(ack){
    return 1;
  } else {
    return 0;
  }
}

