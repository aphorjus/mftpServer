#define _BSD_SOURCE

#include "mftp.h"

// Austin Horjus 

void trim_newline( char* dest, char* sorce ){
	int i = 0;

	while(sorce[i] != '\n'){
		dest[i] = sorce[i];
		i++;
	}
	dest[i] = '\0';
}

void f_error( char* message){
	fprintf(stderr, "FATAL ERROR: %s\n", message);
	exit(1);
}

int make_socket( int port_num, struct sockaddr_in servAddr ){

	int listenfd;
	// struct sockaddr_in servAddr;

	listenfd = socket(AF_INET, SOCK_STREAM, 0);

	if(listenfd < 0){
		fprintf(stderr, "%s\n", "ERROR: failed to open socket");
	}

	memset(&servAddr, 0, sizeof(servAddr));
	servAddr.sin_family = AF_INET;
	servAddr.sin_port = htons(port_num);
	servAddr.sin_addr.s_addr = htonl(INADDR_ANY);

	if ( bind( listenfd, (struct sockaddr *) &servAddr, sizeof(servAddr)) < 0){
		perror("ERROR: failed to bind");
		exit(1);	
	}
	return listenfd;
}

int listen_accept_connection( int listenfd, struct sockaddr_in clientAddr, int backlog ){

	listen(listenfd, backlog);
	socklen_t length = sizeof(struct sockaddr_in);
	int connectfd = accept(listenfd, ( struct sockaddr* ) &clientAddr, &length);

	return connectfd;	
}

int read_from_client(int socket, char* ret_str, int len){
	memset(ret_str, 0, len);
	char c[1];
	int i = 0;
	while(c[0]!='\n'){
		if( read(socket, c, 1) < 0 ){f_error("read error");}
		ret_str[i++] = c[0];
	}
	printf("%s\n", ret_str);
	return 0;
}

int get_port( int listenfd ){

	struct sockaddr_in servAddr;
	memset(&servAddr, 0, sizeof(servAddr));
	// int listenfd = make_socket(0, servAddr);
	socklen_t adderlen = sizeof(servAddr);
	// int lfd;

	// printf("%d\n", listenfd);
	getsockname( listenfd, (struct sockaddr *) &servAddr, &adderlen );
	// printf("%d\n", servAddr.sin_port);
	return servAddr.sin_port;
}

void make_ack( int err, char* ack, int socket ){
	memset(ack, 0, RW_SIZE);

	if( socket ){
		snprintf(ack, 10, "A%d\n", socket);
	}
	else if(err){
		ack[0] = 'E';
		ack[1] = '\n';
		ack[2] = '\0';
	}
	else{
		ack[0] = 'A';
		ack[1] = '\n';
		ack[2] = '\0';
	}
	printf("ack: %s", ack);
	printf("socket: %d\n", socket);
}

void send_ack( int err, int socket, char* ack ){
	make_ack(err, ack, 0);
	write(socket, ack, strlen(ack));
}

int establish_data_connection( int connectfd ){

	struct sockaddr_in servAddr;
	struct sockaddr_in clientAddr;

	int listenfd = socket(AF_INET, SOCK_STREAM, 0);

	if(listenfd < 0){
		fprintf(stderr, "%s\n", "ERROR: failed to open socket");
	}

	memset(&servAddr, 0, sizeof(servAddr));
	servAddr.sin_family = AF_INET;
	servAddr.sin_port = htons( 0 );
	servAddr.sin_addr.s_addr = htonl(INADDR_ANY);

	if ( bind( listenfd, (struct sockaddr *) &servAddr, sizeof(servAddr)) < 0){
		perror("ERROR: failed to bind");
		exit(1);	
	}

	socklen_t adderlen = sizeof(servAddr);

	listen( listenfd, 1 );
	// printf("%d\n", listenfd);

	getsockname( listenfd, (struct sockaddr *) &servAddr, &adderlen );

	char *butt = malloc(sizeof(char) * 10);
	snprintf(butt, 10, "A%d\n", ntohs(servAddr.sin_port));
	write(connectfd, butt, strlen(butt));

	socklen_t clength = sizeof(clientAddr);
	int socket_fd = accept(listenfd, ( struct sockaddr* ) &servAddr, &clength);

	return socket_fd;
}

void s_rcd( char* path ){
	char clean_path[MAX_PATH];
	trim_newline( clean_path, path );
	chdir( clean_path );
}

void s_rls( int socket ){
	
	int fd[2];
	pipe(fd);

	int rdr = fd[0];
	int wtr = fd[1];
	char* buf[1];

	if ( !fork() ){
		close(rdr);
		dup2(wtr, STDOUT_FILENO);
	}
	else{
		close(wtr);
		wait(NULL);
	}
	while( read(rdr, buf, 1) ){
		write( socket, buf, 1 );
	}
}

void send_file( int data_socket, char* path ){

	printf("%s\n", "the fuck?");

	char buf[1];
	int fd = open( path, O_RDONLY );

	printf("%s\n", "the fuck?");

	int i;
	while(read( fd, buf, 1 )){
		write( data_socket, buf, 1 );
		printf("%d-%s-", i++, buf);
		if(!strcmp("", buf)){break;}
	}
	close(fd);

	printf("%s\n", "the fuck?");
}

void save_file( int data_socket, char* path ){

	char buf[1];
	char* fn = strrchr(path, '/');

	if(!fn){
		fn = path;
	}
	printf("%s\n", fn);
	int fd = open( fn, O_WRONLY | O_CREAT, 0644 );

	while(read(data_socket, buf, 1)){
		write(fd, buf, 1);
	}
	close(fd);
}

void client_handler(int connectfd){
	// char input[RW_SIZE];
	char ack[RW_SIZE];
	char command[MAX_PATH];
	int data_socket = 0;
	printf("%s\n", "hello");

	while(1){
		printf("%s\n", "..........................");
		// read(connectfd, input, MAX_PATH);
		read_from_client( connectfd, command, MAX_PATH ); // reads

		if( command[0] == 'Q' ){
			send_ack( 0, connectfd, ack );
			printf("%s\n", "goodbye");
			exit(0);
		}
		else if(command[0] == 'D'){
			data_socket = establish_data_connection( connectfd );
		}
		else if(command[0] == 'C'){
			
			s_rcd( &command[1] );
			send_ack( 0, connectfd, ack );
		
		}
		else if(command[0] == 'L'){
			if(!data_socket){
				send_ack(1, connectfd, ack );
			}
			else{
				s_rls( data_socket );
				send_ack( 0, connectfd, ack );
				close( data_socket );
			}
			
		}
		else if(command[0] == 'G'){
			if(!data_socket){
				send_ack(1, connectfd, ack );
			}
			else{
				send_file(data_socket, &command[1]);
				send_ack( 0, connectfd, ack );
				close( data_socket );
			}
		}
		else if(command[0] == 'P'){
			if(!data_socket){
				send_ack(1, connectfd, ack );
			}
			else{
				send_ack( 0, connectfd, ack );
				save_file( data_socket, &command[1] );
				close( data_socket );
			}
		}
	}
}

int main(int argc, char* argv[]){

	int listenfd;
	int connectfd;
	struct sockaddr_in clientAddr;
	struct sockaddr_in servAddr;
	struct hostent* hostEntry;
	char* hostName;

	listenfd = make_socket(PORT_NUMBER, servAddr);

	/////////////////////////////////////////////////////////////////////
	int p_id;

 	while(1){
		connectfd = listen_accept_connection( listenfd, clientAddr, 4 );

		if((hostEntry = gethostbyaddr(&(clientAddr.sin_addr), 
						sizeof(struct in_addr), AF_INET)) == NULL){
			hostName = inet_ntoa( clientAddr.sin_addr );
		}
		else{
			hostName = hostEntry->h_name;
		}
		printf("host: %s\n", hostName);

		if(( p_id = fork() )){}
		else{ break; }
	}
	client_handler(connectfd);
	printf("%s\n", "child over");
	return 0;
}

