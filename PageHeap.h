#pragma once


#include "Windows.h"
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <unordered_map>

#include "Span.h"
#include "FreeList.h"
#include "SizeClass.h"

class PageHeap
{
public:
	PageHeap(const PageHeap& p) = delete;
	PageHeap& operator=(const PageHeap& p) = delete;
	static PageHeap& GetInstence()
	{
		static PageHeap instance;
		return instance;
	}

	Span* _NewSpan(size_t n);
	Span* NewSpan(size_t n);
	Span* MapObjectToSpan(void* obj);
	void ReleaseSpanToPageCache(Span* span);

	Span* AllocBigPageObj(size_t size);
	void FreeBigPageObj(void* ptr, Span* span);

private:
	std::mutex _mutex;
	SpanList _spanlist[NPAGES];
	std::unordered_map<PageID, Span*> _idspanmap;

private:
	PageHeap() {}
};

Span* PageHeap::NewSpan(size_t n)
{
	std::unique_lock<std::mutex> lock(_mutex);
	return _NewSpan(n);
}

Span* PageHeap::_NewSpan(size_t n)
{
	assert(n < NPAGES);
	if (!_spanlist[n].Empty())
		return _spanlist[n].PopFront();

	for (size_t i = n + 1; i < NPAGES; ++i)
	{
		if (!_spanlist[i].Empty())
		{
			Span* span = _spanlist[i].PopFront();
			Span* splist = new Span;

			splist->pageid_ = span->pageid_;
			splist->npage_ = n;
			span->pageid_ = span->pageid_ + n;
			span->npage_ = span->npage_ - n;

			for (size_t i = 0; i < n; ++i)
				_idspanmap[splist->pageid_ + i] = splist;

			_spanlist[span->npage_].PushFront(span);
			return splist;
		}
	}

	Span* span = new Span;

	void* ptr = VirtualAlloc(0, (NPAGES - 1)*(1 << PAGE_SHIFT), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);


	span->pageid_ = (PageID)ptr >> PAGE_SHIFT;
	span->npage_ = NPAGES - 1;

	for (size_t i = 0; i < span->npage_; ++i)
		_idspanmap[span->pageid_ + i] = span;

	_spanlist[span->npage_].PushFront(span);
	return _NewSpan(n);
}

Span* PageHeap::AllocBigPageObj(size_t size)
{
	assert(size > MAX_BYTES);

	size = SizeClass::_Roundup(size, PAGE_SHIFT);
	size_t npage = size >> PAGE_SHIFT;
	if (npage < NPAGES)
	{
		Span* span = NewSpan(npage);
		span->objsize_ = size;
		return span;
	}
	else
	{
		void* ptr = VirtualAlloc(0, npage << PAGE_SHIFT,
			MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

		if (ptr == nullptr)
			throw std::bad_alloc();

		Span* span = new Span;
		span->npage_ = npage;
		span->pageid_ = (PageID)ptr >> PAGE_SHIFT;
		span->objsize_ = npage << PAGE_SHIFT;

		_idspanmap[span->pageid_] = span;

		return span;
	}
}

void PageHeap::FreeBigPageObj(void* ptr, Span* span)
{
	size_t npage = span->objsize_ >> PAGE_SHIFT;
	if (npage < NPAGES)
	{
		span->objsize_ = 0;
		ReleaseSpanToPageCache(span);
	}
	else
	{
		_idspanmap.erase(npage);
		delete span;
		VirtualFree(ptr, 0, MEM_RELEASE);
	}
}

Span* PageHeap::MapObjectToSpan(void* obj)
{
	PageID id = (PageID)obj >> PAGE_SHIFT;
	auto it = _idspanmap.find(id);
	if (it != _idspanmap.end())
	{
		return it->second;
	}
	else
	{
		assert(false);
		return nullptr;
	}
}

void PageHeap::ReleaseSpanToPageCache(Span* cur)
{
	std::unique_lock<std::mutex> lock(_mutex);

	if (cur->npage_ >= NPAGES)
	{
		void* ptr = (void*)(cur->pageid_ << PAGE_SHIFT);
		_idspanmap.erase(cur->pageid_);
		VirtualFree(ptr, 0, MEM_RELEASE);
		delete cur;
		return;
	}


	while (1)
	{
		PageID curid = cur->pageid_;
		PageID previd = curid - 1;
		auto it = _idspanmap.find(previd);

		if (it == _idspanmap.end())
			break;

		if (it->second->usecount_ != 0)
			break;

		Span* prev = it->second;

		if (cur->npage_ + prev->npage_ > NPAGES - 1)
			break;


		_spanlist[prev->npage_].Erase(prev);
		prev->npage_ += cur->npage_;
		for (PageID i = 0; i < cur->npage_; ++i)
		{
			_idspanmap[cur->pageid_ + i] = prev;
		}
		delete cur;

		cur = prev;
	}


	while (1)
	{
		PageID curid = cur->pageid_;
		PageID nextid = curid + cur->npage_;
		auto it = _idspanmap.find(nextid);

		if (it == _idspanmap.end())
			break;

		if (it->second->usecount_ != 0)
			break;

		Span* next = it->second;

		if (cur->npage_ + next->npage_ >= NPAGES - 1)
			break;

		_spanlist[next->npage_].Erase(next);

		cur->npage_ += next->npage_;
		for (PageID i = 0; i < next->npage_; ++i)
		{
			_idspanmap[next->pageid_ + i] = cur;
		}

		delete next;
	}

	_spanlist[cur->npage_].PushFront(cur);
}