#pragma once

#include "downloadoptions.hpp"
#include <fstream>
#include "httpclient.hpp"
#include <chrono>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>
#include <filesystem>

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

void printProgress(double percent, double mbs){
    const int width=40;
    int filled = static_cast<int>(width*percent/100);

    string bar;
    for(int i=0;i<filled;i++){
        bar+="\u2588";
    }
    for(int i=filled;i<width;i++){
        bar+="\u2591";
    }

    ostringstream line;
    line << bar << " Speed: " << fixed << setprecision(2) << mbs << " mbs" << "            ";

    lock_guard<mutex> lock(*printMutex); 

    cout << "\033[" << (id+1) << "A"
         << "\r" << line.str()
         << "\033[" << (id+1) << "B" 
         << "\r" << flush;
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
        // cout << bytesRecv*100.0/totalSize <<"% done  Speed: " << bytesRecv/(seconds*(1024.0*1024)) << " mbs" <<endl;
        printProgress(bytesRecv*100.0/totalSize,bytesRecv/(seconds*(1024.0*1024)));
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
        thread watchdog;
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
            atomic<long long> lastProgressMs{
                chrono::duration_cast<chrono::milliseconds>(
                    chrono::high_resolution_clock::now().time_since_epoch()
                ).count()
            };

            atomic<bool> done=false;
            atomic<bool> stalled=false;
            watchdog=thread([&](){
                while(!done){
                    auto now = chrono::duration_cast<chrono::milliseconds>(
                            chrono::high_resolution_clock::now().time_since_epoch()
                        ).count();
                    
                    if(now - lastProgressMs > 5000 && done==false){
                        stalled=true; return;
                    }
                    
                    this_thread::sleep_for(chrono::milliseconds(500));
                }
            });
            client.sendLarge(url,&options,[&](const char*buffer,int size){
                if(stalled) throw runtime_error("Stalled");
                if(paused) throw runtime_error("Paused");
                writer(buffer,size,st,out);
                st+=size;
                lock_guard<mutex> lock(metaMutex);
                chunk.downloaded+=size;
                lastProgressMs =
                chrono::duration_cast<chrono::milliseconds>(
                    chrono::high_resolution_clock::now().time_since_epoch()
                ).count();
            });
            done=true;
            out.close();
            chunk.completed=true;
            if(watchdog.joinable()) watchdog.join();
            saveMeta(file + ".meta", chunks);
            return;
        }
        catch (const exception& e){
            if(watchdog.joinable()) watchdog.join();
            // cerr << e.what() << endl;
            if(paused) {saveMeta(file + ".meta", chunks); return;}
            chunk.retries++;
            if(chunk.retries>MAXRETRIES){
                chunk.failed=true;
                return;
            }
            this_thread::sleep_for(chrono::milliseconds(200 * chunk.retries));
        }
    }
}

int chooseThreadCount(long long size){
    if(size < 100 * 1024) return 1;
    if(size < 5 * 1024 * 1024) return 8;
    if(size < 100 * 1024 * 1024) return 16;
    return 64;
}

void downloadmain(string url,string file,bool resume){

    fstream out;
    if(resume){
        out.open(file, ios::in | ios::out | ios::binary);
    }
    else{
        out.open(file, ios::out | ios::binary);
    }
    if(!out){
        throw runtime_error("file creation failed");
    }

    start=chrono::high_resolution_clock::now();
    last=start;

    bytesRecv=0;

    totalSize=client.getSize(url);
    if(totalSize==-1){
        throw runtime_error("failed to get size");
    }

    int threads=16;

    if(!client.supportsRange(url)){
        threads=1;
    }

    vector<Chunk> chunks;
    string metaFileName=file+".meta";
    ifstream in(metaFileName);

    if(in){
        chunks=loadMeta(metaFileName);
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

    auto end=chrono::high_resolution_clock::now();
    double seconds = chrono::duration<double>(end - start).count();
    cout << '\r' << bytesRecv/(1024*1024.0) << "megabytes downloaded in " << seconds << " seconds." << "                                       " << endl;

    if(!paused) filesystem::remove(metaFileName);

}

public:

    int id;
    mutex* printMutex=nullptr;

    void pause(){
        paused=true;
    }

    void resume(string url,string file){
        paused=false;
        downloadmain(url,file,1);
    }

    void download(string url,string file){
        downloadmain(url,file,0);
    }
};