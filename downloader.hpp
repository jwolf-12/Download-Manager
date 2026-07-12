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
    mutex metaMutex;
    atomic<bool> paused=false;

void saveMeta(const string& filename, vector<Chunk>& chunks){
    lock_guard<mutex> lock(metaMutex);
    ofstream out(filename);
    for(auto& chunk : chunks)
    {
        out << chunk.range.start << " "
            << chunk.range.end << " "
            << chunk.downloaded << " "
            << chunk.completed << "\n";
    }
}

vector<Chunk> loadMeta(const string& filename){
    vector<Chunk> chunks;
    ifstream in(filename);

    long long start, end, downloaded;
    bool completed;
    while(in >> start >> end >> downloaded >> completed)
    {
        Chunk chunk({Range({start,end})});

        chunk.downloaded = downloaded;
        chunk.completed = completed;

        chunks.push_back(chunk);
    }
    return chunks;
}

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

void downloadChunk(string url,string file,Chunk& chunk,bool partial,vector<Chunk>& chunks){
    while(!paused){
        try{
            fstream out(file,ios::in | ios::out | ios::binary);
            if(!out){
                throw runtime_error("file creation failed");
            }
            struct downloadOptions options;
            options.range=chunk.range;
            options.range->start+=chunk.downloaded;
            options.expectPartial=partial;
            long long st = options.range->start;
            HttpClient client;
            client.sendLarge(url,&options,[&](const char*buffer,int size){
                writer(buffer,size,st,out);
                st+=size;
                lock_guard<mutex> lock(metaMutex);
                chunk.downloaded+=size;
            });

            out.close();
            chunk.completed=true;
            saveMeta("file.meta", chunks);
            return;
        }
        catch (const exception& e){
            cerr << e.what() << endl;
            chunk.retries++;
            if(chunk.retries>MAXRETRIES){
                chunk.failed=true;
                return;
            }
            this_thread::sleep_for(chrono::milliseconds(200 * chunk.retries));
        }
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

        int threads=16;

        if(!client.supportsRange(url)){
            threads=1;
        }

        totalSize=client.getSize(url);
        if(totalSize==-1){
            throw runtime_error("failed to get size");
        }

        vector<Chunk> chunks;
        ifstream in("file.meta");

        if(in){
            chunks=loadMeta("file.meta");
        }
        else {
            chunks=createChunks(totalSize,threads);
            out.seekp(totalSize - 1);
            out.write("", 1);
            out.flush();
        }
        out.close();

        vector<thread> workers;
        for(auto& chunk:chunks){
            bytesRecv+=chunk.downloaded;
            if(chunk.completed) continue;
            workers.emplace_back(&Downloader::downloadChunk,this,url,file,ref(chunk),threads!=1,ref(chunks));
        }

        for(auto& t:workers){
            t.join();
        }

        for(auto& chunk:chunks){
            if(chunk.failed){
                throw runtime_error("One or more chunks failed to be retrieved");
            }
        }

        for(auto& c: chunks)
        cout << c.completed << endl;

        // bool failed=false;
        // vector<reference_wrapper<Chunk>> failedChunks;

        // do{
        //     failedChunks.clear();
        //     failed=false;
        //     for(auto& chunk:chunks){
        //         if(chunk.failed){
        //             if(chunk.retries>MAXRETRIES){
        //                 throw runtime_error("Cannot Download file\n");
        //             }
        //             failed=true;
        //             failedChunks.push_back(chunk);
        //         }
        //     }

        //     if(failed){
        //         retryChunk(url,file,failedChunks);
        //     }
        // }while(failed);

        auto end=chrono::high_resolution_clock::now();
        double seconds = chrono::duration<double>(end - start).count();
        cout << totalSize/(1024*1024.0) << "megabytes downloaded in " << seconds << " seconds." << endl;

    }

    void pause{
        paused=true;
    }
};