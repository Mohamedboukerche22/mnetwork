#pragma once

// #include<bits/stdc++.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cstring>
#include <vector>
#include <map>
#include <thread>
#include <atomic>
#include <memory>
#include <functional>
#include <regex>
#include <chrono>

#ifdef _WIN32
 // work for windows also ig 
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    #define close closesocket
#else
    #include <unistd.h>
    #include <arpa/inet.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <sys/select.h>
#endif

using namespace std;
namespace mnetwork {

struct ServerConfig {
    int port = 8080;
    int max_connections = 10;
    int buffer_size = 4096;
    int thread_pool_size = 4;
    string host = "0.0.0.0";
    bool verbose = false;
};


struct HttpRequest {
    string method;
    string path;
    string version;
    map<string, string> headers;
    string body;
    map<string, string> query_params;
    
    string get_header(const string& key, const string& default_val = "") const {
        auto it = headers.find(key);
        return it != headers.end() ? it->second : default_val;
    }
};

// HTTP Response structure
struct HttpResponse {
    int status_code = 200;
    string status_text = "OK";
    map<string, string> headers;
    string body;
    
    void set_header(const string& key, const string& value) {
        headers[key] = value;
    }
    
    string to_string() const {
        ostringstream oss;
        oss << "HTTP/1.1 " << status_code << " " << status_text << "\r\n";
    
        auto temp_headers = headers;
        if (temp_headers.find("Content-Type") == temp_headers.end()) {
            temp_headers["Content-Type"] = "text/html";
        }
        if (temp_headers.find("Content-Length") == temp_headers.end()) {
            temp_headers["Content-Length"] = to_string(body.size());
        }
        
        for (const auto& [key, value] : temp_headers) {
            oss << key << ": " << value << "\r\n";
        }
        oss << "\r\n" << body;
        return oss.str();
    }
};

using Middleware = function<bool(const HttpRequest&, HttpResponse&)>;
using RouteHandler = function<void(const HttpRequest&, HttpResponse&)>;

class HttpServer {
private:
    ServerConfig config_;
    int server_fd_;
    atomic<bool> running_{false};
    vector<thread> worker_threads_;
    map<string, RouteHandler> routes_;
    vector<Middleware> middlewares_;
    
    void log(const string& message) const {
        if (config_.verbose) {
            lock_guard<mutex> lock(log_mutex_);
            auto now = chrono::system_clock::now();
            auto time = chrono::system_clock::to_time_t(now);
            cout << "[" << put_time(localtime(&time), "%H:%M:%S") 
                      << "] " << message << endl;
        }
    }
    
    mutable mutex log_mutex_;
    
    void initialize_sockets() {
#ifdef _WIN32
        WSADATA wsa_data;
        if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
            throw runtime_error("WSAStartup failed");
        }
#endif
    }
    
    void cleanup_sockets() {
#ifdef _WIN32
        WSACleanup();
#endif
    }
    
    HttpRequest parse_request(const string& request_str) {
        HttpRequest req;
        istringstream stream(request_str);
        string line;
    
        if (getline(stream, line)) {
            istringstream line_stream(line);
            line_stream >> req.method >> req.path >> req.version;

            size_t qmark = req.path.find('?');
            if (qmark != string::npos) {
                string query = req.path.substr(qmark + 1);
                req.path = req.path.substr(0, qmark);
                
                istringstream query_stream(query);
                string pair;
                while (getline(query_stream, pair, '&')) {
                    size_t equals = pair.find('=');
                    if (equals != string::npos) {
                        string key = pair.substr(0, equals);
                        string value = pair.substr(equals + 1);
                        req.query_params[key] = value;
                    }
                }
            }
        }
        
        while (getline(stream, line) && line != "\r") {
            size_t colon = line.find(':');
            if (colon != string::npos) {
                string key = line.substr(0, colon);
                string value = line.substr(colon + 2); // Skip ": "
                if (!value.empty() && value.back() == '\r') {
                    value.pop_back();
                }
                req.headers[key] = value;
            }
        }
        
        ostringstream body_stream;
        body_stream << stream.rdbuf();
        req.body = body_stream.str();
        
        return req;
    }
    
    void handle_connection(int client_fd) {
        char buffer[config_.buffer_size];
        ssize_t bytes_received = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
        
        if (bytes_received > 0) {
            buffer[bytes_received] = '\0';
            
            try {
                HttpRequest request = parse_request(buffer);
                HttpResponse response;
                
                log(request.method + " " + request.path);
                
                bool continue_processing = true;
                for (const auto& middleware : middlewares_) {
                    if (!middleware(request, response)) {
                        continue_processing = false;
                        break;
                    }
                }
                
                if (continue_processing) {
                    auto it = routes_.find(request.path);
                    if (it != routes_.end()) {
                        it->second(request, response);
                    } else {
                        bool found = false;
                        for (const auto& [pattern, handler] : routes_) {
                            if (pattern.find('*') != string::npos ||
                                pattern.find(':') != string::npos) {
                                if (request.path.find(pattern.substr(0, pattern.find('*'))) == 0) {
                                    handler(request, response);
                                    found = true;
                                    break;
                                }
                            }
                        }
                        
                        if (!found) {
                            response.status_code = 404;
                            response.status_text = "Not Found";
                            response.body = "<h1>404 Not Found</h1>";
                        }
                    }
                }
                
                string response_str = response.to_string();
                send(client_fd, response_str.c_str(), response_str.size(), 0);
                
            } catch (const exception& e) {
                log("Error processing request: " + string(e.what()));
                
                HttpResponse error_response;
                error_response.status_code = 500;
                error_response.status_text = "Internal Server Error";
                error_response.body = "<h1>500 Internal Server Error</h1>";
                string error_str = error_response.to_string();
                send(client_fd, error_str.c_str(), error_str.size(), 0);
            }
        }
        
        close(client_fd);
    }
    
    void worker_thread() {
        while (running_) {
            fd_set read_fds;
            FD_ZERO(&read_fds);
            FD_SET(server_fd_, &read_fds);
            
            timeval timeout{0, 100000}; // 100ms timeout
            
            int activity = select(server_fd_ + 1, &read_fds, nullptr, nullptr, &timeout);
            
            if (activity > 0 && FD_ISSET(server_fd_, &read_fds)) {
                struct sockaddr_in client_addr;
                socklen_t client_len = sizeof(client_addr);
                
                int client_fd = accept(server_fd_, (struct sockaddr*)&client_addr, &client_len);
                
                if (client_fd >= 0) {
                    handle_connection(client_fd);
                }
            }
        }
    }
    
public:
    HttpServer(const ServerConfig& config = ServerConfig()) : config_(config), server_fd_(-1) {
        initialize_sockets();
    }
    
    ~HttpServer() {
        stop();
        cleanup_sockets();
    }
    HttpServer& route(const string& path, RouteHandler handler) {
        routes_[path] = handler;
        return *this;
    }
    HttpServer& use(Middleware middleware) {
        middlewares_.push_back(middleware);
        return *this;
    }
    
    // Static file serving middleware
    void static_files(const string& route, const string& directory) {
        route(route + "/*", [directory](const HttpRequest& req, HttpResponse& res) {
            string filepath = directory + req.path.substr(route.length());
            
            ifstream file(filepath, ios::binary);
            if (file) {
                ostringstream content;
                content << file.rdbuf();
                
                res.body = content.str();
                
                size_t dot = filepath.find_last_of('.');
                if (dot != string::npos) {
                    string ext = filepath.substr(dot);
                    static const map<string, string> mime_types = {
                        {".html", "text/html"},
                        {".css", "text/css"},
                        {".js", "application/javascript"},
                        {".json", "application/json"},
                        {".png", "image/png"},
                        {".jpg", "image/jpeg"},
                        {".jpeg", "image/jpeg"},
                        {".gif", "image/gif"},
                        {".txt", "text/plain"}
                    };
                    
                    auto it = mime_types.find(ext);
                    if (it != mime_types.end()) {
                        res.set_header("Content-Type", it->second);
                    }
                }
            } else {
                res.status_code = 404;
                res.body = "File not found";
            }
        });
    }
    
    // Start the server
    bool start() {
        server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd_ < 0) {
            log("Failed to create socket");
            return false;
        }
        
        // Enable port reuse
        int opt = 1;
#ifdef SO_REUSEADDR
        setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
#endif
        
        struct sockaddr_in address{};
        address.sin_family = AF_INET;
        inet_pton(AF_INET, config_.host.c_str(), &address.sin_addr);
        address.sin_port = htons(config_.port);
        
        if (bind(server_fd_, (struct sockaddr*)&address, sizeof(address)) < 0) {
            log("Bind failed");
            close(server_fd_);
            return false;
        }
        
        if (listen(server_fd_, config_.max_connections) < 0) {
            log("Listen failed");
            close(server_fd_);
            return false;
        }
        
        running_ = true;
        log("Server started on http://" + config_.host + ":" + to_string(config_.port));
        
        for (int i = 0; i < config_.thread_pool_size; ++i) {
            worker_threads_.emplace_back(&HttpServer::worker_thread, this);
        }
        
        return true;
    }
    
    void stop() {
        running_ = false;
        
        for (auto& thread : worker_threads_) {
            if (thread.joinable()) {
                thread.join();
            }
        }
        
        if (server_fd_ >= 0) {
            close(server_fd_);
            server_fd_ = -1;
        }
        
        log("Server stopped");
    }

    void run() {
        if (start()) {
            cout << "Press Enter to stop the server..." << endl;
            cin.get();
            stop();
        }
    }
};

void host(const string& html, int port = 8080) {
    ServerConfig config;
    config.port = port;
    config.verbose = true;
    
    HttpServer server(config);
    server.route("/", [html](const HttpRequest&, HttpResponse& res) {
        res.body = html;
    });
    
    server.run();
}

void host_file(const string& filename, int port = 8080) {
    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "Failed to open file: " << filename << endl;
        return;
    }
    
    ostringstream content;
    content << file.rdbuf();
    file.close();
    
    host(content.str(), port);
}

class HttpClient {
private:
    string host_;
    int port_;
    
public:
    HttpClient(const string& host = "localhost", int port = 80) 
        : host_(host), port_(port) {}
    
    HttpResponse get(const string& path, 
                    const map<string, string>& headers = {}) {
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) {
            throw runtime_error("Failed to create socket");
        }
        
        struct sockaddr_in server_addr{};
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port_);
        
        if (inet_pton(AF_INET, host_.c_str(), &server_addr.sin_addr) <= 0) {
            close(sock);
            throw runtime_error("Invalid address");
        }
        
        if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            close(sock);
            throw runtime_error("Connection failed");
        }
        

        ostringstream request;
        request << "GET " << path << " HTTP/1.1\r\n";
        request << "Host: " << host_ << "\r\n";
        for (const auto& [key, value] : headers) {
            request << key << ": " << value << "\r\n";
        }
        request << "Connection: close\r\n\r\n";
        

        string request_str = request.str();
        send(sock, request_str.c_str(), request_str.size(), 0);
        

        string response_str;
        char buffer[4096];
        ssize_t bytes_received;
        
        while ((bytes_received = recv(sock, buffer, sizeof(buffer) - 1, 0)) > 0) {
            buffer[bytes_received] = '\0';
            response_str.append(buffer, bytes_received);
        }
        
        close(sock);
        
        HttpResponse response;
        istringstream response_stream(response_str);
        string line;
        
        if (getline(response_stream, line)) {
            istringstream status_stream(line);
            string http_version;
            status_stream >> http_version >> response.status_code;
            getline(status_stream, response.status_text);
            
            if (!response.status_text.empty() && response.status_text[0] == ' ') {
                response.status_text = response.status_text.substr(1);
            }
        }
        
        while (getline(response_stream, line) && line != "\r") {
            size_t colon = line.find(':');
            if (colon != string::npos) {
                string key = line.substr(0, colon);
                string value = line.substr(colon + 2);
                if (!value.empty() && value.back() == '\r') {
                    value.pop_back();
                }
                response.headers[key] = value;
            }
        }
        
        ostringstream body_stream;
        body_stream << response_stream.rdbuf();
        response.body = body_stream.str();
        
        return response;
    }
};

} 
