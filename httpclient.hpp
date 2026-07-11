#pragma once

#include <iostream>
#include <unistd.h>
#include <cstring>
#include <sys/types.h>
#include <functional>

#include "downloadoptions.hpp"
#include "socket.hpp"
#include "response.hpp"
#include "request.hpp"
using namespace std;

class HttpClient{

public:

    int getSize(const string& url){
        Socket socket;

        string domain;
        string port;
        Request req;
        int pos = url.find("://");
        pos+=3;
        size_t colon = url.substr(pos).find(":");
        if(colon==string::npos){
            port="80";
            int slash = url.substr(pos).find("/")+pos;
            domain=url.substr(pos,slash-pos);
            req.path=url.substr(slash);
        }
        else{
            colon+=pos;
            domain=url.substr(pos,colon-pos);
            int slash = url.substr(colon).find("/")+colon;
            port=url.substr(colon+1,slash-colon-1);
            req.path = url.substr(slash);
        }
        req.method="HEAD";
        req.headers["Host"]=domain;
        req.version ="HTTP/1.1";

        if(socket.Connect(domain,port)==-1)
            throw runtime_error("connection failed");
        if(socket.Send(req.buildRequest())==-1)
            throw runtime_error("send_failed");

        char buffer[4096];

        bool parsedHeaders=false;

        string headerPart;
        int totalSize=0;
        int received=0;

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

                if(res.statusCode==301 || res.statusCode == 302){
                    string url=res.getHeader("Location");
                    int x=getSize(url);
                    socket.Disconnect();
                    return x;
                }

                return stoi(res.getHeader("Content-Length"));
            }
        }
        return -1;
    }

    void sendLarge(const string& url,struct downloadOptions * options, function<void(const char*,int)> callback){
        Socket socket;

        string domain;
        string port;
        Request req;
        int pos = url.find("://");
        pos+=3;
        size_t colon = url.substr(pos).find(":");
        if(colon==string::npos){
            port="80";
            int slash = url.substr(pos).find("/")+pos;
            domain=url.substr(pos,slash-pos);
            req.path=url.substr(slash);
        }
        else{
            colon+=pos;
            domain=url.substr(pos,colon-pos);
            int slash = url.substr(colon).find("/")+colon;
            port=url.substr(colon+1,slash-colon-1);
            req.path = url.substr(slash);
        }
        req.method="GET";
        req.headers["Host"]=domain;
        req.version ="HTTP/1.1";

        if(options->range){
            req.headers["Range"]="bytes="+to_string(options->range->start)+"-"+to_string(options->range->end);
        }

        if(socket.Connect(domain,port)==-1)
            throw runtime_error("connection failed");
        if(socket.Send(req.buildRequest())==-1)
            throw runtime_error("send_failed");

        // cout << req.buildRequest() << endl;

        char buffer[4096];

        bool parsedHeaders=false;

        string headerPart;
        int totalSize=0;
        int received=0;

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

                if(res.statusCode==301 || res.statusCode == 302){
                    string url=res.getHeader("Location");
                    sendLarge(url,options,callback);
                    socket.Disconnect();
                    return;
                }

                cout << "Status: " << res.statusCode << endl;
                cout << "Length: " << res.getHeader("Content-Length") << endl;

                if(res.headers.count("Content-Range"))
                    cout << "Range: " << res.getHeader("Content-Range") << endl;

                int bodyStart=pos-headerPart.size()+bytes+4;
                totalSize=stoi(res.getHeader("Content-Length"));
                received=bytes-bodyStart;
                callback(buffer+bodyStart,bytes-bodyStart);
                parsedHeaders=true;
            }
            else{
                callback(buffer,bytes);
                received+=bytes;
            }
            if(received==totalSize) break;
        }
        
        socket.Disconnect();
    }
};