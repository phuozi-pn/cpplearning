c 中：可以分为数据和函数，函数主要是处理这些数据，但是这些数据往往是全局的；
c++中：把数据和处理这些数据的函数包在一起，你的数据只有这个函数可以去处理，其他人看不到。从而创建对象。

类的分类：经典分类：一种不带指针，一种带指针。

程序一般包括：头文件+主程序+标准库

头文件：
#ifndef __COMPLEX__
#define __COMPLEX__
...
...
#endif 
防卫式声明

模板：
template<typename T>
class complex{
    private:
    T  re,im;
    friend complex& __dopal(complex*,const complex&);
    pubulic:
    complex(T r=0,T i=0):re(r),im(i){

    }
    double real() const{ return re;}
    double img() const{ return n=im;}  const----->意思是我不会改变数据
    complex& operator+=(const complex&);
}
complex<int>c1(2.6);

类中函数可以只写声明，定义在类外。
定义在类中的函数叫做内联，只是建议，具体看编译器如何处理。

构造函数：没有返回值
complex(double r=0,double i=0):re(r),im(i)  --->初始化列，直接初始化
{}
re=r   ---->初始化后在进行赋值

默认值：
complex(double r=0,double i=0):re(r),im(i)  --->double r=0,double i=0是默认值，如果没有赋值直接采用默认值
{}

构造函数有多个------ 函数重载

double real() const {return  re;}
void real(double r){re =r}

real 函数编译后的版本

？ real@ Complex@QUEST
? real@Complex@SET

二者不同

注意，重载函数不能产生歧义
例如：
complex(double r=0,double i=0):re(r),im(i)  
{}

complex():re(0),im(0){}

两个构造函数

complex c1;
complex c2();

调用构造函数时两者都可以，产生歧义，报错

{
    const complex c1(2,1);
    cout << c1.real();---->如果real 没有加上const,会报错
}

值传递和指针引用传递


友元:可以访问数据
friend complex& __dopal(complex*,const complex&);

inline complex& __doapl (complex* this,const complex& r){
    this->re+=r.re;
    this->im+=r.im;
    return *this;
}

表达式	类型	值类别 (value category)
this   complex*     右值（指针的值）
*this  complex      左值（lvalue）

*this 的类型是 complex，不是 complex&！

那为什么大家都说它"是引用"？
因为它是左值，左值可以绑定到引用，所以在 return *this（返回类型 T&）时，它被自动绑定到了返回的引用上 = "零拷贝地把自己传出去"。

借此优点，可以实现c3+=c2+=c1,连串使用


相同calss 的各个objects 互为友元

c++中操作符是一个函数，是可以重新定义的。

操作符重载---1，成员函数
c1+=c2,+=符号会对c1进行检查，如果c1类对该符号有定义的话把该行定义成调用相关函数。

inline complex&
complex::operator +=( const complex& r){---->complex 成员函数
    return __doapl(this,r);
}

所有的成员函数都带有隐藏的this.c2就是this,this 是指针。

reference 传递者无需知道接受者是以reference形式接受

操作符重载--------2，非成员函数

inline complex
operator + (const complex& x,const complex& y){
    return complex (real(x)+real(y),imag(x)+img(y));
}

inline complex
operator + (const complex& x,double y){
    return complex (real(x)+y,imag(x));
}

inline complex
operator + (double x,const complex& y){
    return complex (x+real(y),img(y));
}

这些函数绝不能返回引用
只能在函数里创建临时变量，如果返回临时变量的引用的话，返回后临时变量已死亡，得到错误结果。


临时对象：typename(),生命下一行就结束了，没有名字。


inline complex 
operator + (const complex& x){
    return x;
}

入参用 const 引用省拷贝，返回值用值（或引用）由语义决定。

return x;
编译器看到这行时，做的是：

1. 函数返回类型是 complex（值，不是引用）
2. x 是一个左值，类型 complex
3. 需要从这个左值构造一个 complex 临时对象返回
4. → 调用拷贝构造函数 complex(const complex&)
5. 这个新构造的临时对象传给调用方
所以"变成值"的本质是：编译器在 return 时偷偷调用了拷贝构造函数

原因 
1：一元 + 应该返回独立的新对象（语义）
内置类型 +5、+x 都返回独立的值，不是引用。一元 + 的语义就是"产生一个新的（可能转换过的）值"。

int x = 5;
auto y = +x;        // y 是新的 int 5
auto z = &(+x);     // ❌ 编译报错，+x 是右值，没有地址
如果你的 + 返回引用，就违反了这种语义对称性

原因 3：避免"对返回值赋值"
如果一元 + 返回引用：

+c = other;          // 看起来合法？这就很怪
按值返回的话，+c 是右值，不能被赋值，符合直觉。

ostream& operator <<(ostream& os,const complex&x){
    return os <<'('<<real(x)<<","<<img(x)<<')';
}

cout 是一个 ostream 的对象
可能是连续输出 所以返回的是ostream&
cout <<c1<<conj(c1)

如果没写拷贝构造和拷贝赋值，那么编译器会提供一套默认的拷贝构造和拷贝复制，1bit 1bit的拷贝

如果class里带指针，不要用默认的拷贝构造，拷贝构造，拷贝赋值，析构函数函数一定要自己写

big three 拷贝构造，拷贝赋值，析构函数

如果使用默认的拷贝构造和拷贝赋值函数

string 是一个字符指针
a=b 会发生什么

由于b里只有指针，所以a变成和b一样的指针，指向同一块。

而我们希望赋值之后两端都有相同的内容，a指向一块空间，b指向另一块空间，空间中内容相同

浅拷贝：只拷贝指针，两指针指向同一空间

stack 栈：
调用函数时，函数本身会形成一个stack,放置参数，只要离开作用域，生命就消失了
heap 堆：手动分配：动态获得，手动释放，delete 理解为将空间还给操作系统。

static local object:生命在作用域结束后任然存在，直到整个程序结束。
{
    static Complex c2(1,2);
}

全局变量：生命周期，整个程序

new: 先分配内存，再调用构造函数，
    Complex*pc=new Complex(1,2);

---->void* men=operator new(sizeof(Complex));------>其内部调用malloc(n)

---->pc=static_cast<Complex*>(men);//转型

pc->Complex::Complex(1,2);//构造函数----->Complex::Complex(pc,1,2);


delete:先调用dltor,再释放memory
delete ps;

String *ps=new String("Hello")
---->String::~String(ps);//析构函数,删去ps内动态分配的字符串
operator delete(ps)//释放内存----->free(ps)，删去ps本身

new char[]

必须搭配 delete []

进一步补充：static
complex 类分为数据和函数，数据和函数前都可以加static 关键字，使其成为静态函数或静态数据

complex c1,c2,c3;
cout<<c1.real();
cout<<c2.ral();

该段代码会创建三个存储空间，里面分别存储不静态的所有数据；

如果以c 的角度看的话：

complex c1, c2,c3;
cout<<complex::real(&c1);
cout<<conplex::real(&c2);
函数是一样的，只有一份。
c1地址成为指针，调用相同函数，传给他的是不同地址，
通过不同地址，处理不同的数据

静态数据和对象脱离了，不属于对象，单独的一份在内存某个区域内，
静态函数和成员函数内存上一样，只有一份。
静态函数没有this point,不能访问和处理对象里的东西，只能处理静态数据。
、
class Account{
    public:
    static double m_rate;
    static void set_rate(const dounle&x){m_rate=x};

}
double Account ::m_rate=8.0;
int main(){
    Account::set_rate(5.0);
    Account a;
    a.set_rate(7,0);
}
静态数据一定要在class外进行初始化；
double Account ::m_rate=8.0;

调用static 函数方法有二：
1.对象调用
2.class name 调用

单件设计模式：
class A{
    public:
    static A & getInstance{return a;} 
    setup(){}
    praivate:
    A();
    A(const A& rhs);
    static A a;
};

这个类构造函数都是private,外界无法创建新的A的对象，
但是由于有静态static A a;空间中存在一个a是静态的，只能由静态函数调用
A::getInstance().setup();天才

但是如果外界没有用到a,仍有静态a存在，占据空间。
优化：
class A{
    public:
    static A & getInstance{return a;} 
    setup(){}
    praivate:
    A();
    A(const A& rhs);
   
};
A& A::getInstance(){

    static A a;
    return a;
}



 精确版规则：
静态函数不能"通过隐式 this 调用"非静态成员函数
                            ↑↑↑↑↑↑↑↑↑
                            关键限定词
但静态函数能：
  ✅ 创建对象（→ 触发构造函数，是"语句"不是"调用"）
  ✅ 通过显式对象调用成员函数（obj.foo()）
  ✅ 通过对象指针 / 引用调用成员函数（p->foo()

static A a;静态函数不能调用非静态成员函数"这条规则，对构造函数其实不适用——因为构造函数压根不是被"调用"的！
构造函数是在创建对象过程中触发的，而静态函数是该类里的，对构造函数可以访问，因此可以成功创建。
如果在{}中创建，则会失败。


模板和模板函数

template<typename T>
模板中必须指明类型
template<class T>
模板函数会对模板进行类型推导，不必指明类型

namespace std{}对{}中进行封装

如何应用 std 里的东西：
1.using namespace std;  :全部打开

2.using std::cout; 局部打开

3.全名：std:: cout

类与类之间的关系

复合composition


[]{
    std::cout<<"Hello lambda"<<std::endl;
}();
没有名称，后面加一个小括号（），表示直接调用

auto L=[]{
     std::cout<<"Hello lambda"<<std::endl;
};
L();

通常这个用法更常见，L可以一直调用

[]()mutable throwSpec ->retType{}

三个参数可有可无，但是如果之中只要出现一个，就得加上（），否则（）可以省略
（）函数参数
[]取用外部变量，可以传值也可以传引用
int id=0;
auto f=[id]()mutable{
    std::cout<<"id:"<<id<<std::endl;
    ++id;
}
id=42;
f();
f();
f();
std::cout<<id <<std::endl;

等价于
class Functor{
    private:
    int id;
    public:
    void operator() (){
        std::cout<<id<<std::endl;
        ++idl
    }
};
Functor f;

int id=0;
auto f=[id]()mutable{
    std::cout<<"id:"<<id<<std::endl;
    ++id;
}
id=42;
f();
f();
f();
std::cout<<id <<std::endl;
运行结果：
id:0
id:1
id:2
42

