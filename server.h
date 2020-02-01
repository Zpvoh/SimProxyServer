#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <ctype.h>
#include <netdb.h>

#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#define LOG(fmt, ...) printf(fmt" %s:%d\n", ##__VA_ARGS__, __FILENAME__, __LINE__)
#define EXIT(error) do {perror(error); exit(EXIT_FAILURE);} while(0)

#define MAX_REQUEST_LEN 10240
#define MAX_METHOD_LEN  32
#define MAX_URI_LEN     256

#define KEEP_ALIVE 0
#define CLOSE_CONNECTION 1

struct sockaddr_in uri2ip(char*);
void forward(char*, struct sockaddr_in, char*);