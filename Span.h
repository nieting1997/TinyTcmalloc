#pragma once
#include <mutex>

typedef size_t PageID;

//连续内存的映射
struct Span
{
	PageID pageid_ = 0;//页号
	size_t npage_ = 0;//页数

	Span* prev_ = nullptr;
	Span* next_ = nullptr;
	void* list_ = nullptr;

	size_t objsize_ = 0;//对象的大小
	size_t usecount_ = 0;//对象使用计数,
};

//环形链表
class SpanList
{
public:
	Span* head_;
	std::mutex mutex_;

public:
	SpanList()
	{
		head_ = new Span;
		head_->next_ = head_;
		head_->prev_ = head_;
	}
	~SpanList()
	{
		Span * cur = head_->next_;
		while (cur != head_)
		{
			Span* next = cur->next_;
			delete cur;
			cur = next;
		}
		delete head_;
		head_ = nullptr;
	}
	SpanList(const SpanList&) = delete;
	SpanList& operator=(const SpanList&) = delete;

	void Lock() { mutex_.lock(); };
	void Unlock() { mutex_.unlock(); };

	bool Empty() { return head_->next_ == head_; }
	Span* Begin() { return head_->next_; };
	Span* End() { return head_; };

	void Insert(Span* cur, Span* newspan);
	void Erase(Span* cur);
	//头插
	void PushFront(Span* newspan) { Insert(Begin(), newspan); };
	//尾插
	void PushBack(Span* newspan) { Insert(End(), newspan); };
	//尾删
	Span* PopBack();
	//头删
	Span* PopFront();
};

void SpanList::Insert(Span* cur, Span* newspan)
{
	Span* prev = cur->prev_;

	prev->next_ = newspan;
	newspan->next_ = cur;

	newspan->prev_ = prev;
	cur->prev_ = newspan;
}

void SpanList::Erase(Span* cur)//此处只是单纯的把pos拿出来，并没有释放掉，后面还有用处
{
	Span* prev = cur->prev_;
	Span* next = cur->next_;
	if (next == nullptr && prev == nullptr) //Todo 等待验证
		return;
	prev->next_ = next;
	next->prev_ = prev;
}

Span* SpanList::PopBack()
{
	Span* span = head_->prev_;
	Erase(span);
	return span;
}

Span* SpanList::PopFront()//实际是将头部位置节点拿出来
{
	Span* span = head_->next_;
	Erase(span);
	return span;
}
