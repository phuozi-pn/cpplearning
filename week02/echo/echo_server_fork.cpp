// 多进程阻塞 TCP Echo Server —— 升级版（完整 + 日志剧情）
// 编译: g++ -std=c++17 -Wall -Wextra -O2 echo_server_fork.cpp -o echo_server_fork
// 运行: ./echo_server_fork
// 客户端复用 W01 的: ../../week01/echo_client 127.0.0.1 9000
//
// 本次学习重点（学完能讲清就过关）：
//   1. 为什么父进程必须 close(conn_fd)？        → fd 引用计数
//   2. 为什么子进程要 close(listen_fd)？         → 资源隔离
//   3. SIGCHLD = SIG_IGN 是干嘛的？             → 避免僵尸
//   4. 子进程为什么用 exit 不用 return？         → 防止跳回父循环

#include <arpa/inet.h>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

int main() {
    const uint16_t kPort = 9000;

    // ============================================================
    // Step 1: 实验 C —— 故意不注册 SIGCHLD（让僵尸出现）
    // 正常版本应该是: signal(SIGCHLD, SIG_IGN);
    // ============================================================
    // signal(SIGCHLD, SIG_IGN);   // ← 实验 C: 故意注释掉

    // ============================================================
    // Step 2: socket / setsockopt / bind / listen
    // ============================================================
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
    std::cout << "[父 pid=" << getpid() << "] 监听端口 " << kPort
              << "，等连接..." << std::endl;
    std::cout << "----------------------------------------" << std::endl;

    // ============================================================
    // Step 3: accept + fork 主循环
    // ============================================================
    while (true) {
        sockaddr_in cli{};
        socklen_t len = sizeof(cli);

        // ---- 3a. accept ----
        int conn_fd = accept(listen_fd,
                             reinterpret_cast<sockaddr*>(&cli), &len);
        if (conn_fd < 0) {
            perror("accept");
            continue;
        }
        std::cout << "[父 pid=" << getpid() << "] >> 新连接 from "
                  << inet_ntoa(cli.sin_addr) << ":" << ntohs(cli.sin_port)
                  << "  (conn_fd=" << conn_fd << ")" << std::endl;

        // ---- 3b. fork ----
        pid_t pid = fork();

        if (pid < 0) {
            // ---- 3c. fork 出错 ----
            perror("fork");
            close(conn_fd);
            continue;
        } else if (pid == 0) {
            // ============================================================
            // 子进程分支
            // ============================================================
            close(listen_fd);  // 子不需要 listen_fd

            std::cout << "  [子 pid=" << getpid() << "] ** 我接手 conn_fd="
                      << conn_fd << " **" << std::endl;

            char buf[1024];
            for (;;) {
                ssize_t n = recv(conn_fd, buf, sizeof(buf) - 1, 0);
                if (n < 0) {
                    perror("recv");
                    break;
                } else if (n == 0) {
                    std::cout << "  [子 pid=" << getpid()
                              << "] -- 客户端关闭连接" << std::endl;
                    break;
                } else {
                    buf[n] = '\0';
                    std::cout << "  [子 pid=" << getpid() << "] << 收到 "
                              << n << " 字节: \"" << buf << "\"" << std::endl;

                    // 循环发满 n 字节
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
                        std::cout << "  [子 pid=" << getpid() << "] >> 已回显 "
                                  << n << " 字节" << std::endl;
                    } else {
                        break;  // send 出错
                    }
                }
            }

            close(conn_fd);
            std::cout << "  [子 pid=" << getpid() << "] ## 任务完成，退出"
                      << std::endl;
            exit(0);  // 必须 exit，不能 return（return 会跳回父循环）

        } else {
            // ============================================================
            // 父进程分支
            // ============================================================
            std::cout << "[父 pid=" << getpid() << "] ++ fork 出子进程 pid="
                      << pid << "，由它服务" << std::endl;
            std::cout << "[父 pid=" << getpid()
                      << "] 回到 accept 等下一个连接..." << std::endl;
            std::cout << "----------------------------------------"
                      << std::endl;

            // !!!!! 关键一行：必须 close(conn_fd) !!!!!
            // 不关 → fd 引用计数永远 ≥ 1 → CLOSE_WAIT 堆积 → fd 用光
            close(conn_fd);
        }
    }

    close(listen_fd);
    return 0;
}

/* ============================================================
 * 写完后必做：3 个实验（每个 5 分钟）
 * ============================================================
 *
 * 实验 A：验证多进程并发  ⭐ 必做
 *   终端 1: ./echo_server_fork
 *   终端 2: ../../week01/echo_client 127.0.0.1 9000   (输入 hello1 回车)
 *   终端 3: ../../week01/echo_client 127.0.0.1 9000   (输入 hello2 回车)
 *   终端 4: ../../week01/echo_client 127.0.0.1 9000   (输入 hello3 回车)
 *   预期：3 个客户端都收到回显，server 端日志出现 3 个不同子进程 pid
 *
 * 实验 B：故意制造 fd 泄漏 bug（理解 close(conn_fd) 的意义）⭐ 必做
 *   1. 把父进程分支里 close(conn_fd) 那行注释掉
 *   2. 重新编译运行
 *   3. 开 5 个客户端连进来又断开
 *   4. 在 server 端执行: ss -t | grep 9000
 *   5. 会看到一堆 CLOSE_WAIT 状态的连接 → fd 引用计数没归零
 *   6. 恢复 close(conn_fd) 再跑一次，CLOSE_WAIT 消失
 *
 * 实验 C：故意制造僵尸（理解 SIGCHLD 的意义）⭐ 必做
 *   1. 把 Step 1 的 signal(SIGCHLD, SIG_IGN) 删掉
 *   2. 重新编译运行
 *   3. 开 5 个客户端连一下就断开
 *   4. 在另一个终端执行: ps -ef | grep defunct
 *   5. 会看到一堆 <defunct> 的僵尸进程
 *   6. 恢复 signal 那行再跑，僵尸消失
 *
 * 实验 D：看进程家族树  ⭐⭐⭐⭐⭐ 强烈推荐（最酷）
 *   1. 启动 server
 *   2. 开 3 个 client 各自输入字符但**先不回车**（保持连接）
 *   3. 在新终端执行: pstree -p $(pgrep echo_server_fork)
 *   4. 会看到：
 *        echo_server_fork(N)─┬─echo_server_fork(N1)
 *                           ├─echo_server_fork(N2)
 *                           └─echo_server_fork(N3)
 *   5. 这就是你创造的"进程家族"，截图存档
 *
 * ============================================================
 * 学完检验（费曼法，关掉文件对着白纸答）：
 * ============================================================
 *   1. 这个程序运行起来有几个进程？它们的关系？
 *   2. 父进程持有哪些 fd？子进程持有哪些 fd？
 *      哪些必须关、为什么？
 *   3. 没有 signal(SIGCHLD, SIG_IGN) 会发生什么？为什么？
 *   4. 子进程结束为什么用 exit(0) 而不是 return 0？
 *   5. 这版多进程 echo 相比 W01 单连接版，优劣在哪？（≥3 条）
 *
 *   答出 4 题以上 → 过关，记入 daily_check.md
 *   答不出 → 回看 fork.md 的"fd 三层结构"和"僵尸 vs 孤儿"
 */
