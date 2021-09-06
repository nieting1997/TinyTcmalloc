#pragma once

#include "Span.h"
#include "FreeList.h"
#include "SizeClass.h"
#include "PageHeap.h"

class CentralCache
{
public:
	CentralCache(const CentralCache& p) = delete;
	CentralCache& operator=(const CentralCache& p) = delete;
	static CentralCache& getInstance()
	{
		static CentralCache instance;
		return instance;
	}

	Span* GetOneSpan(SpanList& spanlist, size_t byte_size);
	size_t FetchRangeObj(void*& start, void*& end, size_t n, size_t byte_size);
	void ReleaseListToSpans(void* start, size_t size);

private:
	SpanList spanlist_[NLISTS];
private:
	CentralCache() {}
};

Span* CentralCache::GetOneSpan(SpanList& spanlist, size_t byte_size)
{
	Span* span = spanlist.Begin();
	while (span != spanlist.End())
	{
		if (span->list_ != nullptr)
			return span;
		else
			span = span->next_;
	}

	Span* newspan = PageHeap::GetInstence().NewSpan(SizeClass::NumMovePage(byte_size));
	char* cur = (char*)(newspan->pageid_ << PAGE_SHIFT);
	char* end = cur + (newspan->npage_ << PAGE_SHIFT);
	newspan->list_ = cur;
	newspan->objsize_ = byte_size;
	while (cur + byte_size < end)
	{
		char* next = cur + byte_size;
		NEXT_OBJ(cur) = next;
		cur = next;
	}
	NEXT_OBJ(cur) = nullptr;
	spanlist.PushFront(newspan);
	return newspan;
}

size_t CentralCache::FetchRangeObj(void*& start, void*& end, size_t n, size_t byte_size)
{
	size_t index = SizeClass::Index(byte_size);
	SpanList& spanlist = spanlist_[index];
	std::unique_lock<std::mutex> lock(spanlist.mutex_);


	Span* span = GetOneSpan(spanlist, byte_size);

	size_t batchsize = 0;
	void* prev = nullptr;
	void* cur = span->list_;
	for (size_t i = 0; i < n; ++i)
	{
		prev = cur;
		cur = NEXT_OBJ(cur);
		++batchsize;
		if (cur == nullptr)
			break;
	}

	start = span->list_;
	end = prev;

	span->list_ = cur;
	span->usecount_ += batchsize;

	if (span->list_ == nullptr)
	{
		spanlist.Erase(span);
		spanlist.PushBack(span);
	}
	return batchsize;
}

void CentralCache::ReleaseListToSpans(void* start, size_t size)
{
	size_t index = SizeClass::Index(size);
	SpanList& spanlist = spanlist_[index];
	std::unique_lock<std::mutex> lock(spanlist.mutex_);

	while (start)
	{
		void* next = NEXT_OBJ(start);
		Span* span = PageHeap::GetInstence().MapObjectToSpan(start);
		NEXT_OBJ(start) = span->list_;
		span->list_ = start;
		if (--span->usecount_ == 0)
		{
			spanlist.Erase(span);
			PageHeap::GetInstence().ReleaseSpanToPageCache(span);
		}
		start = next;
	}
}