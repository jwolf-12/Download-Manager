#include <iostream>
#include <fstream>
using namespace std;

#include "httpclient.hpp"
#include "request.hpp"
#include "response.hpp"

int main(){

    Request req;

    req.method = "GET";
    req.path = "/test.png";
    req.headers["Host"] = "localhost";
    req.headers["Connection"] = "close";
    req.version = "HTTP/1.1";

    HttpClient client;

    Response res = client.send(
        "localhost",
        "8000",
        req
    );

    ofstream out("downloaded.png",ios::binary);

    out.write(res.body.data(),res.body.size());

    return 0;
}