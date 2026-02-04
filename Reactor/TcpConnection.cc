#include "TcpConnection.h"
#include "EventLoop.h"
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

TcpConn::TcpConn(int fd, EventLoop &evloop)
    : fd_(fd), evloop_(evloop), closed_(false)
{
    SetNonBlocking(fd_);
    io_handler_ = [this, fd](){ HandleIO(fd); };
    evloop_.AddEvent(fd, EPOLLIN | EPOLLRDHUP, &io_handler_);
}

TcpConn::~TcpConn()
{
    Close();
}

int TcpConn::Send(const char *data, size_t size)
{
    if (closed_ || data == nullptr || size == 0)
        return -1;

    if (!output_buffer_.empty())
    {
        output_buffer_.append(data, size);
        EnableWrite();
        return size;
    }

    int n = ::send(fd_, data, size, MSG_NOSIGNAL);
    if (n < 0)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            output_buffer_.append(data, size);
            EnableWrite();
        }
        else
        {
            Close();
        }
    }
    else
    {
        if (n < size)
        {
            output_buffer_.append(data + n, size - n);
            EnableWrite();
        }
    }
    return n;
}

void TcpConn::HandleIO(uint32_t events)
{
    if (closed_)
        return;

    if (events & EPOLLIN)
        HandleRead();
    if (events & EPOLLOUT)
        HandleWrite();
    if (events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP))
        Close();
}

void TcpConn::HandleRead()
{
    int err = 0;
    int n = input_buffer_.Recv(fd_, &err);
    if (n > 0)
    {
        if (read_cb_)
            read_cb_();
    }
    else if (n == 0 || (n < 0 && (err != EAGAIN && err != EWOULDBLOCK)))
    {
        Close();
    }
}

void TcpConn::HandleWrite()
{
    int n = ::send(fd_, output_buffer_.data(), output_buffer_.size(), MSG_NOSIGNAL);
    if (n > 0)
    {
        output_buffer_.erase(0, n);
        if (output_buffer_.empty())
        {
            DisableWrite();
        }
    }
    else if (n < 0 && (errno != EAGAIN && errno != EWOULDBLOCK))
    {
        Close();
    }
}

void TcpConn::Close()
{
    if (closed_)
        return;
    closed_ = true;

    evloop_.DelEvent(fd_);
    close(fd_);
}

void TcpConn::DisableWrite()
{
    evloop_.ModEvent(fd_, EPOLLIN | EPOLLRDHUP, &io_handler_);
}

void TcpConn::EnableWrite()
{
    evloop_.ModEvent(fd_, EPOLLIN | EPOLLOUT | EPOLLRDHUP, &io_handler_);
}

void TcpConn::SetNonBlocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
    {
        std::cerr << "fcntl get error: " << errno << std::endl;
        return;
    }
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
    {
        std::cerr << "fcntl set error: " << errno << std::endl;
    }
}

std::string TcpConn::GetDataUntilCrLf()
{
    auto data = input_buffer_.GetDataUntilCRLF();
    if (data.first != nullptr)
    {
        std::string result(reinterpret_cast<char *>(data.first), data.second);
        input_buffer_.ReadCompleted(data.second+2); // 读取 \r\n
        return result;
    }
    return "";
}

std::string TcpConn::GetAllData()
{
    auto data = input_buffer_.GetAllData();
    if (data.first != nullptr)
    {
        std::string result(reinterpret_cast<char *>(data.first), data.second);
        input_buffer_.ReadCompleted(data.second);
        return result;
    }
    return "";
}