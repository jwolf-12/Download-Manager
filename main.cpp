#include "downloader.hpp"
#include <thread>
#include <chrono>

int main(){

    Downloader downloader;

    thread downloadThread(&Downloader::download, &downloader,
        "https://speedtest.tele2.net/50MB.zip", "test.zip", 8);

    this_thread::sleep_for(chrono::milliseconds(6000));
    downloader.pause();
    cout << '\r' << "Paused" << "                                           " << endl;

    if(downloadThread.joinable()) downloadThread.join(); 

    this_thread::sleep_for(chrono::milliseconds(4000));

    downloader.resume("https://speedtest.tele2.net/50MB.zip", "test.zip", 8); 

    return 0;
}

// https://speedtest.tele2.net/100MB.zip