#pragma once

#include "downloadoptions.hpp"
#include <fstream>
#include "httpclient.hpp"
#include <chrono>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>

class Downloader{
private:
    HttpClient client;
    atomic<long long> bytesRecv=0;
    chrono::time_point<chrono::high_resolution_clock> start,last;
    long long totalSize=0;
    mutex progressMutex;
    mutex fileMutex;

void writer(const char*buffer,int size,long long st,fstream& out){
    out.seekp(st);
    out.write(buffer,size);

    bytesRecv+=size;

    auto end=chrono::high_resolution_clock::now();
    lock_guard<mutex> lock(progressMutex);
    double elapse = chrono::duration<double>(end - last).count();

    if(elapse>2) {
        double seconds = chrono::duration<double>(end - start).count();
        cout << bytesRecv*100.0/totalSize <<"% done  Speed: " << bytesRecv/(seconds*(1024.0*1024)) << " mbs" <<endl;
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

void downloadRange(string url,string file,Range& range){
    fstream out(file,ios::in | ios::out | ios::binary);
    if(!out){
        throw runtime_error("file creation failed");
    }

    struct downloadOptions options;
    options.range=range;
    long long st = range.start;
    HttpClient client;
    client.sendLarge(url,&options,[&](const char*buffer,int size){
        writer(buffer,size,st,out);
        st+=size;
    });

    out.close();
}

public:

    void download(string url,string file){
        ofstream out(file,ios::binary);
        if(!out){
            throw runtime_error("file creation failed");
        }

        bytesRecv=0;

        start=chrono::high_resolution_clock::now();
        last=start;

        int threads=32;
        totalSize=client.getSize(url);
        if(totalSize==-1){
            throw runtime_error("failed to get size");
        }

        out.seekp(totalSize - 1);
        out.write("", 1);
        out.flush();
        out.close();

        vector<Range> ranges = createRanges(totalSize,threads);

        vector<thread> workers;
        for(auto& range:ranges){
            workers.emplace_back(&Downloader::downloadRange,this,url,file,ref(range));
        }

        for(auto& t:workers){
            t.join();
        }

        auto end=chrono::high_resolution_clock::now();
        double seconds = chrono::duration<double>(end - start).count();
        cout << totalSize/(1024*1024.0) << "megabytes downloaded in " << seconds << " seconds." << endl;

    }
};