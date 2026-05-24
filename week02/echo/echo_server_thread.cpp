// 多线程阻塞 TCP Echo Server
//
// 编译: g++ -std=c++17 -Wall -Wextra -O2 -pthread 
//          echo_server_thread.cpp -o echo_server_thread
// 运行: ./echo_server_thread
// 客户端: ~/code/cpplearning/week01/echo_client 127.0.0.1 9001

#include <arpa/inet.h>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <signal.h>
#include <sstream>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

void handle_client(int conn_fd, sockaddr_in cli) {
    std::stringstream tid_ss;
    tid_ss << std::this_thread::get_id();
    std::string tid = tid_ss.str();

    std::cout << "  [线程 tid=" << tid << "] ** 接手 conn_fd=" << conn_fd
              << " from " << inet_ntoa(cli.sin_addr) << ":"
              << ntohs(cli.sin_port) << " **" << std::endl;

    char buf[1024];
    for (;;) {
        ssize_t n = recv(conn_fd, buf, sizeof(buf) - 1, 0);

        if (n < 0) {
            perror("recv");
            break;  // ⚠️ 出错必须 break
        } else if (n == 0) {
            std::cout << "  [线程 tid=" << tid << "] -- 客户端关闭连接"
                      << std::endl;
            break;  // ⚠️ 客户端关闭必须 break（你之前缺的就是这行！）
        } else {
            buf[n] = '\0';
            std::cout << "  [线程 tid=" << tid << "] << 收到 " << n
                      << " 字节: \"" << buf << "\"" << std::endl;

            ssize_t sent = 0;
            while (sent < n) {
                ssize_t k = send(conn_fd, buf + sent, n - sent, 0);
                if (k <= 0) {
                    perror("send");
                    break;
                }
                sent += k;
            }

            if (sent == n) {
                std::cout << "  [线程 tid=" << tid << "] >> 已回显 " << n
                          << " 字节" << std::endl;
            } else {
                break;  // send 出错
            }
        }
    }

    close(conn_fd);
    std::cout << "  [线程 tid=" << tid << "] ## 任务完成, 线程退出"
              << std::endl;
}

int main() {
    const uint16_t kPort = 9001;

    signal(SIGPIPE, SIG_IGN);

    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd == -1) {
        perror("socket");
        return 1;
    }
    int opt = 1;
    if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        close(listen_fd);
        return 1;
    }
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(kPort);
    if (bind(listen_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        perror("bind");
        close(listen_fd);
        return 1;
    }
    if (listen(listen_fd, 16) < 0) {
        perror("listen");
        close(listen_fd);
        return 1;
    }

    std::stringstream main_tid;
    main_tid << std::this_thread::get_id();
    std::cout << "[主线程 tid=" << main_tid.str() << "] 监听端口 " << kPort
              << ", 等连接..." << std::endl;
    std::cout << "----------------------------------------" << std::endl;

    while (true) {
        sockaddr_in cli{};
        socklen_t len = sizeof(cli);

        int conn_fd = accept(listen_fd,
                             reinterpret_cast<sockaddr*>(&cli), &len);
        if (conn_fd < 0) {
            perror("accept");
            continue;
        }

        std::cout << "[主线程] >> 新连接 from "
                  << inet_ntoa(cli.sin_addr) << ":" << ntohs(cli.sin_port)
                  << "  (conn_fd=" << conn_fd << ")" << std::endl;

        std::thread([conn_fd, cli]() {
            handle_client(conn_fd, cli);
        }).detach();
    }

    close(listen_fd);
    return 0;
}
