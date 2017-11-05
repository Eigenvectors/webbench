#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/time.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

int Socket(const char *host, int port)
{
    int sockfd;
    in_addr_t inaddr;
    struct sockaddr_in servaddr;
    struct hostent *hp;
    
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;

    inaddr = inet_addr(host);	//the function convert ip address
    if (inaddr != INADDR_NONE)
        memcpy(&servaddr.sin_addr, &inaddr, sizeof(inaddr));
    else
    {
        hp = gethostbyname(host);	//the function convert host name
        if (hp == NULL)
            return -1;
        memcpy(&servaddr.sin_addr, hp->h_addr, hp->h_length);
		//memcpy(&servaddr.sin_addr, hp -> h_addr_list[0], hp -> h_lenght);
    }
    servaddr.sin_port = htons(port);
    
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        return sockfd;
    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
        return -1;
    return sockfd;
}

