
#include <iostream>
#include "EventLoop.h"
#include "TcpServer.h"
#include "TcpConnection.h"
#include "Timer.h"

int main() {
    EventLoop evloop;
    TcpServer server(evloop);

    server.Start(8080, [](TcpConn::Ptr conn) {
        std::cout << "New connection established\n";

        // conn->SetReadCallback([conn]() {
        //     std::cout << "Received: " << conn->GetAllData() << std::endl;
        //     conn->Send("HTTP/1.1 200 OK\r\nContent-Length: 12\r\n\r\nHello World!", 52);
        // });
        conn->SetReadCallback([conn]() {
            std::cout << "Received: " << conn->GetDataUntilCrLf() << std::endl;
            conn->Send("Hello World!\r\n", 15);
            TimerInstance()->AddTimeout(1000, [conn]() {
                std::cout << "Timeout 1 second\n";
                conn->Send("Hello after 1 second!\r\n", 24);
            });
        });
    });

    evloop.Run();
    return 0;
}