#include "RezMgr/RezMgr.hpp"
#include <assert.h>
#include <string.h>

namespace JupiterEx { namespace RezMgr {

// -----------------------------------------------------------------------------------------
// RezItemHashByName

unsigned int RezItemHashByName::HashFunc()
{
	if (rezItem_ == nullptr) return 0;
	else return GetParentTable()->HashFunc(rezItem_->GetName());
}

// -----------------------------------------------------------------------------------------
// RezItemHashTableByName

unsigned int RezItemHashTableByName::HashFunc(const char* s)
{
	if (s == nullptr) return 0;
	unsigned int count;
	for (count = 0; *s != '\0'; ++s) { ++count; }
	assert(GetNumBins() > 0);
	return (count % GetNumBins());
}

RezItem* RezItemHashTableByName::Find(const char *name, bool ignoreCase)
{
	assert(name != nullptr);

	if (name == nullptr) return nullptr;

	RezItemHashByName* item = GetFirstInBin(HashFunc(name));
	if (ignoreCase)
	{
		while (item != nullptr)
		{
			assert(item->GetRezItem() != nullptr);
			assert(item->GetRezItem()->GetName() != nullptr);
			if (_stricmp(item->GetRezItem()->GetName(), name) == 0) return item->GetRezItem();
			item = item->NextInBin();
		}
	}
	else
	{
		while (item != nullptr)
		{
			assert(item->GetRezItem() != nullptr);
			assert(item->GetRezItem()->GetName() != nullptr);
			if (strcmp(item->GetRezItem()->GetName(), name) == 0) return item->GetRezItem();
			item = item->NextInBin();
		}
	}

	return nullptr;
}

// -----------------------------------------------------------------------------------------
// RezTypeHash

unsigned int RezTypeHash::HashFunc()
{
	assert(rezType_ != nullptr);
	return GetParentTable()->HashFunc(rezType_->GetType());
}

// -----------------------------------------------------------------------------------------
// RezTypeHashTable

unsigned int RezTypeHashTable::HashFunc(unsigned long typeId)
{
	assert(GetNumBins() > 0);
	return (typeId % GetNumBins());
}

RezType* RezTypeHashTable::Find(unsigned long typeId)
{
	RezTypeHash* item = GetFirstInBin(HashFunc(typeId));
	while (item != nullptr)
	{
		assert(item->GetRezType() != nullptr);
		if (item->GetRezType()->GetType() == typeId) return item->GetRezType();
		item = item->NextInBin();
	}
	return nullptr;
}

// -----------------------------------------------------------------------------------------
// RezDirHash

unsigned int RezDirHash::HashFunc()
{
	assert(rezDir_ != nullptr);
	return GetParentTable()->HashFunc(rezDir_->GetDirName());
}

// -----------------------------------------------------------------------------------------
// RezDirHashTable

unsigned int RezDirHashTable::HashFunc(const char* s)
{
	assert(s != nullptr);
	if (s == nullptr) return 0;

	unsigned int count;
	for (count = 0; *s != '\0'; ++s) { ++count; }
	assert(GetNumBins() > 0);
	return (count % GetNumBins());
}

RezDir* RezDirHashTable::Find(const char* dirName, bool ignoreCase)
{
	assert(dirName != nullptr);

	if (dirName == nullptr) return nullptr;

	RezDirHash* item = GetFirstInBin(HashFunc(dirName));
	if (ignoreCase)
	{
		while (item != nullptr)
		{
			assert(item->GetRezDir() != nullptr);
			assert(item->GetRezDir()->GetDirName() != nullptr);
			if (_stricmp(item->GetRezDir()->GetDirName(), dirName) == 0) return item->GetRezDir();
			item = item->NextInBin();
		}
	}
	else
	{
		while (item != nullptr)
		{
			assert(item->GetRezDir() != nullptr);
			assert(item->GetRezDir()->GetDirName() != nullptr);
			if (strcmp(item->GetRezDir()->GetDirName(), dirName) == 0) return item->GetRezDir();
			item = item->NextInBin();
		}
	}

	return nullptr;
}

}}
