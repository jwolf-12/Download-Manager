#pragma once

#include <iostream>
#include <unistd.h>
#include <cstring>
#include <sys/types.h>
#include <functional>

#include "socket.hpp"
#include "response.hpp"
#include "request.hpp"
using namespace std;

class HttpClient{
public:
    // Response send(string domain,string port,Request req){

    //     Socket socket;

    //     if(socket.Connect(domain,port)==-1)
    //         throw runtime_error("connection failed");
    //     if(socket.Send(req.buildRequest())==-1)
    //         throw runtime_error("send_failed");
    //     Response res; 
    //     string raw = socket.Receive();
    //     if(raw==""){
    //         throw runtime_error("response failed");
    //     }
    //     res.parseResponse(raw);
    //     socket.Disconnect();
    //     return res;
    // }

    void sendLarge(string domain,string port,Request req,function<void(const char*,int)> callback){
        Socket socket;

        if(socket.Connect(domain,port)==-1)
            throw runtime_error("connection failed");
        if(socket.Send(req.buildRequest())==-1)
            throw runtime_error("send_failed");

        char buffer[4096];

        bool parsedHeaders=false;

        string headerPart;

        while(true){
            int bytes=socket.Receive(buffer,4096);
            if(bytes<=0) break;
            if(!parsedHeaders){
                string raw(buffer,bytes);
                headerPart+=raw;
                size_t pos = headerPart.find("\r\n\r\n");
                if(pos==string::npos){
                    continue;
                }
                Response res;
                res.parseHeaders(headerPart.substr(0,pos));

                int bodyStart=pos-headerPart.size()+bytes+4;
                callback(buffer+bodyStart,bytes-bodyStart);
                parsedHeaders=true;
            }
            else callback(buffer,bytes);
        }
        
        socket.Disconnect();
    }
};