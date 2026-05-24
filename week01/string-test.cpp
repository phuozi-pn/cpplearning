#include"string.h"

using namespace std;
int main(){
    String s1;
    String s2("hello");
    String s3(s1);
    cout << s3<<endl;
    s3=s2;
    cout <<s3<<endl;    
    int id=0;
auto f=[&id]()mutable{
    std::cout<<"id:"<<id<<std::endl;
    ++id;
};
id=42;
f();
f();
f();
std::cout<<id <<std::endl;

}