#define _BSD_SOURCE

#include "mftp.h"

// Austin Horjus

#define HAS_PATH 2

void f_error( char* message){
	fprintf(stderr, "FATAL ERROR: %s\n", message);
	exit(1);
}

void error( char* message ){
	fprintf(stderr, "ERROR: %s\n", message);
}

int create_connect_socket(int port_number, char* hostName){

	struct sockaddr_in servAddr;
	struct hostent* hostEntry;
	struct in_addr** pptr;
	int socketfd;
	
	if((socketfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		perror("ERROR: failed to open socket");
		return(-1);
	}
	memset(&servAddr, 0, sizeof(servAddr));
	servAddr.sin_family = AF_INET;
	servAddr.sin_port = htons(port_number);

	if((hostEntry = gethostbyname(hostName)) == NULL){
		herror("ERROR: unable to resolve host name");
		return(-1);
	}
	pptr = (struct in_addr **) hostEntry->h_addr_list;
	memcpy(&servAddr.sin_addr, *pptr, sizeof(struct in_addr));

	if(connect(socketfd, (struct sockaddr*) &servAddr, sizeof(servAddr)) < 0){
		perror("ERROR: failed to connect");
		return(-1);
	}
	return socketfd;
}


int is_valid_command(char* input){

	char* cmd = strdup(input);
	/*char* cmd = */strtok(cmd, " ");

	if( !strcmp(cmd, "exit\n") || !strcmp(cmd, "ls\n")  ||
		!strcmp(cmd, "rls\n")) {
		return 1;
	}
	else if( !strcmp(cmd, "cd")  || !strcmp(cmd, "rcd") || 
			 !strcmp(cmd, "get") || !strcmp(cmd, "show") ||
			 !strcmp(cmd, "put") ){
		return 2;
	}
	else if( !strcmp(cmd, "cd\n")  || !strcmp(cmd, "rcd\n") || 
			 !strcmp(cmd, "get\n") || !strcmp(cmd, "show\n") ||
			 !strcmp(cmd, "put\n") ){
		printf("argument required for %s", cmd);
	}

	free(cmd);
	return 0;
}

void local_ls() {

	if( fork() ){
		wait(NULL);
	}
	else{
		int fd[2];
		pipe(fd);

		int rdr = fd[0];
		int wtr = fd[1];

		if ( !fork() ){
			close(rdr);
			dup2(wtr, STDOUT_FILENO);
			if( execl("/bin/sh", "/bin/sh", "-c", "ls -l", 0) ){error("ls error");}
			exit(1);
		}
		else{
			close(wtr);
			dup2(rdr, STDIN_FILENO);
			if( execl("/bin/sh", "/bin/sh", "-c", "more -20", 0) ){error("more error");}
			exit(1);
		} 
	}	
}

void trim_newline( char* dest, char* sorce ){
	int i = 0;

	while(sorce[i] != '\n'){
		dest[i] = sorce[i];
		i++;
	}
	dest[i] = '\0';
}

void get_cat_str( char* path, char* cat_str ){
	strcpy(cat_str, "cat > ");
	trim_newline(&cat_str[6], path);
}

void receive_data( int socket, char* path, int cat ) {
    if ( fork () ) {
        wait(NULL); // receive 
    } else {
        dup2(socket, 0);
        close(socket);
        if(cat){
        	char cat_str[MAX_PATH+6];
        	get_cat_str( path, cat_str );
        	if( execl("/bin/sh", "/bin/sh", "-c", cat_str, 0) ){
        		error("transfer error");
        	}
        	printf("%s\n", "l");
        }
        else{
        	if( execl("/bin/sh", "/bin/sh", "-c", "more -20", 0) ){
        		error("transfer error");
        	}
        }
        exit(1);
    }
}

void send_data( int socket, char* path ){

	char clean_path[MAX_PATH];
	trim_newline(clean_path, path);

	int fd = open( clean_path, O_RDONLY, 0644 );
	char buf[1];

	while( read(fd, buf, 1) ){
		if ( write( socket, buf, 1 ) < 0){f_error("write error");}
	}
	close(fd);
}

int get_data_port(char* acknowlegement){
	char* port_str = &(acknowlegement[1]);
	return atoi(port_str);
}

int get_acknowledgement( int socketfd, char* output ){

	memset(output, 0, strlen(output));
	if( read( socketfd, output, RW_SIZE ) < 0 ){f_error( "read error");}
	
	if(output[0] == 'A'){
		return 0;
	}
	else{
		return -1;
	}
}

void send_to_server( int socketfd, char* cmd, char* path ){

	if(!strcmp("no_path", path)){
		if( write(socketfd, cmd, strlen(cmd)) < 0 ){f_error("write error");}
		if( write(socketfd, "\n", 1) < 0 ){f_error( "write error");}
	}
	else{
		if( write(socketfd, cmd, strlen(cmd)) < 0 ){f_error("write error");}
		if( write(socketfd, path, strlen(path)) < 0 ){f_error("write error");}
	}
}

////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////

int proform_data_operation( char* hostName, int socketfd, 
							char* server_command, char* path, int cat ){
	char ack[RW_SIZE];

	send_to_server( socketfd, "D", "no_path" );
	if(get_acknowledgement( socketfd, ack )){ // reads
		error(&ack[1]);
		return -1;
	} 

	int port = get_data_port( ack );
	printf("%d\n", port);
	int data_socket = create_connect_socket( port, hostName );

	send_to_server( socketfd, server_command, path );
	if(get_acknowledgement( socketfd, ack )){ 
		error(&ack[1]);
		return -2; 
	}

	if(server_command[0] == 'P'){
		send_data(data_socket, path);
	}
	else{
		receive_data( data_socket, path, cat );	
	}
	shutdown( data_socket, 2 );
	return 0;
}

////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////


int exists(char* path){

	char clean_path[strlen(path)-1];
	trim_newline(clean_path, path);

	if( access( clean_path, F_OK ) != -1 ){
		return 1;
	}
	return 0;
}

void cd(char* path){

	char cd_path[strlen(path)-1];
	trim_newline(cd_path, path);

	if(chdir(cd_path)){
		printf("error in chdir on path: %s\n", cd_path);
	}
}

void rcd(char* path, int socketfd){
	char ack[RW_SIZE];
	send_to_server(socketfd, "C", path);
	get_acknowledgement( socketfd, ack ); // reads
}

void rls( char* hostName, int socketfd ){
	proform_data_operation(hostName, socketfd, "L", "no_path", 0);
}

void show( char* hostName, int socketfd, char* path ){
	proform_data_operation(hostName, socketfd, "G", path, 0);
}

void get( char* hostName, int socketfd, char* path ){
	proform_data_operation(hostName, socketfd, "G", path, 1);
}

void put( char* hostName, int socketfd, char* path ){
	
	if( exists( path ) ){
		proform_data_operation(hostName, socketfd, "P", path, 0);
	}
	else{
		error("invalid path");
	}
}

void fml(int socketfd, char* hostName){

	char input[MAX_PATH];
	char* path;
	int has_path;

	while(1){
		printf("%s", "mftp-server$ ");
		fgets( input, MAX_PATH, stdin );

		if(!(has_path = is_valid_command(input))){
			printf("%s\n", "invalid command");
		}

		char* cmd = strtok( input,  " ");

		if(has_path == HAS_PATH){
			path = strtok( NULL, " " );
			if(!strcmp(cmd, "cd")){
				cd(path);
			}
			else if(!strcmp(cmd, "rcd")){
				rcd( path, socketfd );
			}
			else if(!strcmp(cmd, "get")){
				get( hostName, socketfd, path );
			}
			else if(!strcmp(cmd, "show")){
				show( hostName, socketfd, path );
			}
			else if (!strcmp(cmd, "put")){
				put( hostName, socketfd, path );
			}
		}
		else {

			if(!strcmp(cmd, "exit\n")){
				send_to_server(socketfd, "Q\n", "no_path");
				return;
			}
			else if(!strcmp(cmd, "ls\n")){
				local_ls();
			}
			else if(!strcmp(cmd, "rls\n")){
				rls( hostName, socketfd );
			}
		}
	}
}

int main(int argc, char* argv[]){

	if(argc < 2){
		fprintf(stderr, "%s\n", "ERROR: too few arguments");
		exit(1);
	}

	char* hostName = argv[1];
	int socketfd = create_connect_socket(PORT_NUMBER, hostName);
	if(socketfd < 0){
		exit(1);
	}

	printf("%s\n", "connected to server");
	fml(socketfd, hostName);
	return 0;
}

