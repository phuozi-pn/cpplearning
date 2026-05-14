# Step 5: 智能指针面试 8 高频题深讲

> 配合 Step 4 智能指针笔记阅读
> 每题包含：面试官意图 + 完整答案 + 加分点

---

## ① unique_ptr 为啥禁拷贝？

### 面试官意图
检查是否懂"独占语义"背后的安全原因，不只是会用。

### 答案

如果允许拷贝，会出现两个 `unique_ptr` 指向同一对象：

```cpp
// 反例（实际不允许）
unique_ptr<int> p1(new int(42));
unique_ptr<int> p2 = p1;       // 假设拷贝成功
// 函数结束:
// p2 析构 → delete 一次  ✅
// p1 析构 → delete 同一个 int 第二次 → 💥 double-free
```

C++11 设计者直接在编译期禁用拷贝：

```cpp
template<typename T>
class unique_ptr {
    unique_ptr(const unique_ptr&) = delete;
    unique_ptr& operator=(const unique_ptr&) = delete;
};
```

**只允许 move**：
```cpp
unique_ptr<int> p2 = std::move(p1);   // ✅ 转移所有权
```

### 加分回答

> "unique_ptr 禁拷贝是 C++ 把 bug 从运行期挪到编译期的典型设计 —— 编译错比运行崩好排查。这种 zero-cost abstraction 是 C++ 现代化的核心理念。"

---

## ② shared_ptr 内部布局？

### 面试官意图
检查 `sizeof = 16` 的来源，引用计数的内存位置。

### 答案

```
栈上 shared_ptr 对象（16 字节）
┌────────────────────────────┐
│  T* ptr_         (8 字节)  │ ────┐ 指向对象
│  Control* ctrl_  (8 字节)  │ ──┐ │
└────────────────────────────┘   │ │
                                 │ │
堆上控制块                        │ │
┌────────────────────────────┐   │ │
│  T* managed_obj            │ <─┘ │  控制块也持有对象指针
│  atomic<int> strong_count  │     │  （为了正确 delete）
│  atomic<int> weak_count    │     │
│  Deleter   (可选)          │     │
└────────────────────────────┘     │
                                   │
堆上对象                            │
┌────────────────────────────┐     │
│  T (实际对象)               │ <───┘
└────────────────────────────┘
```

**关键点**：
1. shared_ptr 对象本身在栈上，只有 16 字节（2 个指针）
2. 控制块在堆上，所有指向同一对象的 shared_ptr 共享一个控制块
3. 控制块里的计数是原子变量（`std::atomic<int>`）

### 加分回答

> "控制块和对象通常是两次堆分配（除非 make_shared 优化成一次）。对象 delete 时机：strong=0；控制块 delete 时机：strong=0 且 weak=0。weak_ptr 也指向同一控制块，所以 weak_ptr 也是 16 字节。"

---

## ③ shared_ptr 线程安全？（最高频）

### 面试官意图
考察对原子操作 + 数据竞争的理解。**这题答得好直接影响面评**。

### 必须分 3 层讲

#### 层 1：引用计数 ✅ 线程安全

```cpp
auto sp = std::make_shared<int>(42);

std::thread t1([sp] { /* sp 拷贝构造 → 计数 +1（原子）*/ });
std::thread t2([sp] { /* sp 拷贝构造 → 计数 +1（原子）*/ });
// 计数最终正确归零 → 对象只 delete 一次 → 安全
```

**为什么**：控制块的 strong_count 是 `std::atomic<int>`，++/-- 是 CPU 原子指令。

#### 层 2：指向的对象本身 ⚠️ 不一定线程安全

```cpp
auto sp = std::make_shared<std::vector<int>>();

std::thread t1([sp] { sp->push_back(1); });
std::thread t2([sp] { sp->push_back(2); });
// 💥 vector 本身不是线程安全的，数据竞争
```

**结论**：shared_ptr 只保证"计数操作"安全，**对象需要用户加锁保护**。

#### 层 3：同一个 shared_ptr 变量被并发修改 ❌ 不安全

```cpp
std::shared_ptr<int> global_sp = std::make_shared<int>(1);

// ❌ 危险！
std::thread t1([&] { global_sp = std::make_shared<int>(2); });   // 写
std::thread t2([&] { auto p = global_sp; });                      // 读
// 💥 race condition: 写两个指针字段(ptr_+ctrl_)，另一个看到半改状态
```

**为什么不安全**：shared_ptr 对象有两个指针字段，**两个字段的修改不是原子的**。

**解法**：
- C++20 之前：用 `std::atomic_load(&sp)` / `std::atomic_store(&sp, new_sp)` 系列函数
- C++20 之后：用 `std::atomic<std::shared_ptr<T>>`

### 加分回答（背熟）

> "控制块计数线程安全，对象本身不一定线程安全，同一 shared_ptr 变量的并发读写也不安全。想并发修改 shared_ptr 变量本身，C++20 用 `atomic<shared_ptr<T>>`，C++20 前用 `std::atomic_*` 系列函数。"

---

## ④ 循环引用怎么解？

### 面试官意图
检查是否真的写过项目 vs 只看过书。

### 答案

**问题**：双向数据结构里两条链都用 shared_ptr，strong count 永远 ≥ 1。

```cpp
// 翻车例子
struct Parent {
    std::shared_ptr<Child> child;
};
struct Child {
    std::shared_ptr<Parent> parent;   // ⚠️ 循环
};
```

**解法**：父→子用 shared（强持有），子→父用 weak（避免环）：

```cpp
struct Parent {
    std::shared_ptr<Child> child;     // ✅ 父持有子
};
struct Child {
    std::weak_ptr<Parent> parent;     // ✅ 子知道父，不阻止父释放
};
```

### 工程常见场景

| 场景 | shared 方向 | weak 方向 |
|---|---|---|
| 双向链表 | next | prev |
| 树（含父指针）| 父→子 | 子→父 |
| 图（含反向边）| 主方向 | 反方向 |
| 观察者模式 | Subject 持有 Observer | Observer 持有 Subject |
| 缓存 | 业务持有 shared | 缓存持有 weak |

### 加分回答

> "循环引用是 shared_ptr 的固有缺陷，weak_ptr 优雅解决。判断方向用 weak 的原则：哪一边持有'强度较弱'的语义，用 weak。"

---

## ⑤ make_shared 优劣？（陷阱题）

### 面试官意图
检查对内存分配开销的敏感度。

### 优势：一次分配

```cpp
// ❌ 方式 A：两次堆分配
auto p = std::shared_ptr<MyObj>(new MyObj);   // 1 次（new MyObj）+ 1 次（控制块）

// ✅ 方式 B：一次堆分配
auto p = std::make_shared<MyObj>();           // MyObj + 控制块 紧挨着放在同一块内存
```

**好处**：
- 减少 1 次 `malloc` 调用
- 对象和控制块内存相邻，**cache 友好**
- 异常安全更好（new + 构造异常时 make_shared 不会内存泄漏）

#### 内存布局对比

```
方式 A（两次分配）:                 方式 B（make_shared）:

栈: sp                              栈: sp
    │                                   │
    ├─→ 控制块 [块 1，地址 A]            └─→ 控制块 + 对象 [一块连续内存]
    │                                       (地址 X)
    └─→ 对象     [块 2，地址 B]
                                        
内存碎片化                          缓存友好
```

### ⚠️ 劣势（陷阱题，能答出来加分）

**问题场景**：weak_ptr 长期存在时，**对象内存被绑死无法释放**。

```cpp
std::weak_ptr<HugeObject> cached;
{
    auto big = std::make_shared<HugeObject>();   // 对象+控制块 连续
    cached = big;
}   // big 析构 → strong=0 → 对象析构（调用 ~HugeObject）
    // 但是控制块还在（weak count=1）
    // ⚠️ 对象和控制块在同一块内存 → 那部分大对象内存无法释放！

// cached 长期持有 → HugeObject 占的大块内存一直不释放
```

如果用方式 A：对象 delete 后那块内存立刻释放，控制块那块小内存等 weak 归零后释放。

**结论**：99% 场景用 `make_shared`，但**对象很大 + 有长期 weak_ptr 持有时**考虑用 `shared_ptr(new T)`。

### 加分回答

> "make_shared 一次分配性能好，cache 友好；但有 weak_ptr 长期持有大对象时，对象内存无法立即释放。经验上 99% 用 make_shared，遇到大对象 + 缓存场景考虑用 new。"

---

## ⑥ weak_ptr 怎么访问对象？

### 面试官意图
检查是否懂 `lock()` 的原子性保证。

### 答案

**错误写法**：
```cpp
weak_ptr<MyObj> wp = ...;

// ❌ 危险（多线程下）
if (!wp.expired()) {
    auto sp = wp.lock();
    sp->use();   // 💥 expired 和 lock 之间，对象可能被释放
}
```

**正确写法**：
```cpp
// ✅ 原子操作：判断 + 升级一次完成
if (auto sp = wp.lock()) {
    sp->use();   // 这一段执行期间，对象保证活着
}   // sp 析构，strong -1
```

### lock() 内部干了什么

伪代码：
```cpp
shared_ptr<T> weak_ptr::lock() {
    // 用原子 CAS 操作尝试把 strong count +1
    // 如果 strong 已经是 0，返回空 shared_ptr
    // 如果 strong > 0，原子地 +1，返回有效 shared_ptr
}
```

**关键**：这个操作是**一步原子的**，避免了"刚判断完活着，下一刻就死了"的竞态。

### 加分回答

> "`lock()` 是原子的 strong 计数 +1，避免 expired+访问 之间的竞态。实务中除了打日志，几乎不用 expired，永远用 lock。"

---

## ⑦ sizeof(unique_ptr) = 8 字节

### 面试官意图
考察对 zero-cost abstraction（零开销抽象）的理解。

### 答案

```cpp
unique_ptr<int> p;
sizeof(p);    // 8（64 位系统）
sizeof(int*); // 8
// 一样大！unique_ptr 没有任何额外开销
```

**为什么**：
- unique_ptr 只有一个成员 `T* ptr_`
- 没有引用计数、没有控制块、没有线程同步
- 析构函数是 inline 的 `delete ptr_`，零运行时开销

**这就是 C++ "零开销抽象" 的体现**：
- 安全性：编译期防止泄漏 / 防止 double-free
- 性能：和裸指针完全等价

### 陷阱：自定义 Deleter 会让 sizeof 变大

```cpp
auto deleter = [](int* p) { delete p; };
unique_ptr<int, decltype(deleter)> p(new int(42), deleter);
sizeof(p);   // 可能 16 字节（多存了函数对象）
```

但默认 Deleter `default_delete<T>` 是**空类**（empty base optimization 优化），不占空间，所以默认 sizeof = 8。

### 加分回答

> "unique_ptr 是零开销抽象的典范，sizeof 等同裸指针，运行时性能完全相同。自定义 Deleter（函数对象）会让 sizeof 变大，函数指针更大；lambda 通常是空的可以保持 8 字节。"

---

## ⑧ sizeof(shared_ptr) = 16 字节

### 面试官意图
能否推出"为啥需要两个指针"。

### 答案

shared_ptr 内部结构：
```cpp
template<typename T>
class shared_ptr {
private:
    T* ptr_;           // 8 字节，指向对象
    Control* ctrl_;    // 8 字节，指向控制块
};
// sizeof = 16
```

### 陷阱题：为什么不只用一个指针？控制块里不是有对象指针吗？

**两个原因**：

#### 原因 1：性能 ★

每次 `p->use()` 都要**两次内存访问**：
1. 先读 `ctrl_` 拿到控制块指针
2. 再读 `ctrl_->managed_obj` 拿到对象指针
3. 再访问对象

而直接存对象指针只要 **一次内存访问**。热路径上是显著开销。

#### 原因 2：支持别名构造（aliasing constructor）★★

```cpp
struct Big {
    int huge_array[1000];
};

auto big = std::make_shared<Big>();

// 别名构造：让 small 共享 big 的生命周期，但指向 big 内部的某个元素
std::shared_ptr<int> small(big, &big->huge_array[5]);
```

`small` 共享 big 的控制块（**控制块还指向 big**），但 `small.get()` 返回 `&big->huge_array[5]`。

**这种 "控制块管的对象" 和 "shared_ptr 看到的对象" 不同**的设计，必须两个指针分别存。

### 加分回答

> "shared_ptr 存两个指针：对象指针让访问只需一次解引用；控制块指针管引用计数。两个指针的设计还支持'别名构造'：让 shared_ptr 指向对象内部的子对象，但和外层对象共享生命周期。"

---

## 速查表（面试前 5 分钟扫一遍）

| # | 问题 | 一句话答案 | 加分点 |
|---|---|---|---|
| 1 | unique_ptr 禁拷贝？ | 防 double-free | 编译期防 bug 是零开销抽象典型 |
| 2 | shared_ptr 布局？ | 2 指针 + 堆上控制块 | 控制块也持有对象指针 |
| 3 | 线程安全？ | 计数原子安全，对象 + 变量不安全 | C++20 `atomic<shared_ptr>` |
| 4 | 循环引用？ | 反向改 weak | 缓存 / 观察者也用 weak |
| 5 | make_shared 优劣？ | 1 次分配 vs 大对象 + 长 weak 占内存 | 99% 用 make_shared |
| 6 | weak_ptr 访问？ | `lock()` 原子升级 | 避免 expired+访问 竞态 |
| 7 | sizeof unique_ptr？ | 8 字节 | 自定义 Deleter 可能更大 |
| 8 | sizeof shared_ptr？ | 16 字节 | 性能 + 别名构造 |

---

## 学习方法

### 第一遍（首次学习）
读 1 遍，理解逻辑，**别担心记不住**。

### 第二遍（隔天复习）
合上文档，**自己用嘴讲一遍**（对着空气讲也行）。
讲不清楚的点 → 回头找。

### 第三遍（这周末）
**写成博客**发掘金（强制完整组织语言）。
这是 Lv2 → Lv3 的关键步骤。

### 第四遍（暑假写网络库时）
muduo 用到 `enable_shared_from_this` 时主动回忆这些知识点。
联系实际，到达 Lv4。

---

## 进阶题（暑假后再啃）

下面这些是面试官追问时可能问到的更难的题，目前先了解有这些东西，后面再深入：

```
1. enable_shared_from_this 是什么？为什么不能在构造函数里 shared_from_this()？
2. 自定义 Deleter 怎么写？sizeof 会变吗？
3. shared_ptr<T[]> 和 shared_ptr<T> 有什么区别？
4. weak_callback 模式怎么用 weak_ptr 实现？
5. 如何用 shared_ptr 实现 RCU（Read-Copy-Update）模式？
6. 为什么 shared_ptr 的 control block 不能用 shared_ptr 自管理？
```

---

> **记住：不要试图今天全记住。记住 50% 已经是 985 应届 Top 10%。剩下的 50% 在后面 5 个月会自然渗透到脑子里。**
