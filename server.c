/* server.c

   Sample code of
   Assignment L2: Simple multithreaded search engine
   for the course PLY510 Operating Systems, University of Ioannina

   (c) A. Hatzieleftheriou, G. Margaritis 2012

   Final Code by Theo Goudis
*/

#include "utils.h"

#define C_THREADS				6							//here is defined the number of consumer threads
#define MY_PORT                 6767
#define BUF_SIZE                1024
#define MAX_PENDING_CONNECTIONS   10						//max number of server conenctions

char **filenames;       									//files to search in
int NUM_FILES = 10;											//max number of files to search in

int completed_requests = 0;									//requests completed by the server
long total_waiting_time;									//total time that connections have been in connection stack
long total_service_time;									//total time of connections' servicing

int exit_status = 0;

/*=========================================================*
 *       Variables for Server Connections handling         *
 *=========================================================*/
typedef struct dcon {										//datatype of connections stack
	int con;
	struct timeval stime;
}dcon;

dcon Server_Con_Stack[MAX_PENDING_CONNECTIONS];				//Stack for incoming server connections
int Cur_Server_Con=-1;										//Current working server connection (socket fd)

int socket_fd=-1;												// listen on this socket for new connections

/*==============================================================*
 * Mutex, Condition and Attribute variables for thread handling *
 *==============================================================*/

pthread_t Producer_Thread;									//1 Producer thread
pthread_t Consumer_Thread[C_THREADS];						//number of Consumer threads (C_THREADS)

pthread_mutex_t Mexcl_mutex=PTHREAD_MUTEX_INITIALIZER;		//mutex for mutual exclusion (lock/unlock) for Server_Con_Stack editing
pthread_mutex_t Texcl_mutex=PTHREAD_MUTEX_INITIALIZER;		//mutex for mutual exclusion (lock/unlock) for total_waiting_time, total_service_time editing and completed_requests
pthread_cond_t Non_full_stack=PTHREAD_COND_INITIALIZER;		//condition variable for stack's full condition
pthread_cond_t Non_empty_stack=PTHREAD_COND_INITIALIZER;	//condition variable for stack's empty condition

pthread_attr_t attr;										//attribute that sets the thread as Detached or Joinable


/*=========================================================*
 *                     process_request                     *
 *=========================================================*/

void process_request(int socket_fd)
{
    char buffer[BUF_SIZE]="\n";
	char filebuf[BUF_SIZE];									//buffer for word (for fscanf)
	char word_to_search[BUF_SIZE];							//the word that has to be searched in the files
	int word_size;
    int numbytes, i, occurrences;
	FILE *search_fd;										//the file descriptor for the opened file

    // receive message (word to search)
    numbytes=read_str_from_socket(socket_fd, word_to_search, BUF_SIZE);

    // search in files and send reply
    for (i=0; i<NUM_FILES; i++)
	{
		occurrences=0;

		// my file search code

		word_size=strlen(word_to_search);					//length of word_to_search

		search_fd=fopen(filenames[i],"r");					//open a file (read) and save his file descriptor in search_fd

		if(search_fd==NULL)									//check if file opening succeeded
			perror("Error: can't open file \n");
		else
			while(fscanf(search_fd,"%s",filebuf)!=EOF)		//read each word in opened file till EOF, compare each word with the word_to_search and find how many times it appears in the file (occurrences)
				if(strncmp(word_to_search, filebuf, word_size)==0)
					occurrences++;

		fclose(search_fd);									//close the opened file

		// add found occurences to buffer
		sprintf(filebuf, "%s:%d\n", filenames[i], occurrences);
		strcat(buffer, filebuf);
    }
    write_str_to_socket(socket_fd, buffer, strlen(buffer));
}


/*=========================================================*
 *                   Consumer Function                     *
 *=========================================================*/

void *myfunc_Consumer(void* arg)								//the function that the Consumer threads will run
{
	int c_socket_fd, i;
	struct timeval tv, tv1 ,tv2;

	while(1)
	{
		pthread_mutex_lock(&Mexcl_mutex);						//lock the mutex (for mutual exclusion)

		while(Cur_Server_Con==-1)								//wait if the Server_Con_Stack is empty
			pthread_cond_wait(&Non_empty_stack, &Mexcl_mutex);	//mutex unlocks till the Server_Con_Stack is not empty, then mutex locks again

		c_socket_fd=Server_Con_Stack[Cur_Server_Con].con;		//save the current connection's fd to c_socket_fd

		pthread_mutex_lock(&Texcl_mutex);
		gettimeofday(&tv, NULL);
		tv1.tv_sec=tv.tv_sec - Server_Con_Stack[Cur_Server_Con].stime.tv_sec;
		tv1.tv_usec=tv.tv_usec - Server_Con_Stack[Cur_Server_Con].stime.tv_usec;
		if (tv1.tv_usec<0)
			tv1.tv_sec-=1;
		total_waiting_time+=(tv1.tv_sec*1000000)+abs(tv1.tv_usec);
		pthread_mutex_unlock(&Texcl_mutex);

		Server_Con_Stack[Cur_Server_Con].con=-1;				//set the current connection's fd to -1
		Cur_Server_Con--;										//move to the next connection

		pthread_mutex_unlock(&Mexcl_mutex);						//unlock the mutex because the "critical section" has ended

		i=MAX_PENDING_CONNECTIONS;								//check if the Server_Con_Stack was full, if it was the signal Non_full_stack is sent so that the Producer thread becomes ready
		i=i-2;
		if(Cur_Server_Con==i)
			pthread_cond_signal(&Non_full_stack);

		gettimeofday(&tv, NULL);
		process_request(c_socket_fd);							//call the process_request(temp_socket_fd) so that the files are searched
		close(c_socket_fd);
		
		pthread_mutex_lock(&Texcl_mutex);
		gettimeofday(&tv1, NULL);
		tv2.tv_sec=tv1.tv_sec-tv.tv_sec;
		tv2.tv_usec=tv1.tv_usec-tv.tv_usec;
		if (tv2.tv_usec<0)
			tv2.tv_sec-=1;
		total_service_time+=(tv2.tv_sec*1000000)+abs(tv2.tv_usec);
		completed_requests++;									//one more connection was served so completed_requests +1
		pthread_mutex_unlock(&Texcl_mutex);
	}
}


/*=========================================================*
 *                    Producer Function                    *
 *=========================================================*/

void *myfunc_Producer(void* arg)							//the function that the Producer thread will run
{
	int new_fd;                     	// use this socket to service a new connection
	socklen_t clen;
	struct sockaddr_in server_addr,		// my address information
			client_addr;				// connector's address information
	struct timeval tv;
	int i;
	struct pollfd fds[2];
	int fds_items=2;					//+1
	
	// create socket
	if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	    ERROR("socket()");
	fcntl(socket_fd, F_SETFL, O_NONBLOCK);
	
	// create socket adress of server (type, IP-adress and port number)
	bzero(&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);    // any local interface
	server_addr.sin_port = htons(MY_PORT);
	
	if(setsockopt(socket_fd,SOL_SOCKET,SO_REUSEADDR,&i,sizeof(int)) == -1) {
		perror("setsockopt");
		exit(1);
	}
	
	// bind socket to address
	if (bind(socket_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) == -1){
		ERROR("bind()");
		exit(1);
	}
	
	// start listening to socket for incomming connections
	listen(socket_fd, MAX_PENDING_CONNECTIONS);
	printf("Listening for new connections on port %d ...\n", MY_PORT);
	clen = sizeof(client_addr);
	
	fds[1].fd=socket_fd;
	fds[1].events=POLL_IN;
	
	while(exit_status!=1){
		if(poll(fds,fds_items,10)>0){				//every 10 milisseconds check if there is a new client connection request
			// accept incomming connection
			if ((new_fd = accept(socket_fd, (struct sockaddr *)&client_addr, &clen)) == -1) {
				ERROR("accept()");
			}
			if(exit_status==1){
				shutdown(new_fd,SHUT_RDWR);
				exit(1);
			}
			// got connection, serve request
			printf("Got connection from '%s'\n", inet_ntoa(client_addr.sin_addr));
			
			pthread_mutex_lock(&Mexcl_mutex);						//lock the mutex (for mutual exclusion)
			
			while(Cur_Server_Con==(MAX_PENDING_CONNECTIONS-1)){		//check if the Server_Con_Stack if full, if so the thread waits for a Non_full_stack signal (mutex unlocks, it locks again after the signal)
				pthread_cond_wait(&Non_full_stack, &Mexcl_mutex);
			}
			if(exit_status==1){
				shutdown(socket_fd,SHUT_RDWR);
				exit(1);
			}
			Cur_Server_Con++;										//add the connection in the connection's stuck
			Server_Con_Stack[Cur_Server_Con].con=new_fd;
			gettimeofday(&tv, NULL);
			Server_Con_Stack[Cur_Server_Con].stime.tv_sec=tv.tv_sec;
			Server_Con_Stack[Cur_Server_Con].stime.tv_usec=tv.tv_usec;

			pthread_mutex_unlock(&Mexcl_mutex);						//unlock the mutex (for mutual exclusion)

			if(Cur_Server_Con==0)									//if the stuck was empty, the signal Non_empty_stack is sent to all the Consumer threads
				pthread_cond_broadcast(&Non_empty_stack);
		}
	}
	if(socket_fd!=-1)
		shutdown(socket_fd,SHUT_RDWR);
	return(0);
}


/*=========================================================*
 *                     SIGNAL handler                      *
 *=========================================================*/

void exit_handle(){
	exit_status=1;
	pthread_cond_signal(&Non_full_stack);
}

static void stop_occured(int signo){
	if(signo==20){
		printf("\n SIGTSTP CAUGHT.\n");
		printf("\n Requests serviced: %d . \n Total waiting time (microsec): %ld . \n Total service time (microsec): %ld . \n", completed_requests, total_waiting_time, total_service_time);
		exit_handle();
	}
}

static void exit_occured(int signo){
	if(signo==SIGINT){
		printf("\n SIGINT CAUGHT.\n");
		exit_handle();
	}
}

/*=========================================================*
 *                          main                           *
 *=========================================================*/

int main(int argc, char *argv[])
{
	int i;
	struct sigaction a,b;
	a.sa_handler = stop_occured;
	a.sa_flags = 0;
	sigemptyset( &a.sa_mask );
	sigaction(20, &a, NULL );
	b.sa_handler = exit_occured;
	b.sa_flags = 0;
	sigemptyset( &b.sa_mask );
	sigaction(SIGINT, &b, NULL );
	
	printf("\n SERVER 2014 \n");
	
	// create array of filenames
	filenames = malloc(NUM_FILES * sizeof(char *));
	for (i = 0; i < NUM_FILES; i++) {
		filenames[i] = malloc(BUF_SIZE);
		sprintf(filenames[i], "samples/f%d.txt", i);
	}

	for(i=0; i<MAX_PENDING_CONNECTIONS; i++)							//initialize Server_Con_Stack[MAX_PENDING_CONNECTIONS].con values to -1
		Server_Con_Stack[i].con=-1;

	pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);			//change attr value to PTHREAD_CREATE_DETACHED so that the thread is created in the detached state (NOT the default value of PTHREAD_CREATE_JOINABLE)

	for(i=0; i<C_THREADS; i++)
		 pthread_create(&Consumer_Thread[i],&attr,myfunc_Consumer,NULL);	//create the consumer threads, in detached state, running the myfunc_Consumer function
	
	pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_JOINABLE);
	pthread_create(&Producer_Thread,&attr,myfunc_Producer,NULL);	//create producer thread, in detached state, running the myfunc_Producer(new_fd) function
	
	if(pthread_join(Producer_Thread,NULL)!=0){
		printf("\n ERROR Thread Join.\n");
	}
	close(socket_fd);
	printf("\n SERVER EXIT.\n");
	return(0);
}

