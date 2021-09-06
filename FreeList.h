#pragma once

inline static void*& NEXT_OBJ(void* obj)
{
	return *(reinterpret_cast<void**>(obj));
}

class Freelist
{
private:
	void* head_;
	size_t size_;
	size_t maxsize_;
public:
	Freelist() : head_(nullptr), size_(0), maxsize_(1) {}

	void  Push(void* obj);
	void  PushRange(void* start, void* end, size_t n);
	void* Pop();
	void* PopRange();

	bool   Empty() { return head_ == nullptr; };
	size_t Size() const { return size_; };
	size_t MaxSize() const { return maxsize_; };
	void   SetMaxSize(size_t maxsize) { maxsize_ = maxsize; };
};

void Freelist::Push(void* obj)
{
	NEXT_OBJ(obj) = head_;
	head_ = obj;
	++size_;
}

void Freelist::PushRange(void* start, void* end, size_t n)
{
	NEXT_OBJ(end) = head_;
	head_ = start;
	size_ += n;
}

void* Freelist::Pop()
{
	void* obj = head_;
	head_ = NEXT_OBJ(obj);
	--size_;
	return obj;
}

void* Freelist::PopRange()
{
	size_ = 0;
	void* list = head_;
	head_ = nullptr;
	return list;
}