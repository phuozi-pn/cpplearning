#pragma once
// ============================================================
// 线程安全队列模板类 —— 暑假网络库的核心组件之一
//
// 用法示例:
//   ThreadSafeQueue<int> q;
//   线程 A: q.push(42);
//   线程 B: int v; q.pop(v);  // 阻塞等数据
//
// 学习目标 (完成后能讲清):
//   1. mutex + condition_variable 的标准搭配
//   2. unique_lock vs lock_guard 的区别
//   3. 虚假唤醒 (spurious wakeup) 是什么，怎么防
//   4. notify_one vs notify_all
//   5. shutdown 优雅停止的设计
// ============================================================

#include <condition_variable>
#include <mutex>
#include <queue>
#include <utility>  // std::move

template <typename T>
class ThreadSafeQueue {
public:
    ThreadSafeQueue() = default;

    // 禁用拷贝 (mutex 不能拷贝)
    ThreadSafeQueue(const ThreadSafeQueue&) = delete;
    ThreadSafeQueue& operator=(const ThreadSafeQueue&) = delete;

    // ============================================================
    // TODO 1: push —— 入队 + 唤醒一个等待的消费者
    // ============================================================
    // 提示步骤:
    //   1. 用 std::lock_guard<std::mutex> 加锁 (作用域结束自动解锁)
    //   2. queue_.push(std::move(value));
    //   3. lock 解锁前 / 后都行, 但通常先解锁再 notify (避免无谓唤醒后又抢锁)
    //   4. cv_.notify_one();
    // 进阶问题: 为什么用 notify_one 不用 notify_all?
    //   → 一次 push 只多了 1 个元素, 唤醒 1 个就够, 唤醒全部是浪费
    void push(T value) {
        // TODO 1
        {
            std::lock_guard<std::mutex> mutex(mtx_);
            queue_.push(std::move(value));
        }
        cv_.notify_one();
        

    }

    // ============================================================
    // TODO 2: pop —— 阻塞出队 (核心)
    // ============================================================
    // 返回值: true = 成功取到一个; false = shutdown 了且队列空, 应该结束
    //
    // 提示步骤:
    //   1. std::unique_lock<std::mutex> lock(mtx_);
    //      ⚠️ cv.wait 必须用 unique_lock (因为它要中途解锁/重锁)
    //
    //   2. cv_.wait(lock, [this]{ return !queue_.empty() || stopped_; });
    //      ↑ 这个谓词解决两件事:
    //        a) 虚假唤醒: 醒了发现队列还是空, 自动继续 wait
    //        b) shutdown 通知: stopped_ = true 时也唤醒, 让线程退出
    //
    //   3. 检查: 如果 stopped_ && queue_.empty() → return false
    //      (告诉调用者: "我醒来是因为关闭, 不是因为有数据")
    //
    //   4. value = std::move(queue_.front());
    //      queue_.pop();
    //      return true;
    bool pop(T& value) {
        // TODO 2
        std::unique_lock<std::mutex> lock(mtx_);
        cv_.wait(lock,[this]{return !queue_.empty()||stopped_;});
        if(stopped_ && queue_.empty()){
            return false;
        }
        value=std::move(queue_.front());
        queue_.pop();

        return true;
    }

    // ============================================================
    // TODO 3: try_pop —— 非阻塞版本 (可选, 但推荐写)
    // ============================================================
    // 提示: 不等, 队列空直接返回 false
    //   1. std::lock_guard<std::mutex> lock(mtx_);
    //   2. if (queue_.empty()) return false;
    //   3. value = std::move(queue_.front()); queue_.pop(); return true;
    bool try_pop(T& value) {
        // TODO 3
        std:: lock_guard<std::mutex>lock(mtx_);
        if(queue_.empty())return false;
        value=std::move(queue_.front());
        queue_.pop();
        return true;
    }

    // ============================================================
    // TODO 4: shutdown —— 优雅停止
    // ============================================================
    // 提示步骤:
    //   1. 加锁
    //   2. stopped_ = true;
    //   3. cv_.notify_all();
    //      ⚠️ 必须 notify_all (不是 one), 唤醒所有在 wait 的消费者
    void shutdown() {
        // TODO 4
        std:: lock_guard<std::mutex>lock(mtx_);
         stopped_ = true;
         cv_.notify_all();


    }

    // ============================================================
    // 工具方法 (已完成, 不用改)
    // ============================================================
    bool empty() const {
        std::lock_guard<std::mutex> lock(mtx_);
        return queue_.empty();
    }

    size_t size() const {
        std::lock_guard<std::mutex> lock(mtx_);
        return queue_.size();
    }

private:
    mutable std::mutex mtx_;        // mutable: 让 const 方法也能加锁
    std::condition_variable cv_;
    std::queue<T> queue_;
    bool stopped_ = false;
};

/* ============================================================
 * 写完后必做: 用 test_thread_safe_queue.cpp 验证
 * ============================================================
 * cd ~/code/cpplearning/week02/thread
 * g++ -std=c++17 -Wall -Wextra -O2 -pthread test_thread_safe_queue.cpp -o test_queue
 * ./test_queue
 *
 * 预期: 看到多个生产者/消费者线程交错打印
 *
 * ============================================================
 * 学完检验 (费曼法, 关掉文件答):
 * ============================================================
 *   1. 为什么 push/pop 都要先加锁?
 *   2. cv.wait 的第二个参数 (lambda) 是干嘛的?
 *   3. 什么是虚假唤醒? 怎么避免?
 *   4. notify_one 和 notify_all 的区别? 我们 push 时用哪个? shutdown 用哪个? 为什么?
 *   5. unique_lock 比 lock_guard 多了什么能力?
 *      (提示: cv.wait 内部要做"释放锁 → 睡眠 → 醒来重新加锁")
 *   6. shutdown 后, 已经在 wait 的线程怎么醒过来? 怎么知道是"该结束了"?
 *
 *   答出 5 题 = 过关, 记入 daily_check.md
 */
