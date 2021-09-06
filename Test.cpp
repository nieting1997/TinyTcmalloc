#include <iostream>
#include "SizeClass.h"
#include "PageHeap.h"
#include "Alloc.h"

using std::cout;
using std::endl;

void TestSize()
{
	cout << SizeClass::Roundup(10) << endl;
	cout << SizeClass::Roundup(1025) << endl;
	cout << SizeClass::Roundup(1024 * 8 + 1) << endl;
	cout << SizeClass::NumMovePage(16) << endl;
	cout << SizeClass::NumMovePage(1024) << endl;
	cout << SizeClass::NumMovePage(1024 * 8) << endl; 
	cout << SizeClass::NumMovePage(1024 * 64) << endl;
}

void Alloc(size_t n)
{
	size_t begin1 = clock();
	std::vector<void*> v;
	for (size_t i = 0; i < n; ++i)
	{
		v.push_back(ConcurrentAlloc(10));
	}

	//v.push_back(ConcurrentAlloc(10));

	for (size_t i = 0; i < n; ++i)
	{
		ConcurrentFree(v[i]);
		cout << v[i] << endl;
	}
	v.clear();
	size_t end1 = clock();

	size_t begin2 = clock();
	cout << endl << endl;
	for (size_t i = 0; i < n; ++i)
	{
		v.push_back(ConcurrentAlloc(10));
	}

	for (size_t i = 0; i < n; ++i)
	{
		ConcurrentFree(v[i]);
		cout << v[i] << endl;
	}
	v.clear();
	size_t end2 = clock();

	cout << end1 - begin1 << endl;
	cout << end2 - begin2 << endl;
}

void TestThreadCache()
{
	std::thread t1(Alloc, 1000000);
	t1.join();
}

void TestCentralCache()
{
	std::vector<void*> v;
	for (size_t i = 0; i < 8; ++i)
	{
		v.push_back(ConcurrentAlloc(10));
	}

	for (size_t i = 0; i < 8; ++i)
	{
		//ConcurrentFree(v[i], 10);
		cout << v[i] << endl;
	}
}

void TestPageCache()
{
	PageHeap::GetInstence().NewSpan(2);
}

void TestConcurrentAllocFree()
{
	size_t n = 2;
	std::vector<void*> v;
	for (size_t i = 0; i < n; ++i)
	{
		void* ptr = ConcurrentAlloc(99999);
		v.push_back(ptr);
	}

	for (size_t i = 0; i < n; ++i)
	{
		ConcurrentFree(v[i]);
	}
}

void AllocBig()
{
	void* ptr1 = ConcurrentAlloc(65 << PAGE_SHIFT);
	void* ptr2 = ConcurrentAlloc(129 << PAGE_SHIFT);

	ConcurrentFree(ptr1);
	ConcurrentFree(ptr2);
}


//int main()
//{
	//AllocBig();
	//TestSize();
	//TestThreadCache();
	//TestCentralCache();
	//TestPageCache();
	//TestConcurrentAllocFree();
	//AllocBig();
	//system("pause");
	//return 0;
//}