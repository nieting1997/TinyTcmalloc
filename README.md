# TinyTcmalloc

类似于go 内存分配方式/Tcmalloc分配方式，实现了一个高并发的内存池。
* Thread Cache
    每个线程独有一份缓存，线程自己申请无需加锁。对比glibc(malloc使用)，这样就会很快。
* Central Cache
    线程共享。用于向Thread Cache提供需要的内存/从Page Heap中获取内存，并分割为想要的大小。
* Page Heap
    以页为单位进行存储与分配。

## 整体架构

<div align=center><img src="https://user-images.githubusercontent.com/90097659/132246387-86dd0fc7-6dae-432d-9913-d3a582b4ddd0.png" width="765"/> </div>


## 流程展示

### 分配流程

<div align=center><img src="https://user-images.githubusercontent.com/90097659/132246429-b4a35862-0dc6-44a9-b6ac-69c62c020407.png" height="765"/> </div>

### 释放流程

<div align=center><img src="https://user-images.githubusercontent.com/90097659/132246444-57f2e566-62e8-4765-8873-4854e9957ff4.png" height="765"/> </div>

## 主要组成

### Span

由多个页组成一个Span对象。一页大小为4K。其中表明了页号[虚拟内存位置]，页数，使用计数等。

### Spanlit

由Span组成的双向链表。

### PageHeap

基于SpanList构建，同时使用unordered_map记录PageID和Span的对应关系。

<div align=center><img src="https://user-images.githubusercontent.com/90097659/132246473-0d069cb6-9e56-40ce-8b96-a36943430822.png" width="765"/> </div>

单例模式实现，全局只有唯一一个Page Cache。

通过返回局部静态变量达到目的。

* 申请

    前提：加锁

    1，对应位置释放满足要求

    2，没有则向更大的页寻找满足要求的span

    3，128Page依然不满足要求，则向系统申请128Page内存挂在链表中，在重新从1开始

* 释放

    前提：加锁

    释放span，先向前找，再向后找，找到能合并的span。这样就将切小的span合并成大的span，减少了内存碎片。

### Central Cache

SpanList实现。

<div align=center><img src="https://user-images.githubusercontent.com/90097659/132246491-83b71aca-9a8e-457d-9c7c-e957983abc69.png" width="765"/> </div>

单例模式实现，全局只有唯一一个Page Cache。

通过返回局部静态变量达到目的。

* 申请

    前提：加锁

    1，Central Cache也有一个哈希映射的freelist，freelist中挂着span，从span中取出对象给Thread Cache。

    2，如果没有非空的span，向Page Heap申请一个span对象，分配内存并按大小分割链接后，返回。

* 释放

    前提：加锁

    当所有对象都回到了span，则将Span释放回Page Cache

### Thread Cache

<div align=center><img src="https://user-images.githubusercontent.com/90097659/132246523-87b86d96-f2ab-4ffa-8ab5-d508ffabc3eb.png" width="765"/> </div>

每个对象是一个自由链表。使用静态TLS使得每个线程都有自己独立的指针。

* 申请内存

    1,当内存申请size<=64k时在Thread Cache中申请内存，计算size在自由链表中的位置，如果自由链表中有内存对象时，直接从FistList[i]中Pop一下对象，时间复杂度是O(1)，且没有锁竞争。
    
    2,当FreeList[i]中没有对象时，则批量从Central Cache中获取一定数量的对象，插入到自由链表并返回一个对象

* 释放内存

    当释放内存小于64k时将内存释放回Thread Cache，计算size在自由链表中的位置，将对象Push到FreeList[i].
    当链表的长度过长，也就是超过一次向中心缓存分配的内存块数目时则回收一部分内存对象到Central Cache。

## 性能测试

1000个线程并发执行10轮，每轮申请/释放内存100次
|  size   | malloc  | TinyTcmalloc |
|  ----   | ----    | ----         |
| 16  | 申请：621ms 释放 299ms|申请：27ms 释放 17ms|
| 100  | 申请：528ms 释放 321ms |申请：32ms 释放 11ms|
| 10000  | 申请：2826ms 释放 1706ms |申请：74ms 释放 34ms|

## Todo

* Tcmalloc采用动态局部缓存，可支持线程退出清理数据。静态不支持。
* Tcmalloc采用Radix Tree保存PageID(即页ID)和它归属的Span之间的映射。可考虑替换。
