#pragma once

#include <fstream>
#include <sstream>
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>


namespace mnetwork
{
    void host(const char* html, int port)
    {
        int server_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd < 0)
        {
            perror("socket");
            return;
        }

        sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(port);

        if (bind(server_fd, (sockaddr*)&address, sizeof(address)) < 0)
        {
            perror("bind");
            close(server_fd);
            return;
        }

        if (listen(server_fd, 1) < 0)
        {
            perror("listen");
            close(server_fd);
            return;
        }

        std::cout << "Hosting at http://localhost:" << port << std::endl;

        while(1){
        int client_fd = accept(server_fd, nullptr, nullptr);
        if (client_fd < 0)
        {
            perror("accept");
            close(server_fd);
            return;
        }

        int html_len = strlen(html);

        std::string response =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html\r\n"

            "Content-Length: " + std::to_string(html_len) + "\r\n"
            "\r\n" +
            std::string(html);

        send(client_fd, response.c_str(), response.size(), 0);

        close(client_fd);
        }
        close(server_fd);
    }
    // another function that host file like index.html from your device 

    void hostFile(const char* filename, int port)
    {
        std::ifstream file(filename);
        if (!file.is_open())
        {
            std::cerr << "Failed to open file: " << filename << std::endl;
            return;
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string html = buffer.str();
        file.close();

        int server_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd < 0) { perror("socket"); return; }

        sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(port);

        if (bind(server_fd, (sockaddr*)&address, sizeof(address)) < 0)
        { perror("bind"); close(server_fd); return; }

        if (listen(server_fd, 1) < 0)
        { perror("listen"); close(server_fd); return; }

        std::cout << "Hosting file '" << filename << "' at http://localhost:" << port << std::endl;

        while(1){
        int client_fd = accept(server_fd, nullptr, nullptr);
        if (client_fd < 0) { perror("accept"); close(server_fd); return; }

        std::string response =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html\r\n"
            "Content-Length: " + std::to_string(html.size()) + "\r\n"
            "\r\n" +
            html;

        send(client_fd, response.c_str(), response.size(), 0);
        close(client_fd);
        }
        close(server_fd);
    }
}

