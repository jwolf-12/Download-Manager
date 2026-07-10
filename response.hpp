#pragma once

#include <iostream>
#include <unistd.h>
#include <errno.h>
#include <cstring>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unordered_map>
#include <sstream>
#include <vector>
using namespace std;

class Response{
public:
    string version;
    int statusCode;
    string statusMessage;

    unordered_map<string,string> headers;

    vector<char> body;

    string const getHeader(const string& key){
        return headers.at(key);
    }

    void parseHeaders(string headerPart){
        stringstream ss(headerPart);
        string line;
        bool firstLine=true;
        while(getline(ss,line)){
            if(!line.empty() && line.back()=='\r') line.pop_back();
            
            if(firstLine){
                stringstream first(line);
                first >> version;
                first >> statusCode;
                getline(first,statusMessage);
                if(!statusMessage.empty() && statusMessage[0]==' ') statusMessage.erase(0,1);
                firstLine=false;
            }
            else{
                size_t colon = line.find(':');

                string key = line.substr(0,colon);
                string value = line.substr(colon+1);

                if(!value.empty() && value[0]==' ') value.erase(0,1);

                headers[key]=value;
            }

        }
    }

    void parseResponse(string response){
        size_t pos = response.find("\r\n\r\n");
        string headerPart= response.substr(0,pos);
        string bodyPart = response.substr(pos+4);
        body.assign(bodyPart.begin(),bodyPart.end());

        parseHeaders(headerPart);
    }

    void printResponse(){
        cout << "Version: " << version << '\n';
        cout << "Status Code: " << statusCode << '\n';
        cout << "Status Message: " << statusMessage << '\n';
        for(const auto& p : headers){
            cout << p.first << ": " << p.second << endl;
        }
        cout.write(body.data(),body.size());
        cout << endl;
    }
};
