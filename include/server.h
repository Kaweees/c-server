#pragma once

#define DEFAULT_WORKING_DIRECTORY "site" /* The default working directory */
#define DEFAULT_PORT 8080 /* The default server port number */
#define DEFAULT_QUEUE_SIZE 5 /* The default queue size */
#define MIN_ALLOWED_PORT 1024 /* Ports below 1024 are reserved for privileged applications */
#define MAX_ALLOWED_PORT 49151 /* Ports above 49151 are ephemeral ports */
#define MIN_QUEUE_SIZE 1 /* The minimum queue size */
#define CRLF "\r\n" /* Carriage return and line feed, end of line marker used by HTTP*/

/* Begin typedef declarations */

/* Represents the options that can be passed to the program */
typedef enum ServerOptions {
  /* The working directory */
  WORKING_DIRECTORY = 'd',
  /* The port number */
  PORT_NUMBER = 'p',
  /* The queue size */
  QUEUE_SIZE = 'q',
  /* The end of the options */
  OUT_OF_OPTIONS = -1,
} ServerOptions;

typedef enum HttpMethod {
  /* The GET HTTP method */
  GET,
  /* The POST HTTP method */
  POST,
} HttpMethod;

/* Begin function prototype declarations */
void handle_sigchld(int sig);
int create_service(short port, int queue_size);
int accept_connection(int sock_fd);
void handle_request(int new_sock_fd);
void run_service(int sock_fd);
void server(char* cwd, int port, int queue_size);
