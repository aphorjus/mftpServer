
#define _BSD_SOURCE

#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <ctype.h>
#include <fcntl.h>

#define PORT_NUMBER 49999
#define RW_SIZE 256
#define MAX_PATH 4096


void trim_newline( char* dest, char* sorce );
