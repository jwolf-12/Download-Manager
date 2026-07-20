#include "idm.hpp"
#include <thread>
#include <chrono>

int main(){

    DownloadManager manager(3);

    manager.addDownload("https://speedtest.tele2.net/10MB.zip", "test1.zip");
    manager.addDownload("https://speedtest.tele2.net/10MB.zip", "test2.zip");

    manager.waitAll();

    return 0;
}

// https://speedtest.tele2.net/100MB.zip