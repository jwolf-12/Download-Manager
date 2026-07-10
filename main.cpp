#include "downloader.hpp"
int main(){

    Downloader downloader;
    downloader.download("http://localhost:8000/test500mb.bin","check.bin");

    return 0;
}