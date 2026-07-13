#include "downloader.hpp"
int main(){

    Downloader downloader;
    downloader.download("https://speedtest.tele2.net/500MB.zip",
    "test.zip");

    return 0;
}

// https://speedtest.tele2.net/100MB.zip