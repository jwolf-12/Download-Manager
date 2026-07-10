#include "downloader.hpp"
int main(){

    Downloader downloader;
    downloader.download("http://speedtest.tele2.net/3MB.zip","test.zip");

    return 0;
}