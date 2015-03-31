
/* utils.h

   Sample code of 
   Assignment L1: Simple multithreaded search engine 
   for the course PLY510 Operating Systems, University of Ioannina

   (c) A. Hatzieleftheriou, G. Margaritis 2012

   Final Code by Theo Goudis
*/


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <assert.h>
#include <pthread.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/signal.h>
#include <unistd.h>
#include <sys/time.h>
#include <poll.h>
#include <fcntl.h>

void ERROR(char *msg);

// write to socket 'numbytes' bytes from buffer 'buf'
int write_str_to_socket(int socket_fd, char *buf, int numbytes);

// read from socket into buffer 'buf' a stream of bytes that were 
// sent using write_to_socket(); terminate data with '\0'.
int read_str_from_socket(int socket_fd, char *buf, int bufsize);
