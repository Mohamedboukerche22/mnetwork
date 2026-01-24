# mnetwork 

# usage 
# MNetwork Documentation

## Table of Contents
- [Overview](#overview)
- [Installation](#installation)
- [Quick Start](#quick-start)
- [HttpServer Class](#httpserver-class)
  - [Basic Server](#basic-server)
  - [Configuration](#configuration)
  - [Routes](#routes)
  - [Static Files](#static-files)
  - [Middleware](#middleware)
  - [Running the Server](#running-the-server)
- [HttpClient Class](#httpclient-class)
  - [Making Requests](#making-requests)
- [Utility Functions](#utility-functions)
- [Advanced Examples](#advanced-examples)
- [API Reference](#api-reference)
- [Performance Tips](#performance-tips)
- [Troubleshooting](#troubleshooting)

## Overview

MNetwork is a lightweight, high-performance C++ HTTP server and client library designed for modern applications. It provides:
- Full HTTP/1.1 support
- Cross-platform compatibility (Windows/Linux/macOS)
- Multi-threaded request handling
- Easy-to-use routing system
- Middleware support
- Static file serving
- Built-in HTTP client

## Installation

### Requirements
- C++17 or higher
- CMake 3.10+ (recommended)
- GCC 7+, Clang 5+, or MSVC 2017+

### Using CMake
```cmake
# Find or download mnetwork
find_package(mnetwork REQUIRED)

# Link to your target
target_link_libraries(your_target mnetwork)
```

## Manual Integration
Copy `mnetwork.h` to your project

Include it in your source files:

```cpp
#include "mnetwork.hpp"
```

## Quick Start
Minimal Example
```cpp
#include "mnetwork.hpp"

int main() {
    mnetwork::HttpServer server;
    
    server.route("/", [](const mnetwork::HttpRequest& req, mnetwork::HttpResponse& res) {
        res.body = "<h1>Hello, World!</h1>";
    });
    
    server.run();  // Runs on port 8080 by default
    return 0;
}
```

## Hosting a File
```cpp
#include "mnetwork.hpp"

int main() {
    // Host a single HTML string
    mnetwork::host("<h1>Welcome!</h1>", 3000);
    
    // Or host a file directly
    mnetwork::host_file("index.html", 8080);
    
    return 0;
}
```
## HttpServer Class
Basic Server
```cpp
mnetwork::HttpServer server;
server.route("/", handler_function);
server.run();
```
Configuration
Customize server behavior with ServerConfig:

```cpp
mnetwork::ServerConfig config;
config.port = 3000;                     // Port to listen on
config.host = "0.0.0.0";               // Bind address
config.max_connections = 50;           // Maximum concurrent connections
config.buffer_size = 8192;             // Buffer size for requests
config.thread_pool_size = 8;           // Worker threads
config.verbose = true;                 // Enable logging

mnetwork::HttpServer server(config);
```
# Routes
## Basic Routes
```cpp
server.route("/", [](const mnetwork::HttpRequest& req, mnetwork::HttpResponse& res) {
    res.body = "Home Page";
});

server.route("/about", [](const mnetwork::HttpRequest& req, mnetwork::HttpResponse& res) {
    res.body = "<h1>About Us</h1>";
});
```
## JSON Responses
```cpp
server.route("/api/data", [](const mnetwork::HttpRequest& req, mnetwork::HttpResponse& res) {
    res.set_header("Content-Type", "application/json");
    res.body = R"({
        "status": "success",
        "data": [1, 2, 3, 4, 5]
    })";
});
```
## Accessing Request Data
```cpp
server.route("/user", [](const mnetwork::HttpRequest& req, mnetwork::HttpResponse& res) {
    // Request method
    std::cout << "Method: " << req.method << std::endl;
    
    // Headers
    std::string user_agent = req.get_header("User-Agent");
    
    // Query parameters (for /user?id=123)
    std::string user_id = req.query_params["id"];
    
    // Request body
    std::cout << "Body: " << req.body << std::endl;
    
    res.body = "User processed";
});
```
Static Files
Serve entire directories with automatic MIME type detection:

```cpp
// Serve files from ./public at /static/*
server.static_files("/static", "./public");

// Serve entire directory at root
server.static_files("/", "./www");
```
### **Supported file extensions with automatic Content-Type:**

`.html`, `.htm` → text/html

`.css` → text/css

`.js` → application/javascript

`.json` → application/json

`.png` → image/png

`.jpg`, `.jpeg` → image/jpeg

`.gif` → image/gif

`.txt` → text/plain

# Middleware
## dd cross-cutting concerns with middleware:

```cpp
// Logging middleware
server.use([](const mnetwork::HttpRequest& req, mnetwork::HttpResponse& res) {
    std::cout << req.method << " " << req.path << std::endl;
    return true; // Continue to next middleware/route
});

// Authentication middleware
server.use([](const mnetwork::HttpRequest& req, mnetwork::HttpResponse& res) {
    std::string token = req.get_header("Authorization");
    if (token != "secret-token") {
        res.status_code = 401;
        res.body = "Unauthorized";
        return false; // Stop processing
    }
    return true;
});

// CORS middleware
server.use([](const mnetwork::HttpRequest& req, mnetwork::HttpResponse& res) {
    res.set_header("Access-Control-Allow-Origin", "*");
    res.set_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE");
    res.set_header("Access-Control-Allow-Headers", "Content-Type, Authorization");
    return true;
});
```

# Running the Server
## Simple Run (Blocks until Enter is pressed)
```cpp
server.run();  // Automatically calls start() and blocks
```
## Manual Control
```cpp
// Start server
if (server.start()) {
    std::cout << "Server is running..." << std::endl;
    
    // Keep server running
    std::this_thread::sleep_for(std::chrono::hours(1));
    
    // Stop when needed
    server.stop();
}
```
## Non-blocking Operation
```cpp
server.start();

// Your application logic here
do_other_work();

// Stop when done
server.stop();
```
# HttpClient Class
## Making Requests
```cpp
// Create client
mnetwork::HttpClient client("example.com", 80);

// Basic GET request
auto response = client.get("/");

std::cout << "Status: " << response.status_code << std::endl;
std::cout << "Body: " << response.body << std::endl;

// With custom headers
std::map<std::string, std::string> headers = {
    {"User-Agent", "MyClient/1.0"},
    {"Accept", "application/json"}
};

auto response2 = client.get("/api/data", headers);

// Access response headers
for (const auto& [key, value] : response.headers) {
    std::cout << key << ": " << value << std::endl;
}
```
## Utility Functions
## Quick Hosting Functions
```cpp
// Host HTML content directly
mnetwork::host(R"(
    <!DOCTYPE html>
    <html>
    <head><title>Test</title></head>
    <body><h1>Hello from MNetwork!</h1></body>
    </html>
)", 8080);

// Host a file
mnetwork::host_file("index.html", 3000);

// Host with custom port
mnetwork::host("<h1>Custom Port</h1>", 5000);
```
# Request/Response Objects
## HttpRequest
```cpp
struct mnetwork::HttpRequest {
    std::string method;              // "GET", "POST", etc.
    std::string path;                // "/api/users"
    std::string version;             // "HTTP/1.1"
    std::map<std::string, std::string> headers;
    std::string body;
    std::map<std::string, std::string> query_params;
    
    // Helper methods
    std::string get_header(const std::string& key, 
                           const std::string& default_val = "") const;
};
```
## HttpResponse
```cpp
struct mnetwork::HttpResponse {
    int status_code = 200;
    std::string status_text = "OK";
    std::map<std::string, std::string> headers;
    std::string body;
    
    // Helper methods
    void set_header(const std::string& key, const std::string& value);
    std::string to_string() const;
};
```
# Advanced Examples
## REST API Server
```cpp
#include "mnetwork.hpp"
#include <vector>
#include <map>

struct User {
    int id;
    std::string name;
    std::string email;
};

std::map<int, User> users;
int next_id = 1;

int main() {
    mnetwork::HttpServer server;
    
    // GET all users
    server.route("/api/users", [](const mnetwork::HttpRequest& req, 
                                  mnetwork::HttpResponse& res) {
        std::string json = "[";
        bool first = true;
        for (const auto& [id, user] : users) {
            if (!first) json += ",";
            json += R"({"id":)" + std::to_string(user.id) + 
                   R"(,"name":")" + user.name + 
                   R"(","email":")" + user.email + "\"}";
            first = false;
        }
        json += "]";
        
        res.set_header("Content-Type", "application/json");
        res.body = json;
    });
    
    // GET single user
    server.route("/api/users/:id", [](const mnetwork::HttpRequest& req, 
                                      mnetwork::HttpResponse& res) {
        int id = std::stoi(req.path.substr(12)); // Extract from /api/users/123
        auto it = users.find(id);
        
        if (it != users.end()) {
            res.set_header("Content-Type", "application/json");
            res.body = R"({"id":)" + std::to_string(it->second.id) + 
                      R"(,"name":")" + it->second.name + 
                      R"(","email":")" + it->second.email + "\"}";
        } else {
            res.status_code = 404;
            res.body = R"({"error": "User not found"})";
        }
    });
    
    // POST create user
    server.route("/api/users", [](const mnetwork::HttpRequest& req, 
                                  mnetwork::HttpResponse& res) {
        // Parse JSON from body (simplified)
        // In production, use a JSON library
        User user;
        user.id = next_id++;
        user.name = "New User"; // Parse from req.body
        user.email = "user@example.com";
        
        users[user.id] = user;
        
        res.status_code = 201; // Created
        res.set_header("Content-Type", "application/json");
        res.body = R"({"id":)" + std::to_string(user.id) + 
                  R"(,"message": "User created"})";
    });
    
    server.run();
    return 0;
}
```
## File Upload Server
```cpp
#include "mnetwork.hpp"
#include <fstream>

int main() {
    mnetwork::HttpServer server;
    
    // Upload form
    server.route("/upload", [](const mnetwork::HttpRequest& req, 
                               mnetwork::HttpResponse& res) {
        res.body = R"(
            <!DOCTYPE html>
            <html>
            <body>
                <form action="/upload" method="post" enctype="multipart/form-data">
                    <input type="file" name="file">
                    <input type="submit" value="Upload">
                </form>
            </body>
            </html>
        )";
    });
    
    // Handle upload
    server.route("/upload", [](const mnetwork::HttpRequest& req, 
                               mnetwork::HttpResponse& res) {
        if (req.method == "POST") {
            // Simple multipart parsing (for demonstration)
            // Extract filename and content from req.body
            std::string filename = "uploaded_" + std::to_string(time(nullptr)) + ".dat";
            
            std::ofstream file(filename, std::ios::binary);
            file.write(req.body.data(), req.body.size());
            file.close();
            
            res.body = "<h1>File uploaded: " + filename + "</h1>";
        }
    });
    
    server.run();
    return 0;
}
```

## Webhook Receiver
```cpp
#include "mnetwork.hpp"
#include <iostream>

int main() {
    mnetwork::HttpServer server;
    
    // GitHub webhook endpoint
    server.route("/webhook/github", [](const mnetwork::HttpRequest& req, 
                                       mnetwork::HttpResponse& res) {
        std::string event = req.get_header("X-GitHub-Event");
        std::string signature = req.get_header("X-Hub-Signature-256");
        
        std::cout << "GitHub Event: " << event << std::endl;
        std::cout << "Payload: " << req.body << std::endl;
        
        // Verify signature and process event here
        
        res.body = R"({"status": "received"})";
        res.set_header("Content-Type", "application/json");
    });
    
    server.run();
    return 0;
}
```


