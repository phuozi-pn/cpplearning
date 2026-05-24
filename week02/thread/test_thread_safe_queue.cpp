// ThreadSafeQueue 测试程序 —— 这个文件不需要你填, 直接编译运行验证你的队列
//
// 编译: g++ -std=c++17 -Wall -Wextra -O2 -pthread 
//          test_thread_safe_queue.cpp -o test_queue
// 运行: ./test_queue
//
// 测试场景:
//   - 2 个生产者线程: 各自 push 5 个数
//   - 3 个消费者线程: 各自 pop 直到 shutdown
//   - 主线程: 等生产者完事, 调用 shutdown(), 等消费者退出

#include "thread_safe_queue.hpp"

#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

int main() {
    ThreadSafeQueue<int> q;
    std::atomic<int> total_produced{0};
    std::atomic<int> total_consumed{0};

    // ---------- 2 个生产者线程 ----------
    auto producer = [&q, &total_produced](int producer_id) {
        for (int i = 0; i < 5; i++) {
            int value = producer_id * 100 + i;
            q.push(value);
            total_produced.fetch_add(1);

            std::cout << "  [生产者 " << producer_id << "] push " << value
                      << " (队列长度 " << q.size() << ")\n" << std::flush;

            // 模拟生产间隔
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        std::cout << "[生产者 " << producer_id << "] 完成, 退出\n" << std::flush;
    };

    // ---------- 3 个消费者线程 ----------
    auto consumer = [&q, &total_consumed](int consumer_id) {
        int value;
        while (q.pop(value)) {  // pop 返回 false 时退出 (shutdown 信号)
            total_consumed.fetch_add(1);
            std::cout << "[消费者 " << consumer_id << "] pop  " << value
                      << " <-- 我抢到了\n" << std::flush;
            std::this_thread::sleep_for(std::chrono::milliseconds(30));
        }
        std::cout << "[消费者 " << consumer_id << "] 收到 shutdown, 退出\n"
                  << std::flush;
    };

    // ---------- 启动线程 ----------
    std::cout << "========== 启动 2 个生产者 + 3 个消费者 ==========\n";

    std::vector<std::thread> producers;
    for (int i = 1; i <= 2; i++) {
        producers.emplace_back(producer, i);
    }

    std::vector<std::thread> consumers;
    for (int i = 1; i <= 3; i++) {
        consumers.emplace_back(consumer, i);
    }

    // ---------- 等生产者结束 ----------
    for (auto& t : producers) {
        t.join();
    }
    std::cout << "\n========== 生产者全部完成, 等队列清空... ==========\n";

    // 等队列清空 (简单做法: 轮询)
    while (!q.empty()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    std::cout << "\n========== 队列空了, 通知消费者退出 ==========\n";
    q.shutdown();

    // ---------- 等消费者结束 ----------
    for (auto& t : consumers) {
        t.join();
    }

    // ---------- 验证 ----------
    std::cout << "\n========== 测试结果 ==========\n";
    std::cout << "生产总数: " << total_produced.load() << "\n";
    std::cout << "消费总数: " << total_consumed.load() << "\n";

    if (total_produced.load() == total_consumed.load() &&
        total_produced.load() == 10) {
        std::cout << "✅ 测试通过! 队列工作正常.\n";
        return 0;
    } else {
        std::cout << "❌ 测试失败! 生产/消费数量不匹配.\n";
        return 1;
    }
}
