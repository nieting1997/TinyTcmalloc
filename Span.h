#pragma once
#include <mutex>

typedef size_t PageID;

//�����ڴ��ӳ��
struct Span
{
	PageID pageid_ = 0;//ҳ��
	size_t npage_ = 0;//ҳ��

	Span* prev_ = nullptr;
	Span* next_ = nullptr;
	void* list_ = nullptr;

	size_t objsize_ = 0;//����Ĵ�С
	size_t usecount_ = 0;//����ʹ�ü���,
};

//��������
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
	//ͷ��
	void PushFront(Span* newspan) { Insert(Begin(), newspan); };
	//β��
	void PushBack(Span* newspan) { Insert(End(), newspan); };
	//βɾ
	Span* PopBack();
	//ͷɾ
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

void SpanList::Erase(Span* cur)//�˴�ֻ�ǵ����İ�pos�ó�������û���ͷŵ������滹���ô�
{
	Span* prev = cur->prev_;
	Span* next = cur->next_;
	if (next == nullptr && prev == nullptr) //Todo �ȴ���֤
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

Span* SpanList::PopFront()//ʵ���ǽ�ͷ��λ�ýڵ��ó���
{
	Span* span = head_->next_;
	Erase(span);
	return span;
}
