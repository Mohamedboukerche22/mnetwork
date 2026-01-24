#include "mnetwork.hpp"
using namespace mnetwork;

int main() {
    HttpServer server;
    server.route("/", [](const HttpRequest& req, HttpResponse& res) {
        res.body = "<h1>Hello, World!</h1>";
    });
    
    server.run();  
    return 0;
}
