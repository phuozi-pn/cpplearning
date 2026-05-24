```
#pragma once

// ============================================================

// 线程安全队列模板类 —— 暑假网络库的核心组件之一

//

// 用法示例:

//   ThreadSafeQueue<int> q;

//   线程 A: q.push(42);

//   线程 B: int v; q.pop(v);  // 阻塞等数据

//

// 学习目标 (完成后能讲清):

//   1. mutex + condition_variable 的标准搭配

//   2. unique_lock vs lock_guard 的区别

//   3. 虚假唤醒 (spurious wakeup) 是什么，怎么防

//   4. notify_one vs notify_all

//   5. shutdown 优雅停止的设计

// ============================================================

  

#include <condition_variable>

#include <mutex>

#include <queue>

#include <utility>  // std::move

  

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

    //   1. 用 std::lock_guard<std::mutex> 加锁 (作用域结束自动解锁)

    //   2. queue_.push(std::move(value));

    //   3. lock 解锁前 / 后都行, 但通常先解锁再 notify (避免无谓唤醒后又抢锁)

    //   4. cv_.notify_one();

    // 进阶问题: 为什么用 notify_one 不用 notify_all?

    //   → 一次 push 只多了 1 个元素, 唤醒 1 个就够, 唤醒全部是浪费

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

    //   1. std::unique_lock<std::mutex> lock(mtx_);

    //      ⚠️ cv.wait 必须用 unique_lock (因为它要中途解锁/重锁)

    //

    //   2. cv_.wait(lock, [this]{ return !queue_.empty() || stopped_; });

    //      ↑ 这个谓词解决两件事:

    //        a) 虚假唤醒: 醒了发现队列还是空, 自动继续 wait

    //        b) shutdown 通知: stopped_ = true 时也唤醒, 让线程退出

    //

    //   3. 检查: 如果 stopped_ && queue_.empty() → return false

    //      (告诉调用者: "我醒来是因为关闭, 不是因为有数据")

    //

    //   4. value = std::move(queue_.front());

    //      queue_.pop();

    //      return true;

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

    //   1. std::lock_guard<std::mutex> lock(mtx_);

    //   2. if (queue_.empty()) return false;

    //   3. value = std::move(queue_.front()); queue_.pop(); return true;

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

    //   1. 加锁

    //   2. stopped_ = true;

    //   3. cv_.notify_all();

    //      ⚠️ 必须 notify_all (不是 one), 唤醒所有在 wait 的消费者

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

    mutable std::mutex mtx_;        // mutable: 让 const 方法也能加锁

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

 *   1. 为什么 push/pop 都要先加锁?

 *   2. cv.wait 的第二个参数 (lambda) 是干嘛的?

 *   3. 什么是虚假唤醒? 怎么避免?

 *   4. notify_one 和 notify_all 的区别? 我们 push 时用哪个? shutdown 用哪个? 为什么?

 *   5. unique_lock 比 lock_guard 多了什么能力?

 *      (提示: cv.wait 内部要做"释放锁 → 睡眠 → 醒来重新加锁")

 *   6. shutdown 后, 已经在 wait 的线程怎么醒过来? 怎么知道是"该结束了"?

 *

 *   答出 5 题 = 过关, 记入 daily_check.md

 */
 
```
总结：
### 1. 锁的管理：`lock_guard` vs `unique_lock`

这是 C++ 中基于 **RAII**（资源获取即初始化）管理互斥锁的两大核心工具，它们在局部函数结束（出作用域）时，都会**自动触发析构函数来释放锁**。

|**工具**|**核心特性**|**内存与底层机制**|**适用场景**|
|---|---|---|---|
|**`std::lock_guard`**|**死板、纯粹**。一生只爱一个人，构造即加锁，至死才解锁。**没有** `.lock()` 和 `.unlock()` 成员函数。|零成本抽象。内部仅保存 `mutex` 的引用，析构时无脑调用 `.unlock()`。|简单的、不需要中途解锁的短临界区。|
|**`std::unique_lock`**|**灵活、全能**。支持中途随时手动 `.lock()` 和 `.unlock()`，支持延迟加锁（`std::defer_lock`）。|略带开销。内部除了锁的指针，还维护了一个**布尔标志位 `_M_owns`**，用来记录当前是否持有锁，防止重复解锁。|复杂的条件分支加锁、**必须配合条件变量使用**。|
```
template <typename _Mutex>
class lock_guard {
private:
    _Mutex& _M_device; // 核心：仅仅保存了外面那把锁的【引用】（8字节指针）

public:
    // 1. 构造函数：在局部变量诞生在栈上的一瞬间，强制把传进来的锁给锁上
    explicit lock_guard(_Mutex& __m) : _M_device(__m) { 
        _M_device.lock(); 
    }

    // 2. 析构函数：局部变量生命周期结束（出大括号弹出栈）时，自动解锁
    ~lock_guard() { 
        _M_device.unlock(); // ！！！注意：这里只负责调用 unlock()，大铁锁本身还在内存里
    }

    // 3. 语法防御：强行禁止拷贝和赋值，防止一把锁被无意中释放两次
    lock_guard(const lock_guard&) = delete;
    lock_guard& operator=(const lock_guard&) = delete;
};
```

```
// 现代 C++ 的标签分发（Tag Dispatch）机制
struct defer_lock_t { explicit defer_lock_t() = default; };
inline constexpr defer_lock_t defer_lock{}; // 这就是你传入的那个“免辣小纸条”

template <typename _Mutex>
class unique_lock {
private:
    _Mutex* _M_device; // 指向互斥锁的指针
    bool    _M_owns;   // 核心：记录当前【是否真正占有着锁】

public:
    // 重载版本 A：普通构造（一出生就加锁）
    explicit unique_lock(_Mutex& __m) : _M_device(&__m), _M_owns(true) {
        _M_device->lock();
    }

    // 重载版本 B：带标签构造（对应 unique_lock lk(mtx, std::defer_lock);）
    // 编译器通过第二个参数的类型（defer_lock_t）精准匹配到这里！
    unique_lock(_Mutex& __m, defer_lock_t) noexcept 
        : _M_device(&__m), _M_owns(false) {
        // ！！！看这里：内部是空的！只记下了锁的地址，但是根本没有去执行 lock()！
    }

    // 手动解锁成员函数
    void unlock() {
        if (!_M_owns) throw std::system_error(...); // 没锁的情况下重复解，直接抛异常
        _M_device->unlock(); // 执行真正的底层的解锁
        _M_owns = false;     // ！！！关键：把状态标记改为 false
    }

    // 析构函数
    ~unique_lock() {
        if (_M_owns) { // ！！！智能的关键：只有当内部标记为 true 时，死的时候才会去解锁
            _M_device->unlock();
        }
    }
};
```

### 2. 线程的协同：`std::condition_variable`

如果说互斥锁是为了“抢资源”，条件变量就是为了“打配合”。它是**生产者-消费者模型**的灵魂。

- **`cv.wait(lock, pred)` 的底层原理解密：**
    
    当条件不满足时，它在原子时间内执行：**释放锁 $\rightarrow$ 线程挂起让出 CPU 卧倒睡觉**。
    
    被唤醒时执行：**重新抢锁 $\rightarrow$ 抢到锁后继续往下走**。由于内部频繁解锁和加锁，它**强制要求**必须传入 `std::unique_lock`。
    
- **高频面试杀手锏：虚假唤醒（Spurious Wakeup）**
    
    线程在没有任何人 notify 的情况下可能自己莫名其妙醒来。所以必须使用 `cv.wait(lock, []{ return ...; })` 传入一个 Lambda 表达式。其底层是一个 `while` 循环，醒来后会重新检测状态，不满足就继续拍回睡眠状态。
```
template <typename _Predicate>
void condition_variable::wait(unique_lock<mutex>& __lock, _Predicate __p) {
    // 它的底层本质其实就是一个伪装起来的 while 循环！
    while (!__p()) { // 1. ！！！重新调用你传进来的 Lambda 匿名函数（高时效性检测）
        
        // 2. 如果检测结果为 false（没货），开始准备睡觉
        // 获取底层的 pthread_mutex_t 锁指针
        pthread_mutex_t* __mtx = __lock.mutex()->native_handle(); 
        
        // 3. 释放当前的锁！修改 unique_lock 内部的状态标志
        __lock._M_owns = false; 

        // 4. 【系统调用原子操作】：陷入内核，解开 mtx 并让当前线程挂起休眠
        // 这一步会卡死在这行代码上，直到另一个线程调用 notify_one() 将其唤醒
        pthread_cond_wait(&_M_cond, __mtx); 

        // 5. 【线程被唤醒的一瞬间】：从内核醒来后，第一件事就是强行把锁再重新抢回来
        // 抢锁成功后，重新把 unique_lock 内部的状态标志设为 true
        __lock._M_owns = true; 
        
    } // 6. 回到循环开头，再次执行 Lambda 函数 __p()，确保不是“虚假唤醒”
}
```

Lambda 表达式的本质（闭包类的内存寻址）
```
cv.wait(lock, [this]{ return !queue_.empty() || stopped_; });

```

```
// 编译器在后台暗暗为你生成的“闭包类”
class __Lambda_Check_Queue_Status {
private:
    MyClass* _this_ptr; // 核心：[this] 捕获的本质就是在结构体里存了一个外面的成员指针！

public:
    // 构造函数：把当前的 this 指针复制进来存好
    __Lambda_Check_Queue_Status(MyClass* current_this) : _this_ptr(current_this) {}

    // 重载了小括号运算符，让这个结构体对象可以像函数一样被调用
    bool operator()() const {
        // 重点：每次调用它，都是顺着内部存的 _this_ptr 指针，
        // 实时去访问主内存（RAM）中最新的 queue_ 和 stopped_ 状态！
        return !_this_ptr->queue_.empty() || _this_ptr->stopped_;
    }
};

// ==========================================
// 刚才那句 cv.wait 在编译后的等价展开其实长这样：
// ==========================================
__Lambda_Check_Queue_Status my_closure(this); // 诞生一个闭包对象，把 this 指针塞进去
cv.wait(lock, my_closure); // 把这个带有指针寻址能力的对象传给条件变量
```
### 3. 性能优化黄金法则：先解锁，再通知

在生产者推送数据后，存在两种唤醒消费者的顺序：

- **标准老手写法（推荐）：** 先 `lock.unlock();` 腾出钥匙，再 `cv.notify_one();` 唤醒消费者。消费者醒来顺理成章直接拿到锁，无缝衔接。
    
- **带锁通知写法（尽量避免）：** 先 `cv.notify_one();` 再出大括号解锁。消费者刚醒来就去撞一个还没释放的锁，抢锁失败被迫二次卡顿，会造成无谓的线程上下文切换（Context Switch）开销。

### 4. 现代 C++ 编译器魔术：Lambda 的内存本质

当你写下 `cv.wait(lock, [this]{ return !queue_.empty(); });` 时，看似神奇的“实时高时效性”来自于 C++ 的底层类型转换：

- **为什么普通变量不行？** 传入普通变量（表达式）相当于传了一个“历史快照照片”，变量值一旦传入函数栈就定死了，无法感知外部主内存的变化。
    
- **Lambda 的真相：** 编译器在后台把 Lambda 生成了一个**匿名的结构体（闭包类）**，`[this]` 使得这个结构体内部**拷贝并牢牢握住了当前类对象的 `this` 指针**。
    
- **时效性来源：** 每次 `cv.wait` 被唤醒重新检查时，都是通过这个 `this` 指针，**顺藤摸瓜实时去访问主内存中 `queue_` 的最新状态**。

### 5. 性能榨汁机：`std::move()` 移动语义

在执行 `queue_push(std::move(value))` 时，其性能提升的核心在于：

- **本质：** `std::move` **不移动任何数据**，它只是一个强转，把变量转成右值引用，相当于打小报告告诉编译器：“这货是个临时工，它的内存资产你可以随便抢。”
    
- **原理：** 队列的 `push` 收到右值信号，放弃高昂的“深拷贝（新开辟内存，挨个复制）”，转而执行“移动构造”——**直接接管 `value` 内部的内存指针地址**。
    
- **代价：** 被 `move` 后的变量内部已被“掏空”（如 vector/string 会变空），后续绝不能再次使用它。

## 进阶高阶认知：C++ 与系统的技术对齐

这一路你不仅弄懂了高级语法，还穿透到了计算机底层：

1. **语法层面（C++）**：通过 **Tag Dispatch（标签分发）** 机制（如 `std::defer_lock` 这种空结构体作为编译期类型标签），在编译阶段精准控制编译器去匹配哪一个重载构造函数。
    
2. **系统层面（Linux）**：`std::mutex::lock()` 的底层是 **Futex（快速用户空间互斥体）**。它极其聪明地采取“无竞争时在用户态用 CPU 原子指令（CAS）解决，快如闪电；有竞争时才发起系统调用陷入内核态挂起线程，不耗 CPU”的渣男策略，将高并发性能压榨到极致。
```
void linux_futex_mutex_lock(int* futex_word) {
    // 1. 【用户态】原子操作：利用 CPU 的 CAS 指令，尝试把内存里代表锁的数字从 0 改为 1
    // 这个操作由于不经过操作系统内核，只需要 2~3 个 CPU 周期（纳秒级），快到飞起！
    if (atomic_compare_and_swap(futex_word, 0, 1) == 0) {
        return; // 抢锁成功！直接返回，无缝执行临界区代码
    }

    // 2. 【用户态自旋】（可选优化）：如果上面被别人占了，原地空循环转几圈，再抢几次试试
    for (int i = 0; i < 100; ++i) {
        if (atomic_compare_and_swap(futex_word, 0, 1) == 0) return; 
        cpu_relax(); // 让出 CPU 简短的执行流水线
    }

    // 3. 【内核态】真的抢不到锁，线程准备认输，不再浪费 CPU 算力
    // 发起系统调用，告诉内核：“大哥，我抢不到锁了，把我的线程状态改成 SLEEP，塞进等待队列吧”
    // 此时内核接管，线程彻底睡眠，CPU 占用率立刻清零
    syscall(SYS_futex, futex_word, FUTEX_WAIT, 1, NULL, NULL, 0);

    // 4. 【被唤醒后】：当占有锁的线程 unlock 并在内核中执行 FUTEX_WAKE 把它摇醒后
    // 线程从这里睁开眼，重新回到步骤 1，再次用原子操作 CAS 抢锁，成功后继续前进！
}
```
`std::lock_guard` 在初始化（构造）时，**必须**传入一个已经存在的、已经初始化的 `std::mutex` 对象。

### 1. 为什么必须要传？

因为 `std::lock_guard` 本身**不负责创建锁**，它只是一个“锁的管理帮手”。 它的工作流程是：

1. **出生（构造）时**：一把夺过你传给它的那个 `mutex`，并立刻调用 `mutex.lock()` 把它锁上。
    
2. **死亡（析构）时**：自动调用该 `mutex.unlock()` 把它释放。
    

如果你不传任何 `mutex` 给它，它就不知道该去锁谁，编译器会直接报错。

```
#include <mutex>

class SmartCounter {
private:
    int count_ = 0;
    std::mutex mtx_; // 1. 先声明并初始化好这个底层的锁（默认构造）

public:
    void increment() {
        // 2. 在这里把 mtx_ 传给 lock_guard。
        //    这一步会立刻执行 mtx_.lock()
        std::lock_guard<std::mutex> lock(mtx_); 
        
        count_++; 
    } // 3. 函数结束，lock 变量销毁，自动执行 mtx_.unlock()
};
```
### 二、 操作系统内核层面：`std::mutex::lock()` 发生了什么？

当我们追随 `_M_device->lock()` 继续往下挖，就来到了操作系统的地盘。在 Linux 环境下，`std::mutex` 的底层技术叫做 **Futex（Fast Userspace Mutex，快速用户空间互斥体）**。

这是多线程历史上最伟大的设计之一。在它诞生之前，每次加锁都要陷入内核态，非常慢。而 Futex 采取了“无竞争时在用户态解决，有竞争时再求助内核”的渣男策略，把性能榨干到了极致。

它的底层执行逻辑可以用下面这个顺序来表达：

**1.原子操作抢锁 (用户态)：**耗时: ~几纳秒。

线程尝试使用 CPU 的**原子指令**（如 `CAS, Compare-And-Swap` 或 `XCHG`）直接去修改内存中的一个锁标志位（比如将 0 改为 1）。

如果运气好，此时没有其他线程争抢，**标志位修改成功，直接加锁成功**！整个过程完全在用户态完成，不需要惊动操作系统内核，速度快到飞起。

**2.自旋等待 (根据策略可选)：**耗时: 极短。

如果第一步发现锁被别人占了，线程不会立刻认输。它会原地转几个圈（执行几次空循环，即**自旋 Spin**），赌对方会在几个纳秒内把锁释放。如果赌对了，立刻抢锁前进。

**3.陷入内核挂起 (内核态)：**进入系统调用。

如果转了几圈锁还是被占着，说明对方在干重活。当前线程不再浪费 CPU 算力，它会发起一个 `futex(..., FUTEX_WAIT, ...)` **系统调用**，正式向操作系统大哥求助。

内核收到请求后，把这个线程的状态改为**睡眠（Blocked）**，塞进该锁的等待队列里，然后把 CPU 资源让给别人。此时，该线程完全不消耗 CPU。

**4.唤醒与重抢：**解锁时的内核动作。

当占有锁的线程执行 `unlock()` 时，它同样先用原子指令释放标志位。如果发现等待队列里有挂起的线程，它会发起 `futex(..., FUTEX_WAKE, ...)`，由内核把等待队列里的第一个线程**唤醒（变回可运行状态）**。

被唤醒的线程重新回到第一步，抢到锁后继续高歌猛进.


std::unique_lock<std::mutex> lock(mtx_, std::defer_lock);  为什么可以这样写

### 1. 语法层面：什么是 `std::defer_lock`？

`std::defer_lock` 并不是一个普通的变量，它是 C++ 标准库中定义的一个**空结构体的常量实例**。它的定义大体上长这样：
```
struct defer_lock_t { explicit defer_lock_t() = default; };
inline constexpr defer_lock_t defer_lock{};
```
它在代码里的唯一作用，就是充当一个“类型标签”（Tag）**。C++ 编译器正是通过这个标签的**类型，来决定去匹配哪一个构造函数。

#### 带标签写法：`std::unique_lock<std::mutex> lock(mtx_, std::defer_lock);`

匹配的是下面这个带标签的重载构造函数：
```
unique_lock(mutex_type& __m, defer_lock_t) noexcept
  : _M_device(&__m), _M_owns(false) { // 1. 同样记录锁的地址，但把 _M_owns 设为 false！
    // 2. ！！！这里是空的，没有任何 lock() 操作
}
```
**底层本质**：当你传入 `std::defer_lock` 时，它只是在内部把 `mtx_` 的地址存了下来，并把**代表是否持有锁的内部标志位 `_M_owns` 设为了 `false`**。整行代码没有触发任何实际的锁竞争，所以它能瞬间执行完，绝对不会卡住。

```
struct defer_lock_t { explicit defer_lock_t() = default; };
```
这句代码定义了一个空结构体（类），名字叫 `defer_lock_t`。

- **`struct defer_lock_t`**：创建一种全新的数据类型。这个后缀 `_t` 代表 `type`（类型）。
    
- **`explicit defer_lock_t() = default;`**：
    
    - `= default` 让编译器生成一个默认的构造函数。
        
    - `explicit` 是为了**防止乱套**。它规定：你想创建这个纸条，必须老老实实写 `defer_lock_t{}`，绝对不允许把其他奇奇怪怪的东西（比如数字 0 或者 `false`）隐式转换成这个纸条。
        

**总结**：这一步，我们在 C++ 的世界里创造了一种专门用来当标签的类型，叫 `defer_lock_t`。

```
inline constexpr defer_lock_t defer_lock{};
```
- **`defer_lock`**：这就是你天天在代码里写的那个小助手，它是一个真正的**变量（实例）**。
    
- **`defer_lock_t`**：说明这个变量的类型就是我们刚刚定义的那个空结构体。
    
- **`{}`**：代表初始化这个空结构体。
    
- **`constexpr`**：告诉编译器，这个小纸条是**编译期常量**。它的存在不会在运行时占用任何多余的开销，效率极高。
    
- **`inline` (C++17引入)**：防止多个头文件重复引入它时导致链接冲突（头文件里定义全局变量的标配写法）。