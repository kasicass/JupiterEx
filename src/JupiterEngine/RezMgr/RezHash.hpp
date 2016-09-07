#pragma once

#include "Common/BaseHash.hpp"

#define kDefaultByNameNumHashBins       19
#define kDefaultByIDNumHashBins         19
#define kDefaultDirNumHashBins          5
#define kDefaultTypNumHashBins          9

namespace JupiterEx { namespace RezMgr {

class RezItem;
class RezType;
class RezDir;
class RezItemHashTableByName;

// -----------------------------------------------------------------------------------------
// RezItemHashByName

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
	virtual unsigned int HashFunc() override;
	RezItemHashTableByName* GetParentTable() { return (RezItemHashTableByName*) BaseHashItem::GetParentTable(); }

private:
	friend class RezItemHashTableByName;
	RezItem *rezItem_;
};

// -----------------------------------------------------------------------------------------
// RezItemHashTableByName

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

// -----------------------------------------------------------------------------------------
// RezItemHashByID

class RezItemHashTableByID;

class RezItemHashByID : public Common::BaseHashItem
{
public:
	RezItemHashByID() { rezItem_ = nullptr; }
	RezItemHashByID(RezItem* item) { rezItem_ = item; }
	
	void SetRezItem(RezItem* item) { rezItem_ = item; }
	RezItem* GetRezItem() { return rezItem_; }

	RezItemHashByID* Next() { return (RezItemHashByID*)BaseHashItem::Next(); }
	RezItemHashByID* Prev() { return (RezItemHashByID*)BaseHashItem::Prev(); }
	RezItemHashByID* NextInBin() { return (RezItemHashByID*)BaseHashItem::NextInBin(); }
	RezItemHashByID* PrevInBin() { return (RezItemHashByID*)BaseHashItem::PrevInBin(); }

protected:
	virtual unsigned int HashFunc() override;
	RezItemHashTableByID* GetParenTable() { return (RezItemHashTableByID*)BaseHashItem::GetParentTable(); }

private:
	friend class RezItemHashTableByID;
	RezItem* rezItem_;
};

// -----------------------------------------------------------------------------------------
// RezItemHashTableByID

class RezItemHashTableByID : public Common::BaseHashTable
{
public:
	RezItemHashTableByID(unsigned int bins) : BaseHashTable(bins) {}
	RezItemHashTableByID() {}

	RezItem* Find(unsigned long rezId);
	void Insert(RezItemHashByID* item) { BaseHashTable::Insert(item); }
	void Delete(RezItemHashByID* item) { BaseHashTable::Delete(item); }
	RezItemHashByID* GetFirst() { return (RezItemHashByID*)BaseHashTable::GetFirst(); }
	RezItemHashByID* GetLast() { return (RezItemHashByID*)BaseHashTable::GetLast(); }

protected:
	friend class RezItemHashByID;
	RezItemHashByID* GetFirstInBin(unsigned int bin) { return (RezItemHashByID*)BaseHashTable::GetFirstInBin(bin); }
	unsigned int HashFunc(unsigned long rezId);
};

// -----------------------------------------------------------------------------------------
// RezTypeHash

class RezTypeHashTable;

class RezTypeHash : public Common::BaseHashItem
{
public:
	RezTypeHash() { rezType_ = nullptr; }
	RezTypeHash(RezType* rezType) { rezType_ = rezType; }

	void SetRezType(RezType* rezType) { rezType_ = rezType; }
	RezType* GetRezType() { return rezType_; }

	RezTypeHash* Next() { return (RezTypeHash*)BaseHashItem::Next(); }
	RezTypeHash* Prev() { return (RezTypeHash*)BaseHashItem::Prev(); }
	RezTypeHash* NextInBin() { return (RezTypeHash*)BaseHashItem::NextInBin(); }
	RezTypeHash* PrevInBin() { return (RezTypeHash*)BaseHashItem::PrevInBin(); }

protected:
	virtual unsigned int HashFunc() override;
	RezTypeHashTable* GetParentTable() { return (RezTypeHashTable*)BaseHashItem::GetParentTable(); }

private:
	friend class RezTypeHashTable;
	RezType* rezType_;
};

// -----------------------------------------------------------------------------------------
// RezTypeHashTable

class RezTypeHashTable : public Common::BaseHashTable
{
public:
	RezTypeHashTable(unsigned int bin) : BaseHashTable(bin) {}

	RezType* Find(unsigned long typeId);
	void Insert(RezTypeHash* item) { BaseHashTable::Insert(item); }
	void Delete(RezTypeHash* item) { BaseHashTable::Delete(item); }
	RezTypeHash* GetFirst() { return (RezTypeHash*)BaseHashTable::GetFirst(); }
	RezTypeHash* GetLast() { return (RezTypeHash*)BaseHashTable::GetLast(); }

protected:
	friend class RezTypeHash;
	RezTypeHash* GetFirstInBin(unsigned int bin) { return (RezTypeHash*)BaseHashTable::GetFirstInBin(bin); }
	unsigned int HashFunc(unsigned long typeId);
};


// -----------------------------------------------------------------------------------------
// RezDirHash

class RezDirHashTable;

class RezDirHash : public Common::BaseHashItem
{
public:
	RezDirHash() { rezDir_ = nullptr; }
	RezDirHash(RezDir* rezDir) { rezDir_ = rezDir; }

	void SetRezDir(RezDir* rezDir) { rezDir_ = rezDir; }
	RezDir* GetRezDir() { return rezDir_; }

	RezDirHash* Next() { return (RezDirHash*)BaseHashItem::Next(); }
	RezDirHash* Prev() { return (RezDirHash*)BaseHashItem::Prev(); }
	RezDirHash* NextInBin() { return (RezDirHash*)BaseHashItem::NextInBin(); }
	RezDirHash* PrevInBin() { return (RezDirHash*)BaseHashItem::PrevInBin(); }

protected:
	virtual unsigned int HashFunc() override;
	RezDirHashTable* GetParentTable() { return (RezDirHashTable*)BaseHashItem::GetParentTable(); }

private:
	friend class RezDirHashTable;
	RezDir* rezDir_;
};

// -----------------------------------------------------------------------------------------
// RezDirHashTable

class RezDirHashTable : public Common::BaseHashTable
{
public:
	RezDirHashTable(unsigned int numBins) : BaseHashTable(numBins) {}

	RezDir* Find(const char* dirName, bool ignoreCase = true);
	void Insert(RezDirHash* item) { BaseHashTable::Insert(item); }
	void Delete(RezDirHash* item) { BaseHashTable::Delete(item); }
	RezDirHash* GetFirst() { return (RezDirHash*)BaseHashTable::GetFirst(); }
	RezDirHash* GetLast() { return (RezDirHash*)BaseHashTable::GetLast(); }

protected:
	friend class RezDirHash;
	RezDirHash* GetFirstInBin(unsigned int bin) { return (RezDirHash*)BaseHashTable::GetFirstInBin(bin); }
	unsigned int HashFunc(const char* dirName);
};

}}