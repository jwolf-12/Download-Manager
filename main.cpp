#include "downloader.hpp"
int main(){

    Downloader downloader;
    downloader.download("http://localhost:8000/test.txt",
    "test.txt");

    return 0;
}

// https://speedtest.tele2.net/100MB.zip