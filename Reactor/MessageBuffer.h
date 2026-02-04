#pragma once

#include <bits/types/struct_iovec.h>
#include <stdint.h>
#include <vector>
#include <cstring>
#include <sys/uio.h>
#include <errno.h>

class MessageBuffer
{
public:
    MessageBuffer() : rpos_(0), wpos_(0)
    {
        buffer_.resize(4096); // Initial size
    }

    explicit MessageBuffer(std::size_t size) : rpos_(0), wpos_(0)
    {
        buffer_.resize(size);
    }

    // 不允许拷贝
    MessageBuffer(const MessageBuffer &) = delete;
    MessageBuffer &operator=(const MessageBuffer &) = delete;

    // 允许移动
    MessageBuffer(MessageBuffer &&other) noexcept
        : buffer_(std::move(other.buffer_)), rpos_(other.rpos_), wpos_(other.wpos_)
    {
        other.rpos_ = 0;
        other.wpos_ = 0;
    }

    MessageBuffer &operator=(MessageBuffer &&other) noexcept
    {
        if (this != &other)
        {
            buffer_ = std::move(other.buffer_);
            rpos_ = other.rpos_;
            wpos_ = other.wpos_;
            other.rpos_ = 0;
            other.wpos_ = 0;
        }
        return *this;
    }

    uint8_t *GetBasePointer()
    {
        return buffer_.data();
    }

    uint8_t *GetReadPointer()
    {
        return buffer_.data() + rpos_;
    }

    uint8_t *GetWritePointer()
    {
        return buffer_.data() + wpos_;
    }

    void ReadCompleted(std::size_t size)
    {
        rpos_ += size;
    }

    void WriteCompleted(std::size_t size)
    {
        wpos_ += size;
    }

    std::size_t GetActiveSize() const
    {
        return wpos_ - rpos_;
    }

    std::size_t GetFreeSize() const
    {
        return buffer_.size() - wpos_;
    }

    std::size_t GetBufferSize() const
    {
        return buffer_.size();
    }

    void Normalize()
    {
        if (rpos_ > 0)
        {
            std::memmove(buffer_.data(), buffer_.data() + rpos_, GetActiveSize());
            wpos_ -= rpos_;
            rpos_ = 0;
        }
    }

    void EnsureFreeSpace(std::size_t size)
    {
        if (GetBufferSize() - GetActiveSize() < size)
        {
            Normalize();
            buffer_.resize(buffer_.size() + std::max(size, buffer_.size() / 2));
        }
        else if (GetFreeSize() < size)
        {
            Normalize();
        }
    }

    // windows iocp  boost.asio
    void Write(const uint8_t *data, std::size_t size)
    {
        if (size > 0)
        {
            EnsureFreeSpace(size);
            std::memcpy(GetWritePointer(), data, size);
            WriteCompleted(size);
        }
    }

    std::pair<uint8_t *, std::size_t> GetAllData()
    {
        return {GetReadPointer(), GetActiveSize()};
    }

    // 获取第一个 \r\n 之前的数据的指针和大小（若未找到返回nullptr和0）
    std::pair<uint8_t *, std::size_t> GetDataUntilCRLF()
    {
        uint8_t *data = GetReadPointer();
        std::size_t active_size = GetActiveSize();
        for (std::size_t i = 0; i < active_size - 1; ++i)
        {
            if (data[i] == '\r' && data[i + 1] == '\n')
            {
                return {data, i}; // 数据长度为i，不包含\r\n
            }
        }
        return {nullptr, 0}; // 未找到
    }

    // linux reactor readv
    // 1. 尽可能的不腾挪数据
    // 2. 避免了每次都从栈上拷贝到堆上
    int Recv(int fd, int *err)
    {
        char extra[65535]; // 65535
        struct iovec iov[2];
        iov[0].iov_base = GetWritePointer();
        iov[0].iov_len = GetFreeSize();
        iov[1].iov_base = extra;
        iov[1].iov_len = sizeof(extra);
        std::size_t n = readv(fd, iov, 2);
        if (n < 0)
        {
            *err = errno;
            return n;
        }
        else if (n == 0)
        {
            *err = ECONNRESET;
            return 0;
        }
        else if (n <= GetFreeSize())
        {
            WriteCompleted(n);
            return n;
        }
        else
        {
            // WRN: GetfreeSize() 在 WriteCompleted() 中会被更新, extra_size 需要提前计算
            std::size_t extra_size = n - GetFreeSize();
            WriteCompleted(GetFreeSize());
            Write(reinterpret_cast<uint8_t *>(extra), extra_size);
            return n;
        }
    }

private:
    std::vector<uint8_t> buffer_;
    std::size_t rpos_;
    std::size_t wpos_;
};
