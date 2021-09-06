#pragma once

#include "SizeClass.h"
#include "ThreadCache.h"
#include "PageHeap.h"

void* ConcurrentAlloc(size_t size)
{
	if (size > MAX_BYTES)
	{
		Span* span = PageHeap::GetInstence().AllocBigPageObj(size);
		void* ptr = (void*)(span->pageid_ << PAGE_SHIFT);
		return ptr;
	}
	else
	{
		if (tlslist == nullptr)
		{
			tlslist = new ThreadCache;
		}

		return tlslist->Allocate(size);
	}
}

void ConcurrentFree(void* ptr){
	Span* span = PageHeap::GetInstence().MapObjectToSpan(ptr);
	size_t size = span->objsize_;
	if (size > MAX_BYTES)
	{
		PageHeap::GetInstence().FreeBigPageObj(ptr, span);
	}
	else
	{
		tlslist->Deallocate(ptr, size);
	}
}