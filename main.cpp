#include "downloader.hpp"
int main(){

    Downloader downloader;
    downloader.download("http://localhost/test.txt",
    "test.txt");

    return 0;
}