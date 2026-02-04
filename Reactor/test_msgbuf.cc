#include <iostream>
#include <cassert>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include "MessageBuffer.h"

// 辅助打印函数
void PrintTestResult(const char* test_name, bool passed) {
    std::cout << test_name << ": " << (passed ? "PASSED" : "FAILED") << std::endl;
}

// 测试1: 基本读写操作
void TestBasicReadWrite() {
    MessageBuffer buf;
    bool passed = true;

    // 验证初始状态
    passed &= (buf.GetActiveSize() == 0);
    passed &= (buf.GetFreeSize() == buf.GetBufferSize());

    // 写入数据
    const char* test_data = "ABCDEFGH";
    buf.Write(reinterpret_cast<const uint8_t*>(test_data), 8);
    
    // 验证写入后状态
    passed &= (buf.GetActiveSize() == 8);
    passed &= (buf.GetFreeSize() == buf.GetBufferSize() - 8);
    
    // 验证数据内容
    auto all_data = buf.GetAllData();
    passed &= (all_data.second == 8);
    passed &= (memcmp(all_data.first, test_data, 8) == 0);

    // 读取部分数据
    buf.ReadCompleted(4);
    passed &= (buf.GetActiveSize() == 4);
    passed &= (memcmp(buf.GetReadPointer(), "EFGH", 4) == 0);

    PrintTestResult("TestBasicReadWrite", passed);
}

// 测试2: 缓冲区扩容
void TestBufferExpansion() {
    MessageBuffer buf(4);  // 初始大小4字节
    bool passed = true;

    // 写入3字节（剩余1字节）
    buf.Write(reinterpret_cast<const uint8_t*>("123"), 3);
    passed &= (buf.GetBufferSize() == 4);

    // 再写入3字节（触发扩容）
    buf.Write(reinterpret_cast<const uint8_t*>("456"), 3);
    passed &= (buf.GetBufferSize() > 4);  // 应扩容至4 + max(3, 2) = 6
    
    // 验证数据完整性
    auto all_data = buf.GetAllData();
    passed &= (all_data.second == 6);
    passed &= (memcmp(all_data.first, "123456", 6) == 0);

    PrintTestResult("TestBufferExpansion", passed);
}

// 测试3: 标准化操作
void TestNormalization() {
    MessageBuffer buf(8);
    bool passed = true;

    // 写入8字节
    buf.Write(reinterpret_cast<const uint8_t*>("12345678"), 8);
    buf.ReadCompleted(4);  // 读走前4字节
    
    // 标准化前状态
    passed &= (buf.GetReadPointer() - buf.GetBasePointer() == 4);
    passed &= (buf.GetActiveSize() == 4);

    // 执行标准化
    buf.Normalize();
    
    // 标准化后状态
    passed &= (buf.GetReadPointer() == buf.GetBasePointer());
    passed &= (buf.GetActiveSize() == 4);
    passed &= (memcmp(buf.GetReadPointer(), "5678", 4) == 0);

    PrintTestResult("TestNormalization", passed);
}

// 测试4: 获取完整数据
void TestGetAllData() {
    MessageBuffer buf;
    bool passed = true;

    // 空缓冲区测试
    auto empty_data = buf.GetAllData();
    passed &= (empty_data.second == 0);

    // 分片写入测试
    buf.Write(reinterpret_cast<const uint8_t*>("Hello"), 5);
    buf.Write(reinterpret_cast<const uint8_t*>(" World"), 6);
    
    auto all_data = buf.GetAllData();
    passed &= (all_data.second == 11);
    passed &= (memcmp(all_data.first, "Hello World", 11) == 0);

    PrintTestResult("TestGetAllData", passed);
}

// 测试5: 查找CRLF
void TestGetDataUntilCRLF() {
    MessageBuffer buf;
    bool passed = true;

    // 测试无CRLF情况
    buf.Write(reinterpret_cast<const uint8_t*>("NO_CRLF_HERE"), 11);
    auto crlf_data = buf.GetDataUntilCRLF();
    passed &= (crlf_data.first == nullptr);
    passed &= (crlf_data.second == 0);

    // 测试有CRLF情况
    buf.Write(reinterpret_cast<const uint8_t*>("\r\nNextLine"), 10);
    crlf_data = buf.GetDataUntilCRLF();
    passed &= (crlf_data.second == 11);  // "NO_CRLF_HERE" + \r之前的长度
    passed &= (memcmp(crlf_data.first, "NO_CRLF_HERE", 11) == 0);

    // 测试多个CRLF情况
    MessageBuffer buf2;
    buf2.Write(reinterpret_cast<const uint8_t*>("First\r\nSecond\r\nThird"), 17);
    auto first_crlf = buf2.GetDataUntilCRLF();
    passed &= (first_crlf.second == 5);  // "First"
    passed &= (memcmp(first_crlf.first, "First", 5) == 0);

    PrintTestResult("TestGetDataUntilCRLF", passed);
}

void TestRecvLogic() {
    MessageBuffer buf(4);  // 初始缓冲区大小4字节
    int err = 0;
    bool passed = true;

    // 模拟读取数据：主缓冲区剩余2字节，额外数据3字节
    // 假设通过mock或pipe提供输入数据 "12345"
    int fds[2];
    pipe(fds);
    write(fds[1], "12345", 5);
    close(fds[1]);

    // 读取5字节（主缓冲区4字节，实际空闲2字节）
    ssize_t n = buf.Recv(fds[0], &err);
    passed &= (n == 5);
    passed &= (buf.GetActiveSize() == 5);
    
    // 验证数据内容
    auto data = buf.GetAllData();
    passed &= (memcmp(data.first, "12345", 5) == 0);

    close(fds[0]);
    PrintTestResult("TestRecvLogic", passed);
}

int main() {
    TestBasicReadWrite();
    TestBufferExpansion();
    TestNormalization();
    TestGetAllData();
    TestGetDataUntilCRLF();
    TestRecvLogic();
    std::cout << "\nAll tests completed." << std::endl;
    return 0;
}