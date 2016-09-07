#pragma once

#include "Common/BaseList.hpp"

namespace JupiterEx { namespace Common {

class BaseHashItem;

class BaseHashTable
{
public:
	BaseHashTable();
	BaseHashTable(unsigned int numBins);
	virtual ~BaseHashTable();

	void Insert(BaseHashItem *item);
	void Delete(BaseHashItem *item);
	BaseHashItem* GetFirst();
	BaseHashItem* GetLast();

protected:
	BaseHashItem* GetFirstInBin(unsigned int bin);
	unsigned int GetNumBins() { return numBins_; }

private:
	friend class BaseHashItem;
	typedef BaseList<BaseHashItem> HashBin;

	unsigned int numBins_;
	HashBin* binArray_;
};

class BaseHashItem : public BaseListItem<BaseHashItem>
{
public:
	BaseHashItem() {}
	virtual ~BaseHashItem() {}

	BaseHashItem* Next();
	BaseHashItem* Prev();
	BaseHashItem* NextInBin() { return BaseListItem::Next(); }
	BaseHashItem* PrevInBin() { return BaseListItem::Prev(); }

protected:
	virtual unsigned int HashFunc() = 0;

	BaseHashTable* GetParentTable() { return parentTable_; }

private:
	friend class BaseHashTable;

	BaseHashTable* parentTable_;
	unsigned int currentBin_;
};

}}