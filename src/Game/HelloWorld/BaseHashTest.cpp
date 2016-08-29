#include "Common/BaseHash.hpp"
#include <stdio.h>

using namespace Jupiter::Common;

unsigned int g_hash = 0;
const int kBinNum = 3;

class MyItem : public BaseHashItem
{
public:
	MyItem(int v) { value_ = v; }
	virtual ~MyItem() {}

	virtual unsigned int HashFunc()
	{
		unsigned int hash = g_hash%kBinNum;
		++g_hash;
		return hash;
	}

	int GetValue() const { return value_; }

private:
	int value_;
};

void BaseHashTest()
{
	BaseHashTable table(kBinNum);

	MyItem *item1 = new MyItem(1);
	MyItem *item2 = new MyItem(2);
	MyItem *item3 = new MyItem(3);
	MyItem *item4 = new MyItem(4);

	table.Insert(item1);
	table.Insert(item2);
	table.Insert(item3);
	table.Insert(item4);

	MyItem *it = dynamic_cast<MyItem*>(table.GetFirst());
	while (it)
	{
		printf("v: %d\n", it->GetValue());
		it = dynamic_cast<MyItem*>(it->Next());
	}

	delete item1;
	delete item2;
	delete item3;
	delete item4;
}