#include <iostream>
#include <unistd.h>
#include <errno.h>
#include <cstring>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
using namespace std;

#define PORT "80"
#define MAXDATASIZE 100 //no. of bytes to get at once

//get sockadrr. void * coz it can be either IPv4 or IPv6
void * get_in_addr(struct sockaddr* sa){
    if(sa->sa_family==AF_INET){
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(){
    const char* host="google.com";

    int sockfd,numbytes;
    char buf[MAXDATASIZE];
    struct addrinfo hints,*servinfo,*p;
    int rv;
    char s[INET6_ADDRSTRLEN]; //46

    memset(&hints,0,sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    rv = getaddrinfo(host, PORT, &hints, &servinfo);
    if(rv!=0){
        cout << gai_strerror(rv) << endl;
        return 1;
    }

    for(p=servinfo;p!=NULL;p=p->ai_next){
        if((sockfd=socket(p->ai_family,p->ai_socktype,p->ai_protocol))==-1){
            perror("client : socket");
            continue;
        }

        inet_ntop(p->ai_family,get_in_addr((struct sockaddr *)p->ai_addr),s,sizeof s);
        cout << "client: attempting connection to " << s << '\n';
        
        if(connect(sockfd,p->ai_addr,p->ai_addrlen)==-1){
            perror("client: connect");
            close(sockfd);
            continue;
        }
        const char* msg =
            "GET / HTTP/1.1\r\n"
            "Host: google.com\r\n"
            "Connection: close\r\n"
            "\r\n";

        send(sockfd, msg, strlen(msg), 0);

        break;
    }

    if(p==NULL){
        cout << "client: failed to connect\n";
        return 2;
    }

    inet_ntop(p->ai_family,get_in_addr((struct sockaddr *)p->ai_addr),s,sizeof s);
    cout << "connected to " << s << endl;

    freeaddrinfo(servinfo);

    buf[numbytes]='\0';

    if((numbytes=recv(sockfd,buf,MAXDATASIZE,0))==-1){
        perror("recv");
        exit(1);
    }
    cout << "client: received "<< buf << endl;
    close(sockfd);
    return 0; 
}