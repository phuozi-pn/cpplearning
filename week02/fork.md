fork调用的一个奇妙之处就是它仅仅被调用一次，却能够返回两次，它可能有三种不同的返回值：  
    1）在父进程中，fork返回新创建子进程的进程ID；  
    2）在子进程中，fork返回0；  
    3）如果出现错误，fork返回一个负值；
   在fork函数执行完毕后，如果创建新进程成功，则出现两个进程，一个是子进程，一个是父进程。在子进程中，fork函数返回0，在父进程中，fork返回新创建子进程的进程ID。我们可以通过fork返回的值来判断当前进程是子进程还是父进程。


“其实就相当于链表，进程形成了链表，父进程的fpid(p 意味point)指向子进程的进程id, 因为子进程没有子进程，所以其fpid为0.

创建新进程成功后，系统中出现两个基本完全相同的进程，这两个进程执行没有固定的先后顺序，哪个进程先执行要看系统的进程调度策略。

每个进程都有一个独特（互不相同）的进程标识符（process ID），可以通过getpid（）函数获得，还有一个记录父进程pid的变量，可以通过getppid（）函数获得变量的值。

```


#include <unistd.h>

#include <stdio.h>

#include<sys/types.h>

int main(void)

{

   int i=0;

   printf("i  son/pa  ppid   pid   fpid \n");

   //ppid指当前进程的父进程pid

   //pid指当前进程的pid,

   //fpid指fork返回给当前进程的值

   for(i=0;i<2;i++){

       pid_t fpid=fork();

       if(fpid==0)

           printf("%d child  %4d  %4d  %4d \n",i,getppid(),getpid(),fpid);

       else

           printf("%d parent %4d  %4d  %4d \n",i,getppid(),getpid(),fpid);

   }

   return 0;

}
```

输出结果：

![[Pasted image 20260522163533.png]]


fork与缓冲区：
```

#include <unistd.h>

#include <stdio.h>

#include<sys/types.h>

int main(void)

{

   int i=0;

   printf("i  son/pa  ppid   pid   fpid /n");

   //ppid指当前进程的父进程pid

   //pid指当前进程的pid,

   //fpid指fork返回给当前进程的值

   for(i=0;i<2;i++){

       pid_t fpid=fork();

       if(fpid==0)

           printf("%d child  %4d  %4d  %4d /n",i,getppid(),getpid(),fpid);

       else

           printf("%d parent %4d  %4d  %4d /n",i,getppid(),getpid(),fpid);

   }

   return 0;

}
```

输出结果:
![[Pasted image 20260522163740.png]]

可以看到：
i  son/pa  ppid   pid   fpid /n输出了四次：


### 标准输出的“行缓冲”机制与进程克隆

1. **缓冲区（塑料袋）未清空**：因为写成了 /n，printf 认为没有结束这一行，于是把表头文字留在了父进程的“内存输出缓冲区”里，没有立刻刷到屏幕上。
    
2. **缓冲区被完整复制**：当程序执行 fork() 时，子进程会**100% 克隆**父进程的内存空间——**连同缓冲区里那句没发出去的表头文字，也一起复制到了子进程的肚子里**。
    
3. **最终集体“倒垃圾”**：4 个进程各自运行结束时，都会强制清空自己的缓冲区。每个人都把肚子里继承来的那句表头连同自己的数据一起吐了出来，导致表头重复了 4 次。

孤儿进程：
在分析打印出来的 PID 数据时，发现了一个奇特现象：

- 大儿子进程（PID: 4373）在第一轮循环时，父进程 PID（PPID）是 4372。
    
- 到了第二轮循环时，它的 PPID 突然变成了 1763（系统进程）。
    

**原因**：父进程 4372 执行速度太快，提前结束退出了。大儿子进程变成了**孤儿进程**。Linux 系统为了避免孤儿无人管理，会让系统中的“孤儿院进程”（如 systemd 或 init）自动收养它，因此它的 ppid 变成了收养它的系统进程 PID。


4. 黄金避坑指南（正确代码模板）
	在 fork() 之前，用 \n 或者手动执行 fflush(stdout); 清空缓冲区
    
   严格检查转义字符，是反斜杠 \n，不是正斜杠 /n。