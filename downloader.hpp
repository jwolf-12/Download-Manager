#pragma once

#include <fstream>
#include "httpclient.hpp"
#include <chrono>


class Downloader{
private:
    HttpClient client;
    string file;
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

        bytesRecv=0;

        start=chrono::high_resolution_clock::now();
        last=start;
        client.sendLarge(url,[&](const char*buffer,int size,int totalSize){
            writer(buffer,size,totalSize);
        });

        out.close();
    }
};