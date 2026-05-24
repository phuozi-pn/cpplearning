# week01/echo — 阻塞 TCP Echo

## 编译

```bash
g++ -std=c++17 -Wall -Wextra -O2 echo_server.cpp -o echo_server
g++ -std=c++17 -Wall -Wextra -O2 echo_client.cpp -o echo_client
```

## 运行

终端 1：

```bash
./echo_server
```

终端 2：

```bash
./echo_client 127.0.0.1 9000
```

## 无客户端时的快速自测

服务端跑起来后，可用：

```bash
nc 127.0.0.1 9000
```

输入内容应被回显（需在服务端实现 echo 后）。

## man 建议顺序

`socket` → `bind` → `listen` → `accept` → `recv` / `send`；客户端 `connect`。
