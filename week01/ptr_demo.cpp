#include<iostream>
#include<memory>
#include<vector>

class Resource{

private:
    int id_;

public:
    Resource(int id):id_(id){std::cout<<"Res"<<id_<<"构造\n";}
    ~Resource(){std::cout<<"Res"<<id_<<"析构\n";}
    void use(){std::cout<<"Res"<<id_<<"使用中\n";}
};

void uniqueDemo(){
    std::cout<<"==== unique_ptr ====\n";
    auto p1=std::make_unique<Resource>(1);
    p1->use();
    auto p2=std::move(p1);
    if(!p1)std::cout<<"p1已空\n";
    p2->use();
}

void sharedDemo(){
    std::cout<<"\n===== shared_ptr=====\n";
    auto p1=std::make_shared<Resource>(2);
    std::cout<<"cout="<<p1.use_count()<<"\n";
    {
        auto p2=p1;
        std::cout<<"cout="<<p1.use_count()<<"\n";

    }
    std::cout<<"count="<<p1.use_count()<<"\n";
}
void weakedDemo(){
    std::cout<<"\n==== weak_ptr===\n";
    std::weak_ptr<Resource>wp;
    {
        auto sp=std::make_shared<Resource>(3);
        wp=sp;
        std::cout<<"expired?"<<wp.expired()<<"\n";
        if(auto temp=wp.lock()){
            temp->use();
        }
    }
    std::cout<<"expired?"<<wp.expired()<<"\n";
    if(auto temp=wp.lock()){
        std::cout<<"不会执行\n";

    }else{
        std::cout<<"lock失败，对象以释放\n";

    }
}
int main(){
    uniqueDemo();
    sharedDemo();
    weakedDemo();
    return 0;
}