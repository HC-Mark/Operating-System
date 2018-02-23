/* server.c inspired by 
	socket turtorial: http://www.cs.rpi.edu/~moorthy/Courses/os98/Pgms/socket.html
	tinyhttpd program. link: http://sourceforge.net/projects/tinyhttpd/
	send_recv example: https://stackoverflow.com/questions/13479760/c-socket-recv-and-send-all-data
	and Kris hint code for 311: https://gist.github.com/kmicinski/afa3c1dec7ef612292fdeb27be8f625a
*/
#include <stdio.h>  
#include <sys/socket.h>  
#include <sys/types.h>  
#include <netinet/in.h>  
#include <arpa/inet.h>  
#include <unistd.h> //used for close
#include <fcntl.h>  //used for open 
#include <ctype.h>  
#include <string.h>  
#include <sys/stat.h>  
#include <pthread.h>  
#include <sys/wait.h>  
#include <stdlib.h>  
#include <stdbool.h>
#include <sys/mman.h>
//#include <sys/sendfile.h> --uncomment if run in linux
//uncomment if run in osx
#include <sys/uio.h>

//predefined constant
#define SERVER_STRING "Server: bugfree/0.1.0\r\n"  
#define BUF_SIZE 1024
#define CHUNK_SIZE 100000
#define OFFSET 0
//Global variables
int server_fd = -1;
char *root;         // Root directory from which the server serves files
char buffer[BUF_SIZE];

//define fatFilePointer struct
struct fatFilePointer {
  int fd; //file descriptor of file
  int length;
  char *data;
};
struct fatFilePointer read_file_s(char* name); //new version read_file
struct fatFilePointer read_file(char* name);
int starts_with(const char *a, const char *b);
void* accept_request(void*); //accept and send request to appropriate function
void serve_file(int client, char* filename, char* filetype);
void success(int, int, char*);
void permission_deny(int);
void unimplemented(int);
void not_found(int);
int startup(u_short*); // set up the socket for sever

struct fatFilePointer read_file_s(char* name) {
	int fd;
	struct stat stat_buf;
	//char* buffer;
	struct fatFilePointer ret;
	ret.fd = -1;
	ret.length = 0;

	fd = open(name, O_RDONLY);
	if(fd == -1) {
		perror("unable to open file");
		exit(1);
	}
	ret.fd = fd;
	fstat(fd, &stat_buf);
	ret.length = (int) stat_buf.st_size;
	return ret;
}

struct fatFilePointer read_file(char *name)
{
	FILE *file;
	//int fd;
	//struct stat stat_buf;
	char *buffer;
	unsigned long fileLen;
  	struct fatFilePointer ret;
  	ret.length = 0;
  	ret.data = NULL;

	//Open file
	file = fopen(name, "rb");
	if (!file)
	{
		fprintf(stderr, "Unable to open file %s", name);
		return ret;
	}

  fileLen = 0;
  buffer = malloc(CHUNK_SIZE);
  char temp[CHUNK_SIZE];
  unsigned long bytesRead;

  do {
    bytesRead = fread(temp,1,CHUNK_SIZE,file);
    char *newbuffer = malloc(fileLen + CHUNK_SIZE);
    for (unsigned long i = 0; i < fileLen; i++) {
      newbuffer[i] = buffer[i];
    }
    for (unsigned long i = 0; i < bytesRead; i++) {
      newbuffer[fileLen + i] = temp[i];
    }
    fileLen += bytesRead;
    char *oldbuf = buffer;
    buffer = newbuffer;
    free(oldbuf);
  } while (bytesRead == CHUNK_SIZE);

  ret.length = fileLen;
  ret.data = buffer;
  return ret;
}

int starts_with(const char *a, const char *b)
{
   if (strncmp(a, b, strlen(b)) == 0) return 1;
   return 0;
}


void* accept_request(void* client) {
	//tranfer client to normal socket number
	long client_socket_tmp = (long) client;
	int client_socket = (int) client_socket_tmp;
	//parse from header from http
	char method[BUF_SIZE] = "GET";
	char url[BUF_SIZE] = "/test.pdf";
	char version[BUF_SIZE]= "HTTP/1.0";
	//variable for convenient
	char filename[BUF_SIZE];
	char filetype[BUF_SIZE];
	FILE *stream = fdopen(client_socket,"r+"); //? I can directly use client_socket, same as that	
	//printf("client socket is %d",client_socket);
	//char buf[BUF_SIZE];
	/* jiaping's parsing */
	//assume they are all correct


	if(strcasecmp(method, "GET") == 0) {
		//get the complete path to file
		strcpy(filename, root);
		strcat(filename, url);
		//if the destination is a directory, default to send index.html -- need to revise to send a list
		if(url[strlen(url)-1] == '/')
			strcat(filename, "index.html");
		  /* serve static content */
  		if (strstr(filename, ".html")) {
    		strcpy(filetype, "text/html");
  		} else if (strstr(filename, ".gif")) {
    		strcpy(filetype, "image/gif");
  		} else if (strstr(filename, ".jpg")) {
    		strcpy(filetype, "image/jpg");
  		} else {
    		strcpy(filetype, "text/plain");
  		}
  		printf("file name is now %s\n", filename);
  		printf("file type is now %s\n", filetype);
  		serve_file(client_socket, filename, filetype);
  		//not_found(client_socket);
  		close(client_socket);

	}
	else{
		unimplemented(client_socket);
		fclose(stream);
		close(client_socket);
	}

	pthread_exit(NULL);
}

void serve_file(int client, char* filename, char* filetype) {
	int offset = OFFSET;
	FILE* stream = fdopen(client,"r+");
	//struct fatFilePointer source = read_file(filename);; // the file we want to send to client
	struct fatFilePointer source = read_file_s(filename);
	//printf("%s\n", source.data);
	if(source.fd == -1) {
		not_found(client);
	}
	else {
		success(client,source.length,filetype);
		//send file
	}
	int num_byte = sendfile(client, source.fd, &offset, source.length);
	if(num_byte == -1) {
		perror("sendfile fails");
		exit(1);
	}
	if(num_byte != source.length) {
		perror("incomplete transfer from sendfile");
		exit(1);
	}
	//close file descirptor
	close(source.fd);

	/*
	old way
	fwrite(source.data, 1, source.length, stream);
	free(source.data);
	fflush(stream);
	*/
	//use sendfile later

}

void success(int client, int file_length, char* filetype)  
{  
 	FILE *stream = fdopen(client,"r+"); 
    /*successful HTTP header */  
	fprintf(stream, "HTTP/1.1 200 OK\n");
  	fprintf(stream, SERVER_STRING);
  	fprintf(stream, "Content-length: %d\n", file_length);
  	fprintf(stream, "Content-type: %s\n", filetype);
  	fprintf(stream, "\r\n");
  	fflush(stream);
  	//fclose(stream);
}  

void unimplemented(int client) {
	char buf[BUF_SIZE];  
  
    /* reponse server can not run this request, feature not be implemented*/  
    sprintf(buf, "HTTP/1.0 501 Unimplemented Error\n");  
    send(client, buf, strlen(buf), 0);  
    sprintf(buf, "Content-type: text/html\n");  
    send(client, buf, strlen(buf), 0);  
    sprintf(buf, "\r\n");  
    send(client, buf, strlen(buf), 0);  
    sprintf(buf, "The request you sent has not been implemented\r\n");  
    send(client, buf, strlen(buf), 0); 

}

void permission_deny(int client) {
	char buf[BUF_SIZE];  
  
    /* reponse server can not run this request, feature not be implemented*/  
    sprintf(buf, "HTTP/1.0 403 Permission Deny Error\r\n");  
    send(client, buf, strlen(buf), 0);  
    sprintf(buf, "Content-type: text/html\r\n");  
    send(client, buf, strlen(buf), 0);  
    sprintf(buf, "\r\n");  
    send(client, buf, strlen(buf), 0);  
    sprintf(buf, "Don't have permission to open directory in the path or write to target file\r\n");  
    send(client, buf, strlen(buf), 0); 

}
//need further revise
void not_found(int client) {
	char buf[BUF_SIZE];  
  
    /* 404 page */  
    sprintf(buf, "HTTP/1.0 404 NOT FOUND\r\n");  
    send(client, buf, strlen(buf), 0);  
    /*server information*/  
    sprintf(buf, SERVER_STRING);  
    send(client, buf, strlen(buf), 0);  
    sprintf(buf, "Content-Type: text/html\r\n");  
    send(client, buf, strlen(buf), 0);  
    sprintf(buf, "\r\n");  
    send(client, buf, strlen(buf), 0);  
    sprintf(buf, "<HTML><TITLE>Not Found</TITLE>\r\n");  
    send(client, buf, strlen(buf), 0);  
    sprintf(buf, "<BODY><P>The server could not fulfill\r\n");  
    send(client, buf, strlen(buf), 0);  
    sprintf(buf, "your request because the resource specified\r\n");  
    send(client, buf, strlen(buf), 0);  
    sprintf(buf, "is unavailable or nonexistent.\r\n");  
    send(client, buf, strlen(buf), 0);  
    sprintf(buf, "</BODY></HTML>\r\n");  
    send(client, buf, strlen(buf), 0);

}


void bad_request(int client)  
{  
    char buf[BUF_SIZE];  
  
    /*回应客户端错误的 HTTP 请求 */  
    sprintf(buf, "HTTP/1.0 400 BAD REQUEST\r\n");  
    send(client, buf, sizeof(buf), 0);  
    sprintf(buf, "Content-type: text/html\r\n");  
    send(client, buf, sizeof(buf), 0);  
    sprintf(buf, "\r\n");  
    send(client, buf, sizeof(buf), 0);  
    sprintf(buf, "<P>Your browser sent a bad request, ");  
    send(client, buf, sizeof(buf), 0);  
    sprintf(buf, "such as a POST without a Content-Length.\r\n");  
    send(client, buf, sizeof(buf), 0);  
}  

int startup(unsigned short* port) {
	int http = 0;
	struct sockaddr_in server;

	http = socket(PF_INET, SOCK_STREAM, 0); 
	if (http == -1) {
		perror("socket can not be created\n");
		exit(1);
	}
	memset(&server, 0, sizeof(server));  
    server.sin_family = AF_INET;  
    server.sin_port = htons(*port);  
    server.sin_addr.s_addr = htonl(INADDR_ANY);

    //sets options associated with a socket --don't quite understand
   	if (setsockopt(http, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int)) < 0) {
    	perror("setsockopt(SO_REUSEADDR) failed");
    	exit(1);
  	}

  	if (bind(http, (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("bind failed!");
    	exit(1);
	}

	  // Listen for connections on this socket
  if (listen(http, 5) != 0) {
    perror("listen failed!");
    exit(1);
  }
  return http;
}

int main(int argc, char** argv) {
	int server_socket = -1;
	int client_socket = -1;
	unsigned short port = 8080; /* default port is 8080 */
	struct sockaddr_in client_name;
	unsigned int client_name_len = sizeof(client_name);
	pthread_t newthread; //used to deal with multiple requests
	int n; //test for socket

	if(argc != 3) {
    	printf("Usage: %s <port-number> <root-directory>\n", argv[0]);
    	exit(1);
  	}

  	port = atoi(argv[1]);
  	root = argv[2];

  	memset(&client_name, 0, sizeof(client_name)); //clean allocated client_name
	//setup the http sever in stratup
	server_socket = startup(&port);
	printf("Team ZTJ sever is running on port %d\n", port);

	while(1) {
		//socket receive the linking request by client
		client_socket = accept(server_socket, (struct sockaddr *)&client_name, &client_name_len);
		if (client_socket == -1) {
			perror("problem of accepting client request, quit\n");
			exit(1);
		}
		bzero(buffer,256);
     	n = read(client_socket,buffer,255);
     	if (n < 0) perror("ERROR reading from socket");
     	printf("Here is the message: %s\n",buffer);
     	n = write(client_socket,"I got your message.\n",20);
    	if (n < 0) perror("ERROR writing to socket");

    	//use created thread to handle request
    	//multiple clients will need to wait all clients send its message -- problem
    	long client_socket_tmp = (long) client_socket;	
    	if(pthread_create(&newthread, NULL, accept_request, (void *)client_socket_tmp) != 0)
    		perror("pthread_create failed");
    	//pthread_join(newthread, NULL);
	}
	pthread_exit(NULL);
	close(server_socket);

	return(0);

}
