#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../include/server.h"
#include "../include/utils.h"

/**
 * @brief Program entry point
 *
 * @param argc - the number of command line arguments
 * @param argv - an array of command line arguments
 * @return int - the exit status
 */
int main(int argc, char* argv[]) {
  enum ServerOptions opt;
  char* cwd = DEFAULT_WORKING_DIRECTORY;
  int port = DEFAULT_PORT;
  int queue_size = DEFAULT_QUEUE_SIZE;

  /* Parse the command line arguments */
  while ((opt = getopt(argc, argv, "p:d:q:")) != OUT_OF_OPTIONS) {
    switch (opt) {
      case PORT_NUMBER: {
        port = strtol(optarg, NULL, 10);
        if (!(MIN_ALLOWED_PORT <= port && port <= MAX_ALLOWED_PORT)) {
          fprintf(stderr, "Invalid port number: %s\n", optarg);
          usage(*argv);
        }
        break;
      }
      case WORKING_DIRECTORY: {
        cwd = optarg;
        break;
      }
      case QUEUE_SIZE: {
        queue_size = strtol(optarg, NULL, 10);
        if (!(MIN_QUEUE_SIZE <= queue_size)) {
          fprintf(stderr, "Invalid queue size: %s\n", optarg);
          usage(*argv);
        }
        break;
      }
      default: {
        usage(*argv);
      }
    }
  }

  /* Start the server */
  server(cwd, port, queue_size);

  return EXIT_SUCCESS;
}