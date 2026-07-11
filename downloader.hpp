#pragma once

#include "downloadoptions.hpp"
#include <fstream>
#include "httpclient.hpp"
#include <chrono>
#include <vector>

class Downloader{
private:
    HttpClient client;
    long long bytesRecv=0;
    chrono::time_point<chrono::high_resolution_clock> start,last;
    long long totalSize=0;
    ofstream out;

void writer(const char*buffer,int size,long long st){
    out.seekp(st);
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
    }

    return ranges;
}

void downloadRange(string url,Range& range){
    struct downloadOptions options;
    options.range=range;
    long long st = range.start;
    client.sendLarge(url,&options,[&](const char*buffer,int size){
        writer(buffer,size,st);
        st+=size;
    });
}

public:

    void download(string url,string file){
        out.open(file,ios::binary);
        if(!out){
            throw runtime_error("file creation failed");
        }

        bytesRecv=0;

        start=chrono::high_resolution_clock::now();
        last=start;

        int threads=4;
        totalSize=client.getSize(url);
        if(totalSize==-1){
            throw runtime_error("failed to get size");
        }

        out.seekp(totalSize - 1);
        out.write("", 1);
        out.flush();

        vector<Range> ranges = createRanges(totalSize,threads);

        for(int i=0;i<ranges.size();i++){
            downloadRange(url,ranges[i]);
        }
        out.close();
    }
};