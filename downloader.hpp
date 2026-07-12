#pragma once

#include "downloadoptions.hpp"
#include <fstream>
#include "httpclient.hpp"
#include <chrono>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>

#define MAXRETRIES 5

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

vector<Chunk> createChunks(long long totalSize, int threads){
    long long size=totalSize/threads;

    vector<Chunk> chunks;

    for(int i=0;i<threads;i++){
        long long end = size*(i+1)-1;
        if(i==threads-1) end=totalSize-1;
        chunks.push_back(Chunk({Range({size*i,end})}));
    }

    return chunks;
}

void retryChunk(string url,string file,vector<reference_wrapper<Chunk>>& chunks){

    vector<thread> workers;

    for(auto &chunkRef: chunks){
        Chunk& chunk=chunkRef.get();

        chunk.failed=false;
        chunk.retries++;
        workers.emplace_back(&Downloader::downloadChunk,this,url,file,ref(chunk));
        cout << "Retried..." << endl;
    }

    for(auto& t:workers){
        t.join();
    }
}

void downloadChunk(string url,string file,Chunk& chunk){
    try{
        fstream out(file,ios::in | ios::out | ios::binary);
        if(!out){
            throw runtime_error("file creation failed");
        }

        struct downloadOptions options;
        options.range=chunk.range;
        long long st = chunk.range.start;
        HttpClient client;
        client.sendLarge(url,&options,[&](const char*buffer,int size){
            writer(buffer,size,st,out);
            st+=size;
            chunk.downloaded+=size;
        });

        out.close();
    }
    catch (const exception& e){
        cerr << e.what() << endl;
        chunk.failed=true;
    }
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

        vector<Chunk> chunks = createChunks(totalSize,threads);

        vector<thread> workers;
        for(auto& chunk:chunks){
            workers.emplace_back(&Downloader::downloadChunk,this,url,file,ref(chunk));
        }

        for(auto& t:workers){
            t.join();
        }

        bool failed=false;
        vector<reference_wrapper<Chunk>> failedChunks;

        do{
            failedChunks.clear();
            failed=false;
            for(auto& chunk:chunks){
                if(chunk.failed){
                    if(chunk.retries>MAXRETRIES){
                        throw runtime_error("Cannot Download file\n");
                    }
                    failed=true;
                    failedChunks.push_back(chunk);
                }
            }

            if(failed){
                retryChunk(url,file,failedChunks);
            }
        }while(failed);

        auto end=chrono::high_resolution_clock::now();
        double seconds = chrono::duration<double>(end - start).count();
        cout << totalSize/(1024*1024.0) << "megabytes downloaded in " << seconds << " seconds." << endl;

    }
};