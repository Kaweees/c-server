#include "../include/server.h"
#include "../include/utils.h"
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>

/**
 * @brief Handler for the SIGCHLD signal, which waits for child processes to
 * exit in order to prevent zombie processes.
 * @param sig - the signal number
 */
void handle_sigchld(int sig) { while (waitpid(-1, NULL, WNOHANG) > 0); }

/**
 * @brief Create a network service that listens on the specified port
 *
 * @param port - the port to listen on
 * @param queue_size - the maximum number of pending connections in the socket's
 * @return int - a socket descriptor for the service
 */
int create_service(short port, int queue_size) {
  int sock_fd; /* Socket file descriptor */
  /* Socket address information */
  struct sockaddr_in sock_addr = {
      .sin_family = AF_INET, /* IPv4-compliant protocol */
      .sin_port = htons(port), /* short, network byte order (big-endian) */
      .sin_addr = {htonl(INADDR_ANY)}, /* long, host byte order (varies),
                                        listen to all interfaces */
      .sin_zero = {0} /* Padding to match size of struct sockaddr */
  };

  /**
   * Create the socket using IPv4 (AF_INET), and TCP (SOCK_STREAM). Use AF_INET6
   * for IPv6 and SOCK_DGRAM for UDP.
   */
  if ((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) == SYSCALL_ERROR) {
    perror("Error creating the server socket");
    exit(EXIT_FAILURE);
  }

  /**
   * Set the socket options to allow the reuse of the address. This is useful
   * when the server is restarted and the port is still in use.
   */
  int reuse = 1;
  if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int)) == SYSCALL_ERROR) {
    perror("Error setting the server socket options");
    exit(EXIT_FAILURE);
  }

  /**
   * Bind the socket to the address and port specified in the sockaddr_in
   * struct.
   */
  if (bind(sock_fd, (struct sockaddr*)&sock_addr, sizeof(struct sockaddr)) == SYSCALL_ERROR) {
    perror("Error binding the server socket to the address");
    exit(EXIT_FAILURE);
  }

  /**
   * Listen for incoming connections on the socket. The second argument is the
   * maximum number of pending connections in the socket's listen queue.
   */
  if (listen(sock_fd, queue_size) == SYSCALL_ERROR) {
    perror("Error listening on the server socket");
    exit(EXIT_FAILURE);
  }

  /* Set the signal handler for the server for when a child process dies */
  struct sigaction sigchld_action = {0};
  sigchld_action.sa_handler = handle_sigchld;
  sigemptyset(&sigchld_action.sa_mask);
  sigchld_action.sa_flags = SA_RESTART;
  if (sigaction(SIGCHLD, &sigchld_action, NULL) == SYSCALL_ERROR) {
    perror("Error setting the server SIGCHLD signal handler ");
    exit(EXIT_FAILURE);
  }

  printf("Server listening on port: %d\n", port);
  return sock_fd;
}

/**
 * @brief Accept an incoming connection on the specified socket descriptor
 *
 * @param sock_fd - the socket descriptor to accept the connection on
 * @return int - the socket descriptor for the new connection
 */
int accept_connection(int sock_fd) {
  int new_fd; /* New socket file descriptor */
  struct sockaddr_in remote_addr = {0}; /* Remote address information */
  socklen_t addr_size = sizeof(struct sockaddr_in);

  /**
   * Accept incoming connections on the socket and populate a new socket
   * descriptor, remote address, and address size information based on it.
   */
  errno = EINTR;
  while (errno == EINTR) {
    if ((new_fd = accept(sock_fd, (struct sockaddr*)&remote_addr, &addr_size)) == SYSCALL_ERROR && errno != EINTR) {
      perror("Error accepting incoming connections on the server socket");
      exit(EXIT_FAILURE);
    } else if (new_fd != SYSCALL_ERROR) {
      break;
    }
  }
  return new_fd;
}

/**
 * @brief Handle a client connection request
 *
 * @param new_sock_fd - the socket descriptor for the client connection
 */
void handle_request(int new_sock_fd) {
  int stdout_fd = dup(STDOUT_FILENO);
  if (stdout_fd == SYSCALL_ERROR) {
    perror("Error duplicating the stdout file descriptor");
    exit(EXIT_FAILURE);
  }
  if (dup2(new_sock_fd, STDOUT_FILENO) == SYSCALL_ERROR) {
    perror("Error redirecting stdout to the network socket");
    exit(EXIT_FAILURE);
  }
  printf("HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: "
         "14\r\n\r\n");
  printf("Network socket");
  if (dup2(stdout_fd, STDOUT_FILENO) == SYSCALL_ERROR) {
    perror("Error restoring the stdout file descriptor");
    exit(EXIT_FAILURE);
  }
  FILE* network = fdopen(new_sock_fd, "r");
  if (network == NULL) {
    perror("Error opening the socket as a file stream");
    close(new_sock_fd);
    exit(EXIT_FAILURE);
  }
  char* line = NULL;
  size_t size;
  ssize_t num;

  while ((num = getline(&line, &size, network)) >= 0 && strcmp(line, CRLF)) {
    printf("Received %ld (%ld): %s", size, strlen(line), line);
    free(line);
    line = NULL;
  }
  free(line);
  line = NULL;
  fclose(network);
}

/**
 * @brief Run the server service
 *
 * @param sock_fd - the socket descriptor for the server
 */
void run_service(int sock_fd) {
  while (true) {
    /* Accept connections to the server */
    int new_sock_fd = accept_connection(sock_fd);
    /* Handle the request in a child process */
    pid_t pid = fork();
    if (pid == SYSCALL_ERROR) {
      perror("Error creating a child process");
      close(new_sock_fd);
      exit(EXIT_FAILURE);
    } else if (pid == 0) {
      /* Code executed by the child process */
      close(sock_fd); /* Close the server socket in the child process */
      printf("Connection established with client %d\n", new_sock_fd);
      handle_request(new_sock_fd);
      printf("Connection closed with client %d\n", new_sock_fd);
      close(new_sock_fd);
      exit(EXIT_SUCCESS);
    } else {
      /* Code executed by the parent process */
      close(new_sock_fd); /* Close the network socket in the parent process */
    }
  }
}

/**
 * @brief server - A function that starts the server
 */
void server(char* cwd, int port, int queue_size) {
  /* Set stdout to be unbuffered so that logs are printed immediately */
  setbuf(stdout, NULL);

  /* Change the working directory */
  if (chdir(cwd) == SYSCALL_ERROR) {
    perror("Error changing directory");
    exit(EXIT_FAILURE);
  } else {
    printf("Changed working directory to: %s\n", cwd);
  }

  /* Create the server */
  int fd = create_service(port, queue_size);
  /* Run the server */
  run_service(fd);
  /* Close the server */
  close(fd);
}