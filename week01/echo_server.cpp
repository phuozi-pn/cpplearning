// 阻塞 TCP Echo Server —— 框架：请补全 TODO
// 编译: g++ -std=c++17 -Wall -Wextra -O2 echo_server.cpp -o echo_server
// 提示: man 2 socket | bind | listen | accept | recv | send | close
//       man 2 setsockopt  (可选 SO_REUSEADDR)
//       man 7 ip          (sockaddr_in, INADDR_ANY)

#include <arpa/inet.h>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

int main() {
    const uint16_t kPort = 9000;  // 端口被占用就换一个

    // ---------- TODO 1: socket ----------
    // 提示: int fd = socket(AF_INET, SOCK_STREAM, 0);
    //       失败则 perror + return
    int fd=socket(AF_INET,SOCK_STREAM,0);
    if(fd==-1){
        perror("socket");
        /*
        void perror(const char *s)
        {
            fprintf(stderr,
            "%s: %s\n",
            s,
            strerror(errno));
        }
        */
        exit(EXIT_FAILURE);
    }
    

    // ---------- TODO 2 (可选): setsockopt SO_REUSEADDR ----------
    // 提示: int opt = 1; setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, ...)
    //int opt=1;
    //setsockopt(listen_fd,SOL_SOCKET,SO_REUSEADDR);
    int opt=1;
    if(setsockopt(fd,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt))<0){
        perror("setsockopt");
        close(fd);
        return 1;
    }
    // ---------- TODO 3: sockaddr_in + bind ----------
    // 提示: sockaddr_in addr{};
    //       addr.sin_family = AF_INET;
    //       addr.sin_addr.s_addr = htonl(INADDR_ANY);
    //       addr.sin_port = htons(kPort);
    //       bind(listen_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
    sockaddr_in addr{};
    addr.sin_family=AF_INET;
    addr.sin_addr.s_addr=htonl(INADDR_ANY);

    addr.sin_port=htons(kPort);
    int bin=bind(fd,reinterpret_cast<sockaddr*>(&addr),sizeof(addr));
    // ---------- TODO 4: listen ----------
    // 提示: listen(listen_fd, backlog);  backlog 如 16 或 128
    if(bin<0){
        perror("bind");
        close(fd);
        return 1;
    }
    int lis=listen(fd,16);
    if(lis<0){
        perror("listen");
        close(fd);
        return 1;
    }
    std::cout << "listening on " << kPort << " ...\n";

    // ---------- TODO 5: accept 循环（第一版可先只 accept 一次）----------
    // 提示: sockaddr_in cli{}; socklen_t len = sizeof(cli);
    //       int conn_fd = accept(listen_fd, ...);
    sockaddr_in cli{};
    socklen_t len=sizeof(cli);
    int conn_fd=accept(fd,reinterpret_cast<sockaddr*>(&cli),&len);
    // ---------- TODO 6: echo 循环 recv -> send ----------
    // 提示: char buf[1024];
    //       for (;;) { ssize_t n = recv(conn_fd, buf, sizeof(buf), 0);
    //         n < 0: perror + break
    //         n == 0: 对端关闭, break
    //         n > 0: send 回去（可能要用 while 发满 n 字节）
    //       }
    char buf[1024];
    for(;;){
        ssize_t n=recv(conn_fd,buf,sizeof(buf),0);
        if (n < 0) {
            perror("recv");
            break;  // 出错，退出循环
        } 
        else if (n == 0) {
            // 对端关闭连接
            printf("Client closed connection\n");
            break;
        }
        else {
            // 发送数据回去，可能要循环确保发送完 n 字节
            ssize_t sent = 0;
            while (sent < n) {
                ssize_t k = send(conn_fd, buf + sent, n - sent, 0);
                if (k <= 0) {
                    perror("send");
                    break;  // 出错，退出发送循环
                }
                sent += k;
            }
            // 如果 send 出错，就退出主循环
            if (sent < n) {
                break;
            }
        }
    }
    // ---------- TODO 7: close(conn_fd); 如需多客户端可回到 accept ----------
    close(conn_fd);
    // ---------- TODO 8: close(listen_fd) ----------

    close(fd);  // 去掉这行当你用上 listen_fd 后
    return 0;
}
