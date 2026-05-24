// 阻塞 TCP Echo Client —— 框架：请补全 TODO
// 编译: g++ -std=c++17 -Wall -Wextra -O2 echo_client.cpp -o echo_client
// 运行: ./echo_client 127.0.0.1 9000
// 提示: man 2 socket | connect | send | recv | close
//       man 3 inet_pton   ("127.0.0.1" -> sin_addr)

#include <arpa/inet.h>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "用法: " << argv[0] << " <ip> <port>\n";
        std::cerr << "示例: " << argv[0] << " 127.0.0.1 9000\n";
        return 1;
    }

    const char* ip = argv[1];
    const uint16_t port = static_cast<uint16_t>(std::stoi(argv[2]));

    // ---------- TODO 1: socket ----------
    int sock_fd = -1;

    // ---------- TODO 2: sockaddr_in + inet_pton + connect ----------
    // 提示: sockaddr_in addr{};
    //       addr.sin_family = AF_INET;
    //       addr.sin_port = htons(port);
    //       if (inet_pton(AF_INET, ip, &addr.sin_addr) != 1) { 无效 IP }
    //       connect(sock_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));

    // ---------- TODO 3: 交互或单次发送：从 cin 读一行，send，再 recv 打印 ----------
    // 提示: std::string line; std::getline(std::cin, line);
    //       send(sock_fd, line.data(), line.size(), 0);  // 注意返回值
    //       char buf[1024]; ssize_t n = recv(sock_fd, buf, sizeof(buf) - 1, 0);
    //       if (n > 0) { buf[n] = '\0'; std::cout << buf << '\n'; }

    // ---------- TODO 4: close(sock_fd) ----------

    (void)ip;
    (void)port;
    (void)sock_fd;
    return 0;
}
