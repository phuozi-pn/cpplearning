
#ifndef __MYSTRING__
#define __MYSTRING__
#include<iostream>
#include<cstring>
class String{

public:
    explicit String(const char* cstr=0);
    String(const String& str);  //构造函数接受自己同一类进行构造，叫做拷贝构造
    String& operator =(const String& str);  //同理，这叫拷贝赋值
    //只要类带指针，一定要写这两个函数
    ~String();  //析构函数，当类的对象死亡的时候，调用析构函数
    char* get_c_str() const{return m_data;}
 private:
    char* m_data;  //字符串长度不一，适合动态分配，所以选字符指针



};

inline 
String::String(const char*cstr){
    if(cstr){
        m_data=new char[strlen(cstr)+1];
        strcpy(m_data,cstr);
    }
    else{
        m_data=new char[1];
        *m_data='\0';
    }
}
inline 
String::~String(){
    delete [] m_data;
}



//深拷贝，拷贝构造
inline 
String::String(const String& str){
    m_data=new char[strlen(str.m_data)+1];
    strcpy(m_data,str.m_data);
}

//拷贝赋值
inline String& String::operator=(const String &str){
    if(this==&str)
    return *this;
    delete [] m_data;
    m_data=new char[strlen(str.m_data)+1];
    strcpy(m_data,str.m_data);
    return *this;
}
inline
std::ostream& operator<<(std::ostream & os,const String& str){
    os<<str.get_c_str();
    return os;
}

#endif