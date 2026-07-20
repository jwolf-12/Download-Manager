#include "downloader.hpp"
#include <iostream>
#include <string>
#include <mutex>

int main(int argc, char* argv[]){
    if (argc < 4) {
        cerr << "Usage: " << argv[0] << " <url> <output_file> <threadcnt>" << endl;
        return 1;
    }

    string url = argv[1];
    string file = argv[2];
    int threadcnt = stoi(argv[3]);

    mutex printMutex;

    Downloader downloader;
    downloader.printMutex = &printMutex;
    downloader.id = 0;

    downloader.download(url, file, threadcnt);

    return 0;
}