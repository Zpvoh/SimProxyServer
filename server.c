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

#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#define LOG(fmt, ...) printf(fmt" %s:%d\n", ##__VA_ARGS__, __FILENAME__, __LINE__)
#define EXIT(error) do {perror(error); exit(EXIT_FAILURE);} while(0)

#define MAX_REQUEST_LEN 10240
#define MAX_METHOD_LEN  32
#define MAX_URI_LEN     256

int parse_request(int sockfd, char *method, char *uri) {
    char buff[MAX_REQUEST_LEN] = {0};
    ssize_t len = recv(sockfd, buff, sizeof(buff), 0);
    if (len <= 0) {
        LOG("call recv error, ret %d", (int)len);
        return -1;
    }
    
    char *cur = buff;
    int i = 0;
    while (i < MAX_METHOD_LEN && !isspace(*cur)) {
        method[i++] = *cur++;
    }
    method[i] = '\0';

    while(isspace(*cur)) cur++;
    i = 0;
    while (i < MAX_URI_LEN && !isspace(*cur)) {
        uri[i++] = *cur++;
    }
    uri[i] = '\0';
    return 0;
}

void unimplemented(int client) {
    char buff[] =
        "HTTP/1.0 501 Method Not Implemented\r\n"
        "Content-Type: text/html\r\n"
        "\r\n"
        "Method Not Implemented";
    send(client, buff, sizeof(buff), 0);
}

void not_found(int client)
{
    char buff[] =
        "HTTP/1.0 404 NOT FOUND\r\n"
        "Content-Type: text/html\r\n"
        "\r\n"
        "The resource specified is unavailable.\r\n";
    send(client, buff, strlen(buff), 0);
}

void url_decode(const char *src, char *dest) {
    const char *p = src;
    char code[3] = {0};
    while (*p && *p != '?') {
        if(*p == '%') {
            memcpy(code, ++p, 2);
            *dest++ = (char)strtoul(code, NULL, 16);
            p += 2;
        } else {
            *dest++ = *p++;
        }
    }
    *dest = '\0';
}

void do_get(int sockfd, const char *uri) {
    char filename[MAX_URI_LEN] = {0};
    const char *cur = uri + 1;
    size_t len = strlen(cur);
    // if (len == 0) {
    //     strcpy(filename, "index.html");
    // } else {
    //     url_decode(cur, filename);
    // }
    // printf("%s\n", filename);
    
    // FILE *f = fopen(filename, "r");
    // if (NULL == f) {
    //     not_found(sockfd);
    //     return;
    // }

    char header[] =
        "HTTP/1.0 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "\r\n";
    send(sockfd, header, sizeof(header), 0);
    printf("uri is %s\n", uri);
    
    char line[128] = {0};
    send(sockfd, "hello world", 11, 0);
    // while (fgets(line, sizeof(line), f) != NULL) {
    //     send(sockfd, line, strlen(line), 0);
    //     memset(line, 0, sizeof(line));
    // }
    char end[] = "\r\n";
    send(sockfd, end, 2, 0);
    // fclose(f);
}

void do_process_packet(int sockfd, const char *uri){

}

void *process(void* psockfd) {
    int sockfd = *(int*)psockfd;
    char method[MAX_METHOD_LEN] = {0};
    char uri[MAX_URI_LEN] = {0};
    
    if (parse_request(sockfd, method, uri) != 0)
        goto FINAL;
    
    if (strcmp(method, "GET") == 0) {
        do_get(sockfd, uri);
    } else {
        unimplemented(sockfd);
    }

FINAL:
    close(sockfd);
    return NULL;
}

int create_server_fd (unsigned int port) {
    int serverfd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverfd == -1)
        EXIT("create socket fail");

    struct sockaddr_in server;
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = INADDR_ANY;

    if (bind(serverfd,(struct sockaddr *)&server, sizeof(server)) == -1)
        EXIT("bind fail");

    if (listen(serverfd, 10) == -1)
        EXIT("listen fail");

    return serverfd;
}

int main() {
    int serverfd, connfd;
    pthread_t tid;
    struct sockaddr_in client;
    socklen_t clientlen = sizeof(client);
    unsigned int port = 8888;

    serverfd = create_server_fd(port);
    LOG("Server started, listen port %d", port);
    while (1) {
        connfd = accept(serverfd, (struct sockaddr *)&client, &clientlen);
        if (pthread_create(&tid, NULL, process, &connfd) == 0) {
            unsigned char *ip = (unsigned char*)&client.sin_addr.s_addr;
            unsigned short port = client.sin_port;
            LOG("request %u.%u.%u.%u:%5u", ip[0], ip[1], ip[2], ip[3], port);
        } else {
            EXIT("create thread fail");
        }
    }
    return 0;
}