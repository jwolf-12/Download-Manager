#pragma once

#include <fstream>
#include "httpclient.hpp"
#include "request.hpp"

class Downloader{
private:
    HttpClient client;
    string file;
    string domain;
    string port;
    Request req;
    ofstream out;

public:

    void writer(const char*buffer,int size){
        out.write(buffer,size);
    }


    void download(string url,string file){
        this->file=file;
        out.open(file,ios::binary);
        if(!out){
            throw runtime_error("file creation failed");
        }
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

        client.sendLarge(domain,port,req,[&](const char*buffer,int size){
            writer(buffer,size);
        });

        out.close();
    }
};