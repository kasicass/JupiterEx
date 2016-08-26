#include <Common/BaseList.hpp>
#include <stdio.h>

using namespace Jupiter::Common;

class MyItem : public BaseListItem<MyItem>
{
public:
	MyItem(int v) : value_(v) {}
	virtual ~MyItem() {}

	int GetValue() const { return value_; }

private:
	int value_;
};

class MyItemList : public BaseList<MyItem>
{
};

int main()
{
	MyItemList mylist;

	MyItem *p1 = new MyItem(10);
	MyItem *p2 = new MyItem(20);
	MyItem *p3 = new MyItem(30);
	MyItem *p4 = new MyItem(40);
	MyItem *p5 = new MyItem(50);

	mylist.Insert(p1);
	mylist.Insert(p2);
	mylist.InsertBefore(p2, p3);
	mylist.InsertAfter(p3, p4);
	mylist.InsertLast(p5);  // p1 p3 p4 p2 p5

	MyItem *p = mylist.GetFirst();
	for (MyItem *p = mylist.GetFirst(); p; p = p->Next())
	{
		printf("v = %d\n", p->GetValue());
	}

	return 0;
}