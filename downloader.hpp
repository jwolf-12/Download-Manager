#pragma once

#include "downloadoptions.hpp"
#include <fstream>
#include "httpclient.hpp"
#include <chrono>
#include <vector>

class Downloader{
private:
    HttpClient client;
    long long bytesRecv;
    chrono::time_point<chrono::high_resolution_clock> start,last;
    long long totalSize=0;

void writer(const char*buffer,int size,ofstream& out){
    out.write(buffer,size);

    bytesRecv+=size;

    auto end=chrono::high_resolution_clock::now();
    double elapse = chrono::duration<double>(end - last).count();

    if(elapse>0.5) {
        double seconds = chrono::duration<double>(end - start).count();
        cout << bytesRecv*100.0/totalSize <<"% done  Speed: " << bytesRecv/(seconds*(1024.0)) << " kbs" <<endl;
        last=end;
    }
}

vector<Range> createRanges(long long totalSize, int threads){
    long long size=totalSize/threads;

    vector<Range> ranges;

    for(int i=0;i<threads;i++){
        long long end = size*(i+1)-1;
        if(i==threads-1) end=totalSize-1;
        ranges.push_back(Range({size*i,end}));
        if(totalSize<=size*(i+1)-1) break;
    }

    return ranges;
}

void downloadRange(string url,string file,Range* range){
    struct downloadOptions options;
    options.range=*range;

    ofstream out;
    out.open(file,ios::binary);
    if(!out){
        throw runtime_error("file creation failed");
    }

    client.sendLarge(url,&options,[&](const char*buffer,int size){
        writer(buffer,size,out);
    });

    out.close();
}

public:

    void download(string url,string file){

        bytesRecv=0;

        start=chrono::high_resolution_clock::now();
        last=start;

        int threads=4;
        totalSize=client.getSize(url);
        if(totalSize==-1){
            throw runtime_error("failed to get size");
        }

        vector<Range> ranges = createRanges(totalSize,threads);

        for(int i=0;i<ranges.size();i++){
            downloadRange(url,"part"+to_string(i)+".zip",&ranges[i]);
        }
    }
};