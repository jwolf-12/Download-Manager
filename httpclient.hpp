#pragma once

#include <iostream>
#include <unistd.h>
#include <cstring>
#include <sys/types.h>

#include "socket.hpp"
#include "response.hpp"
#include "request.hpp"
using namespace std;

class HttpClient{
public:
    Response send(string domain,string port,Request req){

        Socket socket;

        if(socket.Connect(domain,port)==-1)
            throw runtime_error("connection failed");
        if(socket.Send(req.buildRequest())==-1)
            throw runtime_error("send_failed");
        Response res; 
        string raw = socket.Receive();
        if(raw==""){
            throw runtime_error("response failed");
        }
        res.parseResponse(raw);
        socket.Disconnect();
        return res;
    }
};