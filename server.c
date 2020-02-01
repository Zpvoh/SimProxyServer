#include "server.h"

int parse_request(int sockfd, char *method, char *uri, char *buff)
{
    //char buff[MAX_REQUEST_LEN] = {0};
    ssize_t len = recv(sockfd, buff, sizeof(buff), 0);
    if (len <= 0)
    {
        LOG("call recv error, ret %d", (int)len);
        return -1;
    }

    char *cur = buff;
    printf("content: %s\n", buff);
    int i = 0;
    while (i < MAX_METHOD_LEN && !isspace(*cur))
    {
        method[i++] = *cur++;
    }
    method[i] = '\0';

    while (isspace(*cur))
        cur++;
    i = 0;
    while (i < MAX_URI_LEN && !isspace(*cur))
    {
        uri[i++] = *cur++;
    }
    uri[i] = '\0';
    return 0;
}

int handle_request(int sockfd)
{
    int result = KEEP_ALIVE;
    char buff[MAX_REQUEST_LEN] = {0};
    ssize_t len = recv(sockfd, buff, sizeof(buff), 0);
    if (len <= 0)
    {
        LOG("call recv error, ret %d", (int)len);
        return -1;
    }

    char *cur = buff;
    printf("content: %s", buff);

    char method[MAX_METHOD_LEN] = {0};
    int i = 0;
    while (i < MAX_METHOD_LEN && !isspace(*cur))
    {
        method[i++] = *cur++;
    }
    method[i] = '\0';

    while (isspace(*cur))
        cur++;
    char uri[MAX_URI_LEN] = {0};
    i = 0;
    while (i < MAX_URI_LEN && !isspace(*cur))
    {
        uri[i++] = *cur++;
    }
    uri[i] = '\0';

    // struct sockaddr_in dest_addr = uri2ip(uri);
    // char response[2048] = {0};
    // forward(request, dest_addr, response);
    if (strcmp(method, "CONNECT") == 0)
    {
        send(sockfd, "HTTP/1.1 200 Connection Established", 2048, 0);
    }
    else
    {
        struct sockaddr_in dest_addr = uri2ip(uri);
        char response[2048] = {0};
        forward(buff, dest_addr, response);
        send(sockfd, response, 2048, 0);
    }

    return result;
}

void forward(char *packet, struct sockaddr_in dest_addr, char *response)
{
    int recv_num = 0;
    int proxy_sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    LOG("start forwarding...");
    int proxy_ret = connect(proxy_sockfd, (struct sockaddr *)&dest_addr, sizeof(struct sockaddr));
    LOG("connected to destination!");
    send(proxy_sockfd, packet, sizeof(packet), 0);
    LOG("request is sent. Waiting...");

    char *buf = response;
    while (1)
    {
        recv_num = recv(proxy_sockfd, buf, 2048, 0);
        LOG("recv num: %d", recv_num);
        buf += recv_num;
        if (recv_num == 0 || recv_num == -1)
        {
            break;
        }
    }

    LOG("response: %s", response);
}

void unimplemented(int client)
{
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

void url_decode(const char *src, char *dest)
{
    const char *p = src;
    char code[3] = {0};
    while (*p && *p != '?')
    {
        if (*p == '%')
        {
            memcpy(code, ++p, 2);
            *dest++ = (char)strtoul(code, NULL, 16);
            p += 2;
        }
        else
        {
            *dest++ = *p++;
        }
    }
    *dest = '\0';
}

struct sockaddr_in uri2ip(char *uri)
{
    struct sockaddr_in result;
    memset(&result, 0, sizeof(result));

    char *hostname;
    char *port;
    char *buff;
    buff = uri;
    hostname = strsep(&buff, ":");
    port = strsep(&buff, ":");
    port = strsep(&port, "/");
    printf("%s, %s\n", hostname, port);
    int port_num = atoi(port);
    result.sin_port = htons(port_num);

    struct hostent *hptr = gethostbyname(hostname);
    char **pptr;
    char str[INET_ADDRSTRLEN];
    switch (hptr->h_addrtype)
    {
    case AF_INET:
        result.sin_family = AF_INET;
        result.sin_addr.s_addr = inet_addr(inet_ntop(AF_INET, hptr->h_addr, str, sizeof(str)));
        // pptr = hptr->h_addr_list;
        // for (; *pptr != NULL; pptr++)
        // {
        //     printf("\taddress: %s\n",
        //            inet_ntop(hptr->h_addrtype, *pptr, str, sizeof(str)));
        // }
        break;
    default:
        printf("unknown address type\n");
        break;
    }

    return result;
}

void do_get(int sockfd, const char *uri)
{
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

void *process(void *psockfd)
{
    int sockfd = *(int *)psockfd;

    if (handle_request(sockfd) != KEEP_ALIVE)
    {
        close(sockfd);
        return NULL;
    }

    while(handle_request(sockfd)==KEEP_ALIVE);

    close(sockfd);
    return NULL;
}

int create_server_fd(unsigned int port)
{
    int serverfd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverfd == -1)
        EXIT("create socket fail");

    struct sockaddr_in server;
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = INADDR_ANY;

    if (bind(serverfd, (struct sockaddr *)&server, sizeof(server)) == -1)
        EXIT("bind fail");

    if (listen(serverfd, 10) == -1)
        EXIT("listen fail");

    return serverfd;
}

int main()
{
    int serverfd, connfd;
    pthread_t tid;
    struct sockaddr_in client;
    socklen_t clientlen = sizeof(client);
    unsigned int port = 8888;

    serverfd = create_server_fd(port);
    LOG("Server started, listen port %d", port);
    while (1)
    {
        connfd = accept(serverfd, (struct sockaddr *)&client, &clientlen);
        if (pthread_create(&tid, NULL, process, &connfd) == 0)
        {
            unsigned char *ip = (unsigned char *)&client.sin_addr.s_addr;
            unsigned short port = client.sin_port;
            LOG("request %u.%u.%u.%u:%5u", ip[0], ip[1], ip[2], ip[3], port);
        }
        else
        {
            EXIT("create thread fail");
        }
    }
    return 0;
}