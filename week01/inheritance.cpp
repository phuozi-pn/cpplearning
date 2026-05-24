#include <iostream>

// 1. 定义基础组件 Component
class Component {
public:
    Component() { std::cout << "  -> Component Constructor\n"; }
    ~Component() { std::cout << "  -> Component Destructor\n"; }
};

// ==========================================
// Case 1: Derived 内部复合了 Component
// ==========================================
namespace Case1 {
    class Base {
    public:
        Base() { std::cout << "1. Base Constructor\n"; }
        virtual ~Base() { std::cout << "1. Base Destructor\n"; }
    };

    class Derived : public Base {
    private:
        Component comp; // Derived 拥有 Component
    public:
        Derived() { std::cout << "3. Derived Constructor\n"; }
        ~Derived() override { std::cout << "3. Derived Destructor\n"; }
    };
}
int main() {
    std::cout << "=== 测试 Case 1: Derived 复合 Component ===\n";
    {
        Case1::Derived obj1;
    } // obj1 在这里析构
    return 0;
}