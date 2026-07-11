#include "downloader.hpp"
int main(){

    Downloader downloader;
    downloader.download("https://speedtest.tele2.net/100MB.zip",
    "test.zip");

    return 0;
}