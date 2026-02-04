#pragma once

#include "MessageBuffer.h"
#include <memory>
#include <functional>

class EventLoop;
// TCP连接类
class TcpConn : public std::enable_shared_from_this<TcpConn>
{
public:
    using Ptr = std::shared_ptr<TcpConn>;
    using ReadCallback = std::function<void()>;
    using CloseCallback = std::function<void()>;

    TcpConn(int fd, EventLoop &evloop);

    ~TcpConn();

    void SetReadCallback(ReadCallback cb) { read_cb_ = cb; }

    std::string GetAllData();

    std::string GetDataUntilCrLf();

    int Send(const char* data, size_t size);

private:
    static void SetNonBlocking(int fd);
    void Close();

private:
    void HandleIO(uint32_t events);

    void HandleRead();

    void HandleWrite();
    
    void DisableWrite();
    void EnableWrite();

    int fd_;
    EventLoop &evloop_;
    bool closed_;
    std::string output_buffer_;
    MessageBuffer input_buffer_;
    ReadCallback read_cb_;
    std::function<void()> io_handler_;
};