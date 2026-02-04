
#include "TcpConnection.h"
#include "EventLoop.h"
#include "TcpServer.h"
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

TcpServer::TcpServer(EventLoop &evloop)
    : evloop_(evloop), listen_fd_(-1)
{
}

TcpServer::~TcpServer()
{
    if (listen_fd_ != -1)
    {
        evloop_.DelEvent(listen_fd_);
        close(listen_fd_);
    }
}

void TcpServer::Start(uint16_t port, NewConnCallback cb)
{
    new_conn_cb_ = cb;
    listen_fd_ = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (listen_fd_ == -1)
    {
        std::cerr << "socket error: " << errno << std::endl;
        return;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    int opt = 1;
    if (setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
    {
        std::cerr << "setsockopt error: " << errno << std::endl;
        close(listen_fd_);
        return;
    }
    if (setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)) == -1)
    {
        std::cerr << "setsockopt error: " << errno << std::endl;
        close(listen_fd_);
        return;
    }

    if (::bind(listen_fd_, (sockaddr *)&addr, sizeof(addr)) == -1)
    {
        std::cerr << "bind error: " << errno << std::endl;
        close(listen_fd_);
        return;
    }

    if (::listen(listen_fd_, SOMAXCONN) == -1)
    {
        std::cerr << "listen error: " << errno << std::endl;
        close(listen_fd_);
        return;
    }

    accept_handler_ = [this](){ HandleAccept(); };
    evloop_.AddEvent(listen_fd_, EPOLLIN, &accept_handler_);
    std::cout << "Server started on port " << port << std::endl;
}

void TcpServer::HandleAccept()
{
    sockaddr_in client_addr{};
    socklen_t len = sizeof(client_addr);
    int conn_fd = ::accept4(listen_fd_, (sockaddr *)&client_addr, &len, SOCK_NONBLOCK);
    if (conn_fd == -1)
        return;

    auto conn = std::make_shared<TcpConn>(conn_fd, evloop_);
    if (new_conn_cb_)
        new_conn_cb_(conn);
}
