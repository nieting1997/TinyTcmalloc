#pragma once

#include "SizeClass.h"
#include "FreeList.h"
#include "ThreadCache.h"
#include "CentralCache.h"

class ThreadCache
{
private:
	Freelist _freelist[NLISTS];//��������

public:
	//������ͷ��ڴ����
	void* Allocate(size_t size);
	void Deallocate(void* ptr, size_t size);

	//�����Ļ����ȡ����
	void* FetchFromCentralCache(size_t index, size_t size);

	//�ͷŶ���ʱ���������ʱ�������ڴ�ص����Ķ�
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

//�ͷŶ���ʱ���������ʱ�������ڴ�ص����Ļ���
void ThreadCache::ListTooLong(Freelist* freelist, size_t size)
{
	void* start = freelist->PopRange();
	CentralCache::getInstance().ReleaseListToSpans(start, size);
}

//������ͷ��ڴ����
void* ThreadCache::Allocate(size_t size)
{
	size_t index = SizeClass::Index(size);//��ȡ�����Ӧ��λ��
	Freelist* freelist = &_freelist[index];
	if (!freelist->Empty())//��ThreadCache����Ϊ�յĻ���ֱ��ȡ
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

	//����ĳ������ʱ(�ͷŻ�һ�������Ķ���)���ͷŻ����Ļ���
	if (freelist->Size() >= freelist->MaxSize())
	{
		ListTooLong(freelist, size);
	}
}


