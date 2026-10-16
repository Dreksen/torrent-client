#include "tcp_connect.h"
#include "byte_tools.h"

#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdexcept>
#include <cstring>
#include <iostream>
#include <chrono>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/poll.h>
#include <limits>
#include <utility>

const std::string &TcpConnect::GetIp() const {
    return ip_;
}

int TcpConnect::GetPort() const {
    return port_;
}

TcpConnect::TcpConnect(std::string ip, int port, std::chrono::milliseconds connectTimeout, std::chrono::milliseconds readTimeout) :
    ip_(ip), port_(port), sock_(-1), connectTimeout_(connectTimeout), readTimeout_(readTimeout) {}

TcpConnect::~TcpConnect() {
    CloseConnection();
}

void TcpConnect::EstablishConnection() {
    sock_ = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_ < 0) {
        throw std::runtime_error("Failed to create socket: " + std::string(strerror(errno)));
    }

    struct sockaddr_in server_addr;
    std::memset(&server_addr, 0, sizeof(server_addr)); 
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port_);

    if (inet_pton(AF_INET, ip_.c_str(), &server_addr.sin_addr) <= 0) {
        CloseConnection(); 
        throw std::runtime_error("Invalid IP address: " + ip_);
    }

    int flags = fcntl(sock_, F_GETFL, 0);
    if (flags < 0) {
        CloseConnection();
        throw std::runtime_error("fcntl F_GETFL failed");
    }
    if (fcntl(sock_, F_SETFL, flags | O_NONBLOCK) < 0) {
        CloseConnection();
        throw std::runtime_error("fcntl F_SETFL (O_NONBLOCK) failed");
    }

    int res = connect(sock_, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if (res < 0) {
        if (errno != EINPROGRESS) { 
            CloseConnection();
            throw std::runtime_error("Connect failed: " + std::string(strerror(errno)));
        }
    }

    struct pollfd pfd;
    pfd.fd = sock_;
    pfd.events = POLLOUT;

    int timeout_ms = std::chrono::duration_cast<std::chrono::milliseconds>(connectTimeout_).count();

    res = poll(&pfd, 1, timeout_ms);

    if (res == 0) {
        CloseConnection();
        throw std::runtime_error("Connection timeout");
    }
    
    if (res < 0) {
        CloseConnection();
        throw std::runtime_error("poll failed: " + std::string(strerror(errno)));
    }

    int so_error = 0;
    socklen_t len = sizeof(so_error);
    if (getsockopt(sock_, SOL_SOCKET, SO_ERROR, &so_error, &len) < 0) {
        CloseConnection();
        throw std::runtime_error("getsockopt failed: " + std::string(strerror(errno)));
    }
    
    if (so_error != 0) {
        CloseConnection();
        throw std::runtime_error("Connection failed/refused: " + std::string(strerror(so_error)));
    }

    if (fcntl(sock_, F_SETFL, flags) < 0) {
        CloseConnection();
        throw std::runtime_error("fcntl F_SETFL (restore) failed");
    }
}

void TcpConnect::SendData(const std::string& data) const {
    size_t total_sent = 0;
    size_t bytes_left = data.size();

    while (total_sent < bytes_left) {
        ssize_t sent = send(sock_, data.data() + total_sent, bytes_left - total_sent, MSG_NOSIGNAL);

        if (sent < 0) {
            throw std::runtime_error("Failed to send data: " + std::string(strerror(errno)));
        }
        total_sent += sent;
    }
}

std::string TcpConnect::ReceiveData(size_t bufferSize) const {
    auto read_exactly = [this](size_t amount) -> std::string {
        if (amount == 0) return "";

        std::string buffer(amount, '\0'); 
        size_t total_read = 0;
        int timeout_ms = std::chrono::duration_cast<std::chrono::milliseconds>(readTimeout_).count();

        while (total_read < amount) {
            struct pollfd pfd;
            pfd.fd = sock_;
            pfd.events = POLLIN;

            int res = poll(&pfd, 1, timeout_ms);
            
            if (res == 0) {
                throw std::runtime_error("Receive timeout");
            }
            if (res < 0) {
                throw std::runtime_error("poll failed during receive: " + std::string(strerror(errno)));
            }

            ssize_t bytes_received = recv(sock_, &buffer[total_read], amount - total_read, 0);
            
            if (bytes_received == 0) {
                throw std::runtime_error("Connection closed by peer");
            }
            if (bytes_received < 0) {
                throw std::runtime_error("recv failed: " + std::string(strerror(errno)));
            }

            total_read += bytes_received;
        }

        return buffer;
    };

    if (bufferSize > 0) 
        return read_exactly(bufferSize);
    std::string len = read_exactly(4);
    
    int intLen = BytesToInt(len);

    if (intLen == 0) {
        return len;
    }

    std::string payload = read_exactly(intLen);

    return len + payload;
}

void TcpConnect::CloseConnection() {
    if (sock_ != -1) {
        close(sock_);
        sock_ = -1;
    }
}
