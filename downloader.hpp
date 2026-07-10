#pragma once

#include <fstream>
#include "httpclient.hpp"
#include "request.hpp"
#include <chrono>


class Downloader{
private:
    HttpClient client;
    string file;
    string domain;
    string port;
    Request req;
    ofstream out;
    int bytesRecv;
    chrono::time_point<chrono::high_resolution_clock> start,last;
    double seconds;

public:

    void writer(const char*buffer,int size,int totalSize){
        out.write(buffer,size);

        bytesRecv+=size;

        auto end=chrono::high_resolution_clock::now();
        double elapse = chrono::duration<double>(end - last).count();

        if(elapse>2) {
            double seconds = chrono::duration<double>(end - start).count();
            cout << bytesRecv*100.0/totalSize <<"% done  Speed: " << bytesRecv/(seconds*(1024.0)) << " kbs" <<endl;
            last=end;
        }
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
        bytesRecv=0;

        start=chrono::high_resolution_clock::now();
        last=start;
        client.sendLarge(domain,port,req,[&](const char*buffer,int size,int totalSize){
            writer(buffer,size,totalSize);
        });

        out.close();
    }
};