#include "downloader.hpp"
int main(){

    Downloader downloader;
    downloader.download("http://speedtest.tele2.net/1MB.zip",
    "test.zip");

    return 0;
}