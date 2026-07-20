#pragma once

#include "downloader.hpp"
#include <map>
#include <memory>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <string>

class DownloadManager {
private:
    map<pair<string,string>, unique_ptr<Downloader>> jobs;
    vector<thread> workers;
    mutex sharedPrintMutex;  
    atomic<int> nextId = 0;
    int maxConcurrent;

public:
    DownloadManager(int maxConcurrent = 3) : maxConcurrent(maxConcurrent) {}
    void addDownload(string url, string file) {
        auto key = make_pair(url, file);

        auto downloader = make_unique<Downloader>();
        downloader->id = nextId.fetch_add(1);
        downloader->printMutex = &sharedPrintMutex;

        cout << "\n";  

        Downloader* rawPtr = downloader.get();
        jobs[key] = move(downloader);

        workers.emplace_back(&Downloader::download, rawPtr, url, file);
    }

    void pauseDownload(string url, string file) {
        auto key = make_pair(url, file);
        auto it = jobs.find(key);
        if (it != jobs.end()) {
            it->second->pause();
        }
    }

    void resumeDownload(string url, string file) {
        auto key = make_pair(url, file);
        auto it = jobs.find(key);
        if (it != jobs.end()) {
            workers.emplace_back(&Downloader::resume, it->second.get(), url, file);
        }
    }

    void waitAll() {
        for (auto& t : workers) {
            if (t.joinable()) t.join();
        }
    }
};