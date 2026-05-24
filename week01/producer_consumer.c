#include<stdio.h>
#include<pthread.h>
#include<stdlib.h>

int num[50];
int head=0;
int tail=0;
int count=0;

pthread_mutex_t mutex;
pthread_cond_t cond;
void * function_p(void* arg){

    while(1){
        pthread_t tid=pthread_self();
        pthread_mutex_lock(&mutex);
        while(head==tail&&count==50){
            pthread_cond_wait(&cond,&mutex);
        }
    
        num[tail]=rand()%1000;
        printf("子线程%ld号生产%d序列数据:%d\n",tid,tail,num[tail]);
        tail=(tail+1)%50;
        count++;
        pthread_cond_broadcast(&cond);
        pthread_mutex_unlock(&mutex);
    }
    
    return NULL;
}
void* function_c(void* arg){

    while(1){
        pthread_t tid=pthread_self();

        pthread_mutex_lock(&mutex);
        while(head==tail&&count==0){
            pthread_cond_wait(&cond,&mutex);
        }
   
        printf("子线程%ld号消费%d序列数据:%d\n",tid,head,num[head]);
        head=(head+1)%50;
        count--;
        pthread_cond_broadcast(&cond);
        pthread_mutex_unlock(&mutex);
    }
    return NULL;
}
int main(){
    pthread_t p[5],c[5];
    
    pthread_mutex_init(&mutex,NULL);
    pthread_cond_init(&cond, NULL);
    for(int i=0;i<5;i++){
        pthread_create(&p[i],NULL,function_p,NULL);
        pthread_create(&c[i],NULL,function_c,NULL);
    }


    for(int i=0;i<5;i++){
        pthread_join(p[i],NULL);
        pthread_join(c[i],NULL);

    }
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);
}