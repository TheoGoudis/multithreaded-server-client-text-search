/* client.c

   Sample code of 
   Assignment L2: Simple multithreaded search engine 
   for the course PLY510 Operating Systems, University of Ioannina

   (c) A. Hatzieleftheriou, G. Margaritis 2012
   
   Final Code by Theo Goudis
*/

#include "utils.h"

#define SERVER_PORT     6767
#define BUF_SIZE        1024
#define MAXHOSTNAMELEN  1024
#define MAXCOMMANDS	10					//max numbers of words that can be searched in one commmand


char *Command_stack[MAXCOMMANDS];		//list of the words to be searched
int Cur_command_stack=-1;
pid_t pid[MAXCOMMANDS];					//pid list to wait() for child processes


/*=========================================================*
 *                  initCommand function                   *
 *=========================================================*/

void initCommand(int argc, char** argv)
{
	int i=0;
	int temp_length=0;
	
	for(i=2; i<argc; i++)
	{
		Cur_command_stack++;
		if(Cur_command_stack>=MAXCOMMANDS)
		{
			perror("Error: too many arguments");
			exit(0);
		}
		temp_length=strlen(argv[i]);
		Command_stack[Cur_command_stack]=calloc(temp_length,sizeof(char));
		strcpy(Command_stack[Cur_command_stack],argv[i]);
	}
}

/*=========================================================*
 *     func_exec function that searches for many words     *
 *=========================================================*/

void func_exec(char **argv, int pid_num)
{

	argv[2]=realloc(NULL,sizeof(Command_stack[Cur_command_stack]));
	strcpy(argv[2], Command_stack[Cur_command_stack]);
	Command_stack[Cur_command_stack]=NULL;
		
	pid[pid_num]=fork();
	
	if(pid[pid_num]<0)
	{
		perror("error: forking child failed");
		exit(1);
	}
	else if(pid[pid_num]==0)
	{
		if(execvp(argv[0], argv)<0)
		{
			perror("error: execvp failed");
			exit(1);
		}
	}
	else
	{	
		argv[2]=NULL;
	}
}




/*=========================================================*
 *                          main                           *
 *=========================================================*/
int main(int argc, char **argv)
{
    struct sockaddr_in server_addr;
    struct hostent *host_info;
    int socket_fd, numbytes;
    char buffer[BUF_SIZE];
	char *argvT[4];
	int temp_size=0;
	int pid_num=-1;
	int i=0;
	
	
    if(argc<3)
	{
		printf("usage: %s <hostname> <message>\n", argv[0]);
        exit(EXIT_FAILURE);
	}
	else if(argc==3)											//if only one word is to be searched
	{
		// get the host (server) info
		if ((host_info = gethostbyname(argv[1])) == NULL)
			ERROR("gethostbyname()"); 
		// create socket
		if ((socket_fd = socket(PF_INET, SOCK_STREAM, 0)) == -1)
			ERROR("socket()");

		// create socket adress of server (type, IP-adress and port number)
		bzero(&server_addr, sizeof(server_addr));
		server_addr.sin_family = AF_INET;
		server_addr.sin_addr = *((struct in_addr*)host_info->h_addr);
		server_addr.sin_port = htons(SERVER_PORT);

		// connect to the server
		if (connect(socket_fd, (struct sockaddr*) &server_addr, 
			sizeof(server_addr)) == -1)
			ERROR("connect()");

		// send message
		write_str_to_socket(socket_fd, argv[2], strlen(argv[2]));

		// receive message
		numbytes = read_str_from_socket(socket_fd, buffer, BUF_SIZE);
		printf("Word %s \n Reply: %s\n", argv[2], buffer); // print to stdout

		// close the connection to the server
		close(socket_fd);        
	}
	else													//if many words have to be searched
	{
		initCommand(argc, argv);
		
		temp_size=strlen(argv[0]);
		argvT[0]=calloc(temp_size,sizeof(char));
		strcpy(argvT[0], argv[0]);
		temp_size=strlen(argv[1]);
		argvT[1]=calloc(temp_size,sizeof(char));
		strcpy(argvT[1], argv[1]);
		argvT[3]=NULL;
		
		while(Cur_command_stack!=-1)
		{
			pid_num++;
			func_exec(argvT, pid_num);
			Cur_command_stack--;
		}
		for(i=0; i<=pid_num; i++)
			waitpid(pid[pid_num],NULL,0);
	}
    return(0);
}
