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

class Request{
public:
    string method;
    string path;
    string version;

    unordered_map<string,string> headers;

    string body;

    string buildRequest(){
        string raw;
        raw+=method;
        raw+=' ';
        raw+=path;
        raw+=' ';
        raw+=version;
        raw+="\r\n";
        for(auto const & p : headers){
            raw+=p.first; raw+=": ";
            raw+=p.second+"\r\n";
        }
        raw+="\r\n";
        raw+=body;
        return raw;
    }
};
