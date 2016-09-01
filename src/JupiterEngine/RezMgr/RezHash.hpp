#pragma once

#include "Common/BaseHash.hpp"

#define kDefaultByNameNumHashBins       19
#define kDefaultByIDNumHashBins         19
#define kDefaultDirNumHashBins          5
#define kDefaultTypNumHashBins          9

namespace JupiterEx { namespace RezMgr {

//
// RezItemHashByName / RezItemHashTableByName
//

class RezItem;
class RezItemHashTableByName;

class RezItemHashByName : public Common::BaseHashItem
{
public:
	RezItemHashByName() { rezItem_ = nullptr; }
	RezItemHashByName(RezItem *rezItem) { rezItem_ = rezItem; }
	virtual ~RezItemHashByName() {}

	void SetRezItem(RezItem *rezItem) { rezItem_ = rezItem; }
	RezItem* GetRezItem() { return rezItem_; }

	RezItemHashByName* Next() { return (RezItemHashByName*)BaseHashItem::Next(); }
	RezItemHashByName* Prev() { return (RezItemHashByName*)BaseHashItem::Prev(); }
	RezItemHashByName* NextInBin() { return (RezItemHashByName*)BaseHashItem::NextInBin(); }
	RezItemHashByName* PrevInBin() { return (RezItemHashByName*)BaseHashItem::PrevInBin(); }

protected:
	virtual unsigned int HashFunc();
	RezItemHashTableByName* GetParentTable() { return (RezItemHashTableByName*) BaseHashItem::GetParentTable(); }

private:
	friend class RezItemHashTableByName;
	RezItem *rezItem_;
};

class RezItemHashTableByName : public Common::BaseHashTable
{
public:
	RezItemHashTableByName() : BaseHashTable(1) {}
	RezItemHashTableByName(unsigned int numBins) : BaseHashTable(numBins) {}
	RezItem* Find(const char *name, bool ignoreCase = true);
	void Insert(RezItemHashByName* item) { BaseHashTable::Insert(item); }
	void Delete(RezItemHashByName* item) { BaseHashTable::Delete(item); }
	RezItemHashByName* GetFirst() { return (RezItemHashByName*)BaseHashTable::GetFirst(); }
	RezItemHashByName* GetLast() { return (RezItemHashByName*)BaseHashTable::GetLast(); }

protected:
	friend class RezItemHashByName;
	RezItemHashByName* GetFirstInBin(unsigned int bin) { return (RezItemHashByName*)BaseHashTable::GetFirstInBin(bin); }
	unsigned int HashFunc(const char* s);
};

}}