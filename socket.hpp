#pragma once

#include <iostream>
#include <unistd.h>
#include <errno.h>
#include <cstring>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unordered_map>
#include <sstream>
using namespace std;

#define MAXDATASIZE 4096

class Socket{
private:
    string port;
    string domain;
    int sockfd=-1;

public:
    int Connect(string domain,string port){
        this->domain=domain;
        this->port=port;

        struct addrinfo hints;
        struct addrinfo * servinfo;
        memset(&hints,0,sizeof hints);
        hints.ai_family=AF_UNSPEC;
        hints.ai_socktype=SOCK_STREAM;
        int rv = getaddrinfo(domain.c_str(),port.c_str(),&hints,&servinfo);

        if(rv!=0){
            return -1;
        }

        struct addrinfo * p;
        for(p=servinfo;p!=NULL;p=p->ai_next){
            if((sockfd=socket(p->ai_family,p->ai_socktype,p->ai_protocol))==-1) continue;

            if(connect(sockfd,p->ai_addr,p->ai_addrlen)==-1){
                close(sockfd);
                continue;
            }

            break;
        }
        freeaddrinfo(servinfo);
        if(p==NULL){
            if(sockfd!=-1){
                close(sockfd);
                sockfd=-1;
            }
            return -1;
        }
        return 0;
    }

    int Send(string req){
        if(send(sockfd,req.c_str(),req.size(),0)==-1) return -1;
        return 0;
    }

    int Receive(char *buffer,int size){
        return recv(sockfd,buffer,size,0);
    }

    void Disconnect(){
        if(sockfd!=-1){
            close(sockfd);
            sockfd=-1;
        }
    }
};
