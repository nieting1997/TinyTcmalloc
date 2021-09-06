#pragma once

#include "SizeClass.h"
#include "FreeList.h"
#include "ThreadCache.h"
#include "CentralCache.h"

class ThreadCache
{
private:
	Freelist _freelist[NLISTS];//自由链表

public:
	//申请和释放内存对象
	void* Allocate(size_t size);
	void Deallocate(void* ptr, size_t size);

	//从中心缓存获取对象
	void* FetchFromCentralCache(size_t index, size_t size);

	//释放对象时，链表过长时，回收内存回到中心堆
	void ListTooLong(Freelist* list, size_t size);
};

_declspec (thread) static ThreadCache* tlslist = nullptr;


void* ThreadCache::FetchFromCentralCache(size_t index, size_t size)
{
	Freelist* freelist = &_freelist[index];
	size_t maxsize = freelist->MaxSize();
	size_t numtomove = min(SizeClass::NumMoveSize(size), maxsize);

	void* start = nullptr, *end = nullptr;
	size_t batchsize = CentralCache::getInstance().FetchRangeObj(start, end, numtomove, size);

	if (batchsize > 1)
	{
		freelist->PushRange(NEXT_OBJ(start), end, batchsize - 1);
	}

	if (batchsize >= freelist->MaxSize())
	{
		freelist->SetMaxSize(maxsize + 1);
	}

	return start;
}

//释放对象时，链表过长时，回收内存回到中心缓存
void ThreadCache::ListTooLong(Freelist* freelist, size_t size)
{
	void* start = freelist->PopRange();
	CentralCache::getInstance().ReleaseListToSpans(start, size);
}

//申请和释放内存对象
void* ThreadCache::Allocate(size_t size)
{
	size_t index = SizeClass::Index(size);//获取到相对应的位置
	Freelist* freelist = &_freelist[index];
	if (!freelist->Empty())//在ThreadCache处不为空的话，直接取
	{
		return freelist->Pop();
	}
	else
	{
		return FetchFromCentralCache(index, SizeClass::Roundup(size));
	}
}

void ThreadCache::Deallocate(void* ptr, size_t size)
{
	size_t index = SizeClass::Index(size);
	Freelist* freelist = &_freelist[index];
	freelist->Push(ptr);

	//满足某个条件时(释放回一个批量的对象)，释放回中心缓存
	if (freelist->Size() >= freelist->MaxSize())
	{
		ListTooLong(freelist, size);
	}
}


