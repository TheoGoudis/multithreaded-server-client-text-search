
/* utils.c
   Sample code of 
   Assignment L1: Simple multithreaded search engine 
   for the course PLY510 Operating Systems, University of Ioannina

   (c) A. Hatzieleftheriou, G. Margaritis 2012

   Final Code by Theo Goudis
*/


#include "utils.h"

/*=========================================================*
 *                          ERROR                          *
 *=========================================================*/
void ERROR(char *msg)
{
    fprintf(stderr, "Error in %s. Cause: ", msg);
    fflush(stdout);
    perror("");
    exit(EXIT_FAILURE);
}

/*=========================================================*
 *                     write_to_socket                     *
 *=========================================================*/
int write_str_to_socket(int socket_fd, char *buf, int numbytes)
{
    char *ptr;
    int nwritten, nleft;
    int wsize;

    // write the amount of data to be sent
    wsize = numbytes;
    if (write(socket_fd, &wsize, sizeof(wsize)) < 0)
        ERROR("write_to_socket()");

    // write data
    ptr = buf;
    nleft = numbytes;
    while (nleft > 0 ) {
      if ((nwritten = write(socket_fd, ptr, nleft)) < 0)
            ERROR("write_to_socket()");
      nleft -= nwritten;
      ptr += nwritten;
    }

    return numbytes;
}


/*=========================================================*
 *                    read_from_socket                     *
 *=========================================================*/
int read_str_from_socket(int socket_fd, char *buf, int bufsize)
{
    char *ptr;
    int nread, nleft, rc;
    int rsize;

    // read the amount of sent data 
    if ((rc = read(socket_fd, &rsize, sizeof(rsize))) < 0)
        ERROR("read_from_socket()");
    else if (rc == 0)
        return 0;

    // read data
    ptr = buf;
    nleft = rsize;
    while (nleft > 0 ) {
      if ((nread = read(socket_fd, ptr, nleft)) <= 0)
        ERROR("read_from_socket()");
      nleft -= nread;
      ptr += nread;
    }

    *ptr = '\0';

    return rsize;
}
