## Step 1: 顺时针螺旋解析命名法

![[Pasted image 20260514182133.png]]

步骤如上，不再赘述。

简单例子（练手）：
- `int* const p`  ——  p 是 const 指针，指向 int
- `int (*p)[5]`   ——  p 是数组指针，指向 int[5]
- `int (*p)(int)` ——  p 是函数指针，参数 int，返回 int

---

## Step 2: 辨析 const 常量指针 和 指针常量

![[Pasted image 20260514182632.png]]

- **常量指针**（`int* const p`）：指针**本身**是常量，**不能修改指向**，但是可以修改所指的内容。
- **指针常量**（`const int* p` 或 `int const* p`）：指针指向**常量**，可以修改指向，但是不能修改所指的内容。

**记忆神器**：`const` 修饰它**左边**的东西；如果 const 在最左边，则修饰右边。

```cpp
int * const p     // const 左边是 *，修饰 "指针本身" → 指向不能改
const int * p     // const 左边没东西，看右边 int → 修饰 "int 值" → 内容不能改
```

⚠️ 中文教材里 "常量指针" / "指针常量" 容易混淆，**面试时直接读代码语义**比记中文名靠谱。

---

## Step 3: 左值、右值 与 引用

### 左值 vs 右值

- **左值**：可以取地址的值（如变量名）
- **右值**：不能取址的值（中间计算结果、将消亡值、字面量）

### 引用：变量的别名

引用是变量的另一个名字，**底层可能用指针实现**（编译器决定），但语义上和指针不同：

| 引用 | 指针 |
|---|---|
| 必须初始化 | 可以延迟初始化 |
| 不能重新绑定 | 可以重新指向 |
| 不能为空 | 可以为 nullptr |

### 左值引用 / 右值引用

```cpp
int x = 5;
int& lref = x;          // 左值引用，必须绑左值
int&& rref = 10;        // 右值引用，必须绑右值
// int& bad = 10;       // ❌ 编译错
// int&& bad = x;       // ❌ 编译错
```

### 重载选择规则

- 实参是**左值**：优先匹配**拷贝**构造 / 拷贝赋值
- 实参是**右值**：优先匹配**移动**构造 / 移动赋值（前提是有，且 noexcept）

### `std::move` 函数

显式将左值强制转换为右值，触发移动语义。

⚠️ `std::move` **本身不移动任何东西**，它只是 `static_cast<T&&>`。
真正"移动"的是后面被调用的 **移动构造** 或 **移动赋值** 函数。
如果类没有 move ctor，`std::move(a)` 实际等价于 copy。

### `noexcept` 关键字

告诉**编译器**：这个函数承诺**不抛异常**（注意：不是"不出错"，出错和异常是两件事）。

**为什么对移动语义重要**：
- STL 容器（如 `vector`）扩容时需要把旧元素搬到新内存
- 用 **move**：旧元素被掏空，**中途抛异常会导致状态损坏**
- 用 **copy**：抛异常时旧元素还在，可以安全回滚
- 所以 STL 策略是：**move ctor 是 noexcept 才用 move，否则用 copy**

**结论**：手写 move ctor 一定要加 `noexcept`，否则 vector 扩容时退化成 copy。

```cpp
MyClass(MyClass&&) noexcept { ... }      // ✅ vector 会用 move
MyClass(MyClass&&) { ... }                // ❌ vector 会用 copy（更慢）
```

---

## Step 4: C++ 智能指针

### 4.1 RAII 原则

**R**esource **A**cquisition **I**s **I**nitialization
"资源获取即初始化"

**核心思想**：栈上的资源管理是自动的，堆上的资源管理是手动的，让栈上对象（自动析构）来托管堆上资源，**让"释放资源"这件事从程序员脑子里挪到编译器手里**。

**原理**：在栈上创建一个类，类中包含指针指向堆上资源，类的析构函数自动释放资源。栈对象离开作用域 → 自动析构 → 资源释放。

**适用范围远不止内存**：

| 资源 | RAII 类 |
|---|---|
| 堆内存 | `unique_ptr` / `shared_ptr` |
| 文件 | `std::fstream` |
| 锁 | `std::lock_guard` / `std::unique_lock` |
| 线程 | `std::jthread`（C++20）|
| socket / DB 连接 | 自定义 RAII 类 |

### 4.2 三大智能指针

#### unique_ptr —— 独占

- 独占所有权
- 禁止拷贝，只能 `std::move` 转移
- 内部只有一个裸指针，**零开销**
- `sizeof(unique_ptr<T>) == sizeof(T*)`（8 字节）
- 创建：`std::make_unique<T>(...)` 比 `new` 更安全

#### shared_ptr —— 共享

- 共享所有权，通过**引用计数**
- 内部含两个指针：**指向对象** + **指向控制块**（控制块在堆上）
- 控制块存：**strong count** + **weak count** + **deleter**
- 计数归 0 → `delete` 对象
- 易陷入循环引用 → 内存泄漏
- 创建：`std::make_shared<T>(...)` 比 `new shared_ptr<T>(...)` 高效（一次分配 vs 两次）

**线程安全**：
- 引用计数操作是**原子的** → 线程安全
- shared_ptr 指向的**对象本身**不一定线程安全（用户自己加锁）
- 同一个 shared_ptr 变量**并发修改不安全**

#### weak_ptr —— 弱观察者

- **不持有对象**，不参与 strong 计数（但占 weak 计数）
- 不延长对象生命周期，只观察对象是否还活着
- 必须先 `lock()` 升级为 shared_ptr 才能访问
- 用途：**打破循环引用** / **缓存** / **观察者模式**

关键 API：
- `expired()`：判断对象是否已释放
- `lock()`：原子地尝试升级为 shared_ptr，对象已死则返回空 shared_ptr
- 多线程安全访问**永远用 lock()，不要用 expired()+访问**（中间可能被释放）

### 4.3 循环引用问题

```cpp
struct Node {
    std::shared_ptr<Node> next;
    std::shared_ptr<Node> prev;   // ⚠️ 双 shared 会循环
};
```

**问题**：a, b 互相通过 shared_ptr 持有，函数结束后 strong count 永远 ≥ 1 → 永不释放。

**解法**：把"反向"的那条改成 `weak_ptr`：

```cpp
struct Node {
    std::shared_ptr<Node> next;
    std::weak_ptr<Node> prev;     // ✅ 反向用 weak
};
```

**原则**：父→子用 shared（强持有），子→父用 weak（避免环）。

### 4.4 面试高频要点

| 问题 | 简要答案 |
|---|---|
| unique_ptr 为啥禁拷贝？ | 防 double-free |
| shared_ptr 内部布局？ | 两个指针 + 堆上控制块 |
| shared_ptr 线程安全？ | 计数原子安全，对象本身和变量修改不安全 |
| 循环引用怎么解？ | 一边改成 weak_ptr |
| `make_shared` 优劣？ | 优：一次分配；劣：weak_ptr 存在时对象内存延迟释放 |
| weak_ptr 怎么访问对象？ | 必须 `lock()` 升级，**不要直接 `wp->`** |
| `sizeof(unique_ptr)` ? | 8 字节（一个裸指针）|
| `sizeof(shared_ptr)` ? | 16 字节（对象指针 + 控制块指针）|
