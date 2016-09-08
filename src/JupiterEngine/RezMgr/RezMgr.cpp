#include "RezMgr/RezMgr.hpp"
#include "Memory/Memory.hpp"
#include "Common/SafeString.hpp"

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <io.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>

namespace JupiterEx { namespace RezMgr {

// file format data structures
#pragma pack(1)
struct FileMainHeaderStruct
{
	char CR1;
	char LF1;
	char FileType[RezMgrUserTitleSize];
	char CR2;
	char LF2;
	char UserTitle[RezMgrUserTitleSize];
	char CR3;
	char LF3;
	char EOF1;
	unsigned long FileFormatVersion;   // the file format verison number, only 1 is possible here right now
	unsigned long RootDirPos;          // Position of the root directory struct in the file
	unsigned long RootDirSize;         // Size of root directory
	unsigned long RootDirTime;         // Time root dir was last updated
	unsigned long NextWritePos;        // Position of first directory in the file
	unsigned long Time;                // Time resource file was last updated
	unsigned long LargestKeyAry;       // Size of the largest key array in the resource file
	unsigned long LargestDirNameSize;  // Size of the largest directory name in the resource file (including 0 terminator)
	unsigned long LargestRezNameSize;  // Size of the largest resource name in the resource file (include '\0')
	unsigned long LargestCommentSize;  // Size of the largest comment in the resrouce file (include '\0')
	unsigned char IsSorted;
};

enum FileDirEntryType
{
	ResourceEntry  = 0,
	DirectoryEntry = 1,
};

struct FileDirEntryDirHeader
{
	unsigned long Pos;    // File position of dir entry
	unsigned long Size;   // Size of directory data
	unsigned long Time;   // Last time anything in directory was modified
	// char Name[];       // Name of this directory
};

struct FileDirEntryRezHeader
{
	unsigned long Pos;       // File position of dir entry
	unsigned long Size;      // Size of directory data
	unsigned long Time;      // Last time this resource was modified
	unsigned long ID;        // Resource ID number
	unsigned long Type;      // Type of resource this is
	unsigned long NumKeys;   // The number of keys to read in for this resource
	// char Name[];          // The name of this resource
	// char Comment[];       // The comment data for this resource
	// unsigned long Keys[]; // The key values for this resource
};

struct FileDirEntryHeader
{
	unsigned long Type;
	union
	{
		FileDirEntryRezHeader Rez;
		FileDirEntryDirHeader Dir;
	};
};
#pragma pack()

//------------------------------------------------------------------------------------------
// RezItem

RezItem::RezItem()
{
	rezFile_   = nullptr;
	parentDir_ = nullptr;
	name_      = nullptr;
	hashByName_.SetRezItem(this);
}

void RezItem::InitRezItem(RezDir* parentDir, const char* name, unsigned long id, RezType* type, const char* desc,
					unsigned long size, unsigned long filePos, unsigned long time, unsigned long numKeys,
					unsigned long* keyArray, BaseRezFile* rezFile)
{
	assert(parentDir != nullptr);

	rezFile_   = rezFile;
	parentDir_ = parentDir;

	if (name == nullptr)
	{
		name_ = nullptr;
	}
	else
	{
		LT_MEM_TRACK_ALLOC(name_ = new char[strlen(name)+1], LT_MEM_TYPE_MISC);
		assert(name_ != nullptr);
		if (name_ != nullptr) strcpy(name_, name);
	}

	type_ = type;

	size_ = size;
	filePos_ = filePos;
	time_ = time;

	data_ = nullptr;
	currPos_ = 0;

	hashByName_.SetRezItem(this);
}

void RezItem::TermRezItem()
{
	if (name_ != nullptr) delete [] name_;

	if (parentDir_ != nullptr)
	{
		if (parentDir_->memBlock_ == nullptr)
		{
			if (data_ != nullptr)
			{
				delete [] data_;
			}
		}
	}
	else if (data_ != nullptr)
	{
		delete [] data_;
	}

	name_ = nullptr;
	type_ = nullptr;

	time_ = 0;
	size_ = 0;
	data_ = nullptr;

	parentDir_ = nullptr;
	filePos_ = 0;
	currPos_ = 0;

	hashByName_.SetRezItem(nullptr);
}

unsigned long RezItem::GetType()
{
	assert(type_ != nullptr);
	return type_->GetType();
}

const char* RezItem::GetPath(char* buf, unsigned long bufSize)
{
	// Note! This is really slow, could be speed up!
	assert(buf != nullptr);
	assert(bufSize > 0);

	// if this is in the root directory just return that
	if (parentDir_->parentDir_ == nullptr)
	{
		LTStrCpy(buf, "\\", bufSize);
	}
	else
	{
		char* tempBuf;
		LT_MEM_TRACK_ALLOC(tempBuf = new char[bufSize], LT_MEM_TYPE_MISC);
		assert(tempBuf != nullptr);
		strcpy(buf, "");

		// loop through all directories appending them to the path
		assert(parentDir_ != nullptr);
		RezDir* dir = parentDir_;
		while (dir != nullptr)
		{
			assert((strlen(dir->GetDirName()) + strlen(buf) + 1) < bufSize);
			strcpy(tempBuf, buf);
			if (dir->parentDir_ != nullptr) strcpy(buf, "\\");
			else buf[0] = '\0';
			strcat(buf, dir->GetDirName());
			strcat(buf, tempBuf);
			dir = dir->parentDir_;
		}

		LT_MEM_TRACK_FREE(delete [] tempBuf);
	}

	return buf;
}

const char* RezItem::GetDir()
{
	assert(parentDir_ != nullptr);
	return parentDir_->GetDirName();
}

unsigned char* RezItem::Load()
{
	assert(parentDir_ != nullptr);
	assert(rezFile_ != nullptr);

	// check if the whole directory is in memory already
	if (parentDir_->memBlock_ != nullptr)
	{
		return parentDir_->memBlock_ + filePos_ - parentDir_->itemsPos_;
	}

	// check if the data is already in memory
	if (data_ != nullptr) return data_;

	// allocate memory for the data
	if (size_ == 0) return nullptr;
	LT_MEM_TRACK_ALLOC(data_ = new unsigned char[size_], LT_MEM_TYPE_MISC);
	assert(data_ != nullptr);
	if (data_ == nullptr) return nullptr;

	// load in the data from disk
	assert(parentDir_->rezMgr_ != nullptr);
	if (rezFile_->Read(filePos_, 0, size_, data_) != size_)
	{
		delete [] data_;
		data_ = nullptr;
	}

	return data_;
}

bool RezItem::UnLoad()
{
	if (data_ != nullptr)
	{
		delete [] data_;
		data_ = nullptr;
	}
	return true;
}

bool RezItem::IsLoaded()
{
	assert(parentDir_ != nullptr);
	if (parentDir_->memBlock_ != nullptr) return true;
	else return (data_ != nullptr);
}

bool RezItem::Get(unsigned char* bytes)
{
	return (Get(bytes, 0, size_));
}

bool RezItem::Get(unsigned char* bytes, unsigned long startOffset, unsigned long length)
{
	assert(parentDir_ != nullptr);
	assert(rezFile_ != nullptr);
	assert(bytes != nullptr);
	assert(length > 0);
	assert(length <= size_ - startOffset);

	// Check if the whole directory is in memory already and just copy it if it is
	if (parentDir_->memBlock_ != nullptr)
	{
		memcpy(bytes, parentDir_->memBlock_ + filePos_ + startOffset - parentDir_->itemsPos_, length);
		return true;
	}

	// Check if this resource is in memory already and just copy it if so
	if (data_ != nullptr)
	{
		memcpy(bytes, data_ + startOffset, length);
		return true;
	}

	// Load this part of the resource from disk
	assert(parentDir_->rezMgr_ != nullptr);
	if (rezFile_->Read(filePos_, startOffset, length, bytes) != length)
	{
		return false;
	}

	return true;
}

bool RezItem::Seek(unsigned long offset)
{
	currPos_ = offset;
	return true;
}

unsigned long RezItem::Read(unsigned char* bytes, unsigned long length, unsigned long seekPos)
{
	assert(parentDir_ != nullptr);
	assert(bytes != nullptr);
	assert(rezFile_ != nullptr);

	// do seek if necessary
	if (seekPos != REZ_SEEKPOS_ERROR) Seek(seekPos);

	// check if we are already past the end of the file
	if (currPos_ > size_) return 0;

	// truncate length if necessary
	if ((length + currPos_) > size_) length = size_ - currPos_;

	// if length is zero just return
	if (length <= 0) return 0;

	// Check if the whole directory is in memory already and just copy it if it is
	if (parentDir_->memBlock_ != nullptr)
	{
		memcpy(bytes, parentDir_->memBlock_ + filePos_ + currPos_ - parentDir_->itemsPos_, length);
		currPos_ += length;
		return length;
	}

	// Check if this resource is in memory already and just copy it if so
	if (data_ != nullptr)
	{
		memcpy(bytes, data_ + currPos_, length);
		currPos_ += length;
		return length;
	}

	// Load from disk
	assert(parentDir_->rezMgr_ != nullptr);
	if (rezFile_->Read(filePos_, currPos_, length, bytes) == length)
	{
		currPos_ += length;
		return length;
	}

	return 0;
}

bool RezItem::EndOfRes()
{
	return (currPos_ >= size_);
}

char RezItem::GetChar()
{
	// This could be way more efficent!!!
	char ch;
	Read((unsigned char*)&ch, 1);
	return ch;
}

unsigned char* RezItem::Create(unsigned long size)
{
	assert(parentDir_ != nullptr);
	assert(parentDir_->rezMgr_ != nullptr);
	assert(parentDir_->rezMgr_->readOnly_ != true);

	unsigned long oldSize = size_;

	UnLoad();

	// make sure parent does not have resources in memory (if so remove them)
	assert(parentDir_ != nullptr);
	if (parentDir_->memBlock_ != nullptr) parentDir_->UnLoad();

	// allocate the new memory and set the new size member
	size_ = size;
	LT_MEM_TRACK_ALLOC(data_ = new unsigned char[size_], LT_MEM_TYPE_MISC);
	assert(data_ != nullptr);

	// mark this resource as not exiting in the resource file yet
	filePos_ = 0;

	// update the directory items size in the parent directory
	parentDir_->itemsSize_ += size_;
	parentDir_->itemsSize_ -= oldSize;

	// make data as not sorted
	parentDir_->rezMgr_->isSorted_ = false;

	MarkCurTime();

	return data_;
}

bool RezItem::Save()
{
	assert(data_ != nullptr);
	assert(rezFile_ != nullptr);
	assert(parentDir_ != nullptr);
	assert(parentDir_->rezMgr_ != nullptr);
	assert(parentDir_->rezMgr_->readOnly_ != true);

	if (size_ <= 0) return true;

	// if this resource has never been saved
	if (filePos_ == 0)
	{
		// mark resource file as not sorted
		parentDir_->rezMgr_->isSorted_ = false;

		// write out the data
		if (rezFile_->Write(parentDir_->rezMgr_->nextWritePos_, 0, size_, data_) != size_)
		{
			assert(false);
			return false;
		}

		filePos_ = parentDir_->rezMgr_->nextWritePos_;
		parentDir_->rezMgr_->nextWritePos_ += size_;
	}
	else
	{
		// write out the data
		if (rezFile_->Write(filePos_, 0, size_, data_) != size_)
		{
			assert(false);
			return false;
		}
	}

	MarkCurTime();

	return true;
}

void RezItem::MarkCurTime()
{
	assert(parentDir_ != nullptr);
	assert(parentDir_->rezMgr_ != nullptr);
	assert(parentDir_->rezMgr_->readOnly_ != true);

	unsigned long time = parentDir_->rezMgr_->GetCurTime();
	time_ = time;
	parentDir_->lastTimeModified_ = time;
	parentDir_->rezMgr_->lastTimeModified_ = time;
}

//------------------------------------------------------------------------------------------
// RezType

RezType::RezType(unsigned long typeId, RezDir* parentDir, unsigned int nByIDNumHashBins, unsigned int nByNameNumHashBins) :
	hashTableByID_(nByIDNumHashBins),
	hashTableByName_(nByNameNumHashBins)
{
	typeId_ = typeId;
	hashElementType_.SetRezType(this);
	parentDir_ = parentDir;
}

RezType::RezType(unsigned long typeId, RezDir* parentDir, unsigned int nByNameNumHashBins) :
	hashTableByName_(nByNameNumHashBins)
{
	typeId_ = typeId;
	hashElementType_.SetRezType(this);
	parentDir_ = parentDir;
}

RezType::~RezType()
{
	// remove all of the items in the ByID hash table
	// Don't delete objects here, they are deleted in the ByName table below!

	// no use ??? - kasicass
	if (parentDir_->rezMgr_->itemByIDUsed_)
	{
		RezItemHashByID* item = hashTableByID_.GetFirst();
		RezItemHashByID* toDel = nullptr;
		while (item != nullptr)
		{
			toDel = item;
			item = item->Next();
			hashTableByID_.Delete(toDel);
		}
	}

	// remove all of the items in the ByName hash table and delete the objects
	{
		RezItemHashByName* item = hashTableByName_.GetFirst();
		RezItemHashByName* toDel = nullptr;
		while (item != nullptr)
		{
			toDel = item;
			item = item->Next();
			hashTableByName_.Delete(toDel);
			assert(toDel->GetRezItem() != nullptr);
			toDel->GetRezItem()->TermRezItem();
			parentDir_->rezMgr_->DeAllocateRezItem(toDel->GetRezItem());
		}
	}

	typeId_ = 0;
	hashElementType_.SetRezType(nullptr);
}

//------------------------------------------------------------------------------------------
// RezDir

RezDir::RezDir(RezMgr* rezMgr, RezDir* parentDir, const char* dirName, unsigned long dirPos,
		unsigned long dirSize, unsigned long time, unsigned int nDirNumHashBins, unsigned int nTypeNumHashBins) :
	hashTableSubDirs_(nDirNumHashBins),
	hashTableTypes_(nTypeNumHashBins)
{
	assert(rezMgr_ != nullptr);
	assert(dirName_ != nullptr);

	LT_MEM_TRACK_ALLOC(dirName_ = new char[strlen(dirName)+1], LT_MEM_TYPE_MISC);
	assert(dirName_ != nullptr);
	if (dirName_ != nullptr) strcpy(dirName_, dirName);

	lastTimeModified_ = time;
	dirSize_          = dirSize;
	dirPos_           = dirPos;
	itemsSize_        = 0;
	itemsPos_         = 0;
	memBlock_         = nullptr;
	rezMgr_           = rezMgr;
	parentDir_        = parentDir;
	hashElementDir_.SetRezDir(this);
}

RezDir::~RezDir()
{
	{
		RezTypeHash *item = hashTableTypes_.GetFirst();
		RezTypeHash *toDel;
		while (item != nullptr)
		{
			toDel = item;
			item  = item->Next();
			hashTableTypes_.Delete(toDel);
			assert(toDel->GetRezType() != nullptr);
			delete toDel->GetRezType();
		}
	}

	{
		RezDirHash *item = hashTableSubDirs_.GetFirst();
		RezDirHash *toDel;
		while (item != nullptr)
		{
			toDel = item;
			item = item->Next();
			hashTableSubDirs_.Delete(toDel);
			assert(toDel->GetRezDir() != nullptr);
			delete toDel->GetRezDir();
		}
	}

	if (dirName_ != nullptr) delete [] dirName_;
	if (memBlock_ != nullptr) delete [] memBlock_;

	dirName_ = nullptr;
	lastTimeModified_ = 0;
	dirSize_ = 0;
	dirPos_ = 0;
	itemsSize_ = 0;
	itemsPos_ = 0;
	memBlock_ = nullptr;
	rezMgr_ = nullptr;
	parentDir_ = nullptr;
	hashElementDir_.SetRezDir(nullptr);
}

RezItem* RezDir::GetRez(const char* rezName, unsigned long rezTypeId)
{
	assert(rezName != nullptr);

	RezType *rezType = hashTableTypes_.Find(rezTypeId);
	if (rezType == nullptr) return nullptr;

	return rezType->hashTableByName_.Find(rezName, !GetParentMgr()->GetLowerCasedUsed());
}

RezItem* RezDir::GetRezFromDosName(const char* rezNameDOS)
{
	assert(rezNameDOS != nullptr);

	char sExt[5];
	unsigned long rezTypeId;

	char drive[_MAX_DRIVE+1];
	char dir[_MAX_DIR+1];
	char fname[_MAX_FNAME+1+_MAX_EXT+1];
	char ext[_MAX_EXT+1];
	_splitpath(rezNameDOS, drive, dir, fname, ext);

	bool extensionTooLong = false;
	if (strlen(ext) > 5)
	{
		extensionTooLong = true;
		strncat(fname, ext, _MAX_FNAME+1+_MAX_EXT);
		fname[_MAX_FNAME] = '\0';
		rezTypeId = 0;
	}
	else
	{
		if (strlen(ext) > 0)
		{
			strcpy(sExt, &ext[1]);
			_strupr(sExt);
			rezTypeId = rezMgr_->StrToType(sExt);
		}
		else
		{
			rezTypeId = 0;
		}
	}

	return GetRez(fname, rezTypeId);
}

bool RezDir::Load(bool loadAllSubDirs)
{
	if (memBlock_ != nullptr) return true;

	// if the file is not sorted then we cannot load it by directory (actually we could but it would take more effort)
	// we also can't do this if we have loaded more than 1 rez file
	if ((rezMgr_->isSorted_ == false) || (rezMgr_->numRezFiles_ > 1))
	{
		return false;
	}

	// if the data size is 0 then we don't need to do anything
	if (itemsSize_ > 0)
	{
		LT_MEM_TRACK_ALLOC(memBlock_ = new unsigned char[itemsSize_], LT_MEM_TYPE_MISC);
		assert(memBlock_ != nullptr);
		if (memBlock_ != nullptr)
		{
			assert(rezMgr_ != nullptr);
			assert(itemsPos_ > 0);
			rezMgr_->primaryRezFile_->Read(itemsPos_, 0, itemsSize_, memBlock_);
		}
	}

	if (loadAllSubDirs)
	{
		RezDirHash *item = hashTableSubDirs_.GetFirst();
		while (item != nullptr)
		{
			assert(item->GetRezDir() != nullptr);
			item->GetRezDir()->Load(true);
			item = item->Next();
		}
	}

	return true;
}

bool RezDir::UnLoad(bool unLoadAllSubDirs)
{
	if (memBlock_ != nullptr)
	{
		delete [] memBlock_;
		memBlock_ = nullptr;
	}
	else
	{
		// if the memory is not stored globally then unload all items individually
		RezType *rezType = GetFirstType();
		while (rezType != nullptr)
		{
			RezItem *rezItem = GetFirstItem(rezType);
			while (rezItem != nullptr)
			{
				rezItem->UnLoad();
				rezItem = GetNextItem(rezItem);
			}
			rezType = GetNextType(rezType);
		}
	}

	if (unLoadAllSubDirs)
	{
		RezDirHash *item = hashTableSubDirs_.GetFirst();
		while (item != nullptr)
		{
			assert(item->GetRezDir() != nullptr);
			item->GetRezDir()->UnLoad(true);
			item = item->Next();
		}
	}

	return true;
}

RezDir* RezDir::GetDir(const char* dirName)
{
	assert(dirName != nullptr);
	if (dirName == nullptr) return nullptr;
	return hashTableSubDirs_.Find(dirName, !GetParentMgr()->GetLowerCasedUsed());
}

RezDir* RezDir::GetFirstSubDir()
{
	RezDirHash *item = hashTableSubDirs_.GetFirst();
	if (item == nullptr) return nullptr;
	assert(item->GetRezDir() != nullptr);
	return item->GetRezDir();
}

RezDir* RezDir::GetNextSubDir(RezDir *rezDir)
{
	assert(rezDir != nullptr);
	RezDirHash *item = rezDir->hashElementDir_.Next();
	if (item == nullptr) return nullptr;
	assert(item->GetRezDir() != nullptr);
	return item->GetRezDir();
}

RezType* RezDir::GetRezType(unsigned long rezTypeId)
{
	return hashTableTypes_.Find(rezTypeId);
}

RezType* RezDir::GetFirstType()
{
	RezTypeHash *item = hashTableTypes_.GetFirst();
	if (item == nullptr) return nullptr;
	assert(item->GetRezType() != nullptr);
	return item->GetRezType();
}

RezType* RezDir::GetNextType(RezType *rezType)
{
	assert(rezType != nullptr);
	RezTypeHash *item = rezType->hashElementType_.Next();
	if (item == nullptr) return nullptr;
	assert(item->GetRezType() != nullptr);
	return item->GetRezType();
}

RezItem* RezDir::GetFirstItem(RezType* rezType)
{
	assert(rezType != nullptr);
	RezItemHashByName *item = rezType->hashTableByName_.GetFirst();
	if (item == nullptr) return nullptr;
	return item->GetRezItem();
}

RezItem* RezDir::GetNextItem(RezItem *rezItem)
{
	assert(rezItem != nullptr);
	RezItemHashByName *item = rezItem->hashByName_.Next();
	if (item == nullptr) return nullptr;
	return item->GetRezItem();
}

RezDir* RezDir::CreateDir(const char* dirName)
{
	assert(dirName != nullptr);
	assert(rezMgr_ != nullptr);

	// make sure directory does not already exist
	RezDir* dir = hashTableSubDirs_.Find(dirName, !GetParentMgr()->GetLowerCasedUsed());
	assert(dir == nullptr);
	if (dir != nullptr) return nullptr;

	LT_MEM_TRACK_ALLOC(dir = new RezDir(rezMgr_, this, dirName, 0, 0, rezMgr_->GetCurTime(), rezMgr_->dirNumHashBins_, rezMgr_->typeNumHashBins_), LT_MEM_TYPE_MISC);
	assert(dir != nullptr);
	if (dir == nullptr) return nullptr;

	hashTableSubDirs_.Insert(&dir->hashElementDir_);

	unsigned long nameLength = strlen(dirName);
	if (rezMgr_->largestDirNameSize_ <= nameLength) rezMgr_->largestDirNameSize_ = nameLength+1;

	return dir;
}

RezItem* RezDir::CreateRez(unsigned long rezId, const char* rezName, unsigned long rezTypeId)
{
	assert(rezName != nullptr);
	assert(rezMgr_ != nullptr);
	assert(rezMgr_->readOnly_ == false);

	RezType* type = GetOrMakeType(rezTypeId);
	assert(type != nullptr);

	RezItem* item = nullptr;
	item = type->hashTableByName_.Find(rezName, !GetParentMgr()->GetLowerCasedUsed());
	if (item != nullptr) return nullptr;

	item = rezMgr_->AllocateRezItem();
	assert(item != nullptr);
	if (item == nullptr) return nullptr;

	item->InitRezItem(this, rezName, rezTypeId, type, nullptr, 0, 0, rezMgr_->GetCurTime(), 0, nullptr, rezMgr_->primaryRezFile_);
	type->hashTableByName_.Insert(&item->hashByName_);

	unsigned long nameLength = strlen(rezName);
	if (rezMgr_->largestRezNameSize_ <= nameLength) rezMgr_->largestRezNameSize_ = nameLength+1;

	return item;
}

RezItem* RezDir::CreateRezInternal(unsigned long rezId, const char* rezName, RezType* rezType, BaseRezFile* rezFile)
{
	assert(rezName != nullptr);
	assert(rezMgr_ != nullptr);

	RezItem* item = rezMgr_->AllocateRezItem();
	assert(item != nullptr);
	if (item == nullptr) return nullptr;

	item->InitRezItem(this, rezName, rezId, rezType, nullptr, 0, 0, rezMgr_->GetCurTime(), 0, nullptr, rezFile);
	rezType->hashTableByName_.Insert(&item->hashByName_);

	unsigned long nameLength = strlen(rezName);
	if (rezMgr_->largestRezNameSize_ <= nameLength) rezMgr_->largestRezNameSize_ = nameLength+1;

	return item;
}

bool RezDir::RemoveRezInternal(RezType* rezType, RezItem* rezItem)
{
	assert(rezItem != nullptr);

	// update the directory items size
	itemsSize_ -= rezItem->size_;

	// remove from hash tables
	rezType->hashTableByName_.Delete(&rezItem->hashByName_);

	// delete item
	rezItem->TermRezItem();
	rezMgr_->DeAllocateRezItem(rezItem);

	// mark data as not sorted
	rezMgr_->isSorted_ = false;

	return true;
}

bool RezDir::ReadAllDirs(BaseRezFile* rezFile, unsigned long pos, unsigned long size, bool overwriteItems)
{
	assert(pos > 0);

	// return if this is an empty directory
	if (size <= 0) return true;

	// clear out the dirPos_ variables in any currently existing directories so
	// we don't try to read a dir we shouldn't
	{
		RezDirHash* it = hashTableSubDirs_.GetFirst();
		while (it != nullptr)
		{
			RezDir* rezDir = it->GetRezDir();
			assert(rezDir != nullptr);
			rezDir->dirPos_ = 0;
			it = it->Next();
		}
	}

	// read in this directory
	bool retFlag = true;
	if (ReadDirBlock(rezFile, pos, size, overwriteItems))
	{
		RezDirHash* it = hashTableSubDirs_.GetFirst();
		while (it != nullptr)
		{
			RezDir* rezDir = it->GetRezDir();
			assert(rezDir != nullptr);
			if (rezDir->dirPos_ != 0)
			{
				if (!rezDir->ReadAllDirs(rezFile, rezDir->dirPos_, rezDir->dirSize_, overwriteItems))
				{
					retFlag = false;
				}
			}
			it = it->Next();
		}
	}
	else
	{
		retFlag = false;
	}

	return retFlag;
}

bool RezDir::ReadDirBlock(BaseRezFile* rezFile, unsigned long pos, unsigned long size, bool overwriteItems)
{
	assert(pos > 0);

	itemsSize_ = 0;
	itemsPos_  = REZ_SEEKPOS_ERROR;
	unsigned long lastItemPos = 0;
	unsigned long lastItemSize = 0;

	unsigned char* buf;
	LT_MEM_TRACK_ALLOC(buf = new unsigned char[size], LT_MEM_TYPE_MISC);
	assert(buf != nullptr);
	if (buf == nullptr) return false;

	if (rezFile->Read(pos, 0, size, buf) != size)
	{
		LT_MEM_TRACK_FREE(delete [] buf);
		return false;
	}

	// process all data in directory block
	unsigned char* curr = buf;
	unsigned char* end  = buf + size;
	while (curr < end)
	{
		if ((*(unsigned long*)curr) == DirectoryEntry)
		{
			curr += sizeof(unsigned long);

			// variables to store data read in
			unsigned long pos;
			unsigned long size;
			unsigned long time;
			char* dirName;

			pos  = (*(unsigned long*)curr); curr += sizeof(unsigned long);
			size = (*(unsigned long*)curr); curr += sizeof(unsigned long);
			time = (*(unsigned long*)curr); curr += sizeof(unsigned long);

			dirName = (char*)buf;
			curr += strlen(dirName)+1;

			// make sure this dir doesn't already exist if it does we don't need to add it again
			RezDir* rezDir = hashTableSubDirs_.Find(dirName, !GetParentMgr()->GetLowerCasedUsed());
			if (rezDir == nullptr)
			{
				LT_MEM_TRACK_ALLOC(rezDir = new RezDir(rezMgr_, this, dirName, pos, size, time, rezMgr_->dirNumHashBins_, rezMgr_->typeNumHashBins_), LT_MEM_TYPE_MISC);
				assert(rezDir != nullptr);

				hashTableSubDirs_.Insert(&rezDir->hashElementDir_);
			}
			else
			{
				rezDir->dirPos_ = pos;
				rezDir->dirSize_ = size;
				rezDir->lastTimeModified_ = time;
			}
		}
		else
		{
			// a resource item entry
			assert((*(unsigned long*)curr) == ResourceEntry);
			curr += sizeof(unsigned long);

			unsigned long pos;
			unsigned long size;
			unsigned long time;
			unsigned long id;
			unsigned long rezTypeId;
			unsigned long numKeys;
			char* rezName;
			char* rezDesc;
			unsigned long* keyArray;

			pos       = (*(unsigned long*)curr); curr += sizeof(unsigned long);
			size      = (*(unsigned long*)curr); curr += sizeof(unsigned long);
			time      = (*(unsigned long*)curr); curr += sizeof(unsigned long);
			id        = (*(unsigned long*)curr); curr += sizeof(unsigned long);
			rezTypeId = (*(unsigned long*)curr); curr += sizeof(unsigned long);
			numKeys   = (*(unsigned long*)curr); curr += sizeof(unsigned long);

			rezName = (char*)curr;
			curr += strlen(rezName)+1;

			RezType* rezType = GetOrMakeType(rezTypeId);
			assert(rezType != nullptr);

			bool skipThisItem = false;
			RezItem* dupNameItem = rezType->hashTableByName_.Find(rezName);
			if (dupNameItem != nullptr)
			{
				if (overwriteItems)
				{
					RemoveRezInternal(rezType, dupNameItem);
				}
				else
				{
					skipThisItem = true;
				}
			}

			rezDesc = (char*)curr;
			curr += strlen(rezDesc)+1;
			if (rezDesc[0] == '\0') rezDesc = nullptr;

			if (numKeys > 0)
			{
				LT_MEM_TRACK_ALLOC(keyArray = new unsigned long[numKeys], LT_MEM_TYPE_MISC);
				assert(keyArray != nullptr);
				for (unsigned int i = 0; i < numKeys; ++i)
				{
					keyArray[i] = *(unsigned long*)curr;
					curr += sizeof(unsigned long);
				}
			}
			else
			{
				keyArray = nullptr;
			}

			if (!skipThisItem)
			{
				RezItem *rezItem = rezMgr_->AllocateRezItem();
				assert(rezItem != nullptr);
				rezItem->InitRezItem(this, rezName, id, rezType, rezDesc, size, pos, time, numKeys, keyArray, rezFile);
			
				rezType->hashTableByName_.Insert(&rezItem->hashByName_);

				itemsSize_ += rezItem->size_;
				if (rezItem->filePos_ < itemsPos_) itemsPos_ = rezItem->filePos_;
				if (rezItem->filePos_ > lastItemPos)
				{
					lastItemPos  = rezItem->filePos_;
					lastItemSize = rezItem->size_;
				}
			}

			if (keyArray != nullptr)
			{
				delete [] keyArray;
			}
		}
	}

	delete [] buf;
	return true;
}

RezType* RezDir::GetOrMakeType(unsigned long rezTypeId)
{
	RezType* rezType = hashTableTypes_.Find(rezTypeId);
	if (rezType == nullptr)
	{
		if (rezMgr_->itemByIDUsed_)
		{
			LT_MEM_TRACK_ALLOC(rezType = new RezType(rezTypeId, this, rezMgr_->byIDNumHashBins_, rezMgr_->byNameNumHashBins_), LT_MEM_TYPE_MISC);
		}
		else
		{
			LT_MEM_TRACK_ALLOC(rezType = new RezType(rezTypeId, this, rezMgr_->byNameNumHashBins_), LT_MEM_TYPE_MISC);
		}
		assert(rezType != nullptr);
		if (rezType == nullptr) return nullptr;

		hashTableTypes_.Insert(&rezType->hashElementType_);
	}

	return rezType;
}

//------------------------------------------------------------------------------------------
// RezMgr

RezMgr::RezMgr()
{
	fileOpened_ = false;
	renumberIDCollisions_ = true;
	nextIDNumToUse_ = 2000000000;
	primaryRezFile_ = nullptr;
	numRezFiles_ = 0;
	rootDirPos_ = 0;
	nextWritePos_ = 0;
	readOnly_ = true;
	rootDir_ = nullptr;
	lastTimeModified_ = 0;
	mustReWriteDirs_ = false;
	fileFormatVersion_ = 0;
	largestKeyArray_ = 0;
	largestDirNameSize_ = 0;
	largestRezNameSize_ = 0;
	largestCommentSize_ = 0;
	isSorted_ = 0;
	filename_ = nullptr;
	maxOpenFilesInEmulatedDir_ = 3;
	dirSeparators_ = nullptr;
	lowerCaseUsed_ = false;
	itemByIDUsed_ = false;
	byNameNumHashBins_ = kDefaultByNameNumHashBins;
	byIDNumHashBins_   = kDefaultByIDNumHashBins;
	dirNumHashBins_    = kDefaultDirNumHashBins;
	typeNumHashBins_   = kDefaultTypNumHashBins;
	rezItemChunkSize_  = 100;
	userTitle_[0]      = '\0';
}

RezMgr::RezMgr(const char* filename, bool readOnly, bool createNew)
{
	RezMgr();
	Open(filename, readOnly, createNew);
}

RezMgr::~RezMgr()
{
	if (fileOpened_) Close();

	BaseRezFile* rezFile;
	while ((rezFile = rezFilesList_.GetFirst()) != nullptr)
	{
		rezFilesList_.Delete(rezFile);
		--numRezFiles_;
		delete rezFile;
	}

	if (rootDir_ != nullptr)
	{
		delete rootDir_;
		rootDir_ = nullptr;
	}

	if (filename_ != nullptr)
	{
		delete [] filename_;
		filename_ = nullptr;
	}

	if (dirSeparators_ != nullptr)
	{
		delete [] dirSeparators_;
		dirSeparators_;
	}

	fileOpened_ = false;
	primaryRezFile_ = nullptr;
	rootDirPos_ = 0;
	rootDirSize_ = 0;
	rootDirTime_ = 0;
	nextWritePos_ = 0;
	readOnly_ = true;
	rootDir_ = nullptr;
	lastTimeModified_ = 0;
	mustReWriteDirs_  = false;
	fileFormatVersion_ = 1;
	largestKeyArray_ = 0;
	largestDirNameSize_ = 0;
	largestRezNameSize_ = 0;
	largestCommentSize_ = 0;
	isSorted_ = true;
	filename_ = nullptr;

	// remove all RezItem Chunks
	RezItemChunk* chunk;
	while ((chunk = rezItemChunksList_.GetFirst()) != nullptr)
	{
		delete [] chunk->rezItemArray_;
		rezItemChunksList_.Delete(chunk);
		delete chunk;
	}
}

bool RezMgr::Open(const char* filename, bool readOnly, bool createNew)
{
	assert(filename != nullptr);
	assert(!(readOnly && createNew));
	readOnly_ = readOnly;

	if (filename_ != nullptr) delete [] filename_;
	LT_MEM_TRACK_ALLOC(filename_ = new char[strlen(filename)+1], LT_MEM_TYPE_MISC);
	assert(filename_ != nullptr);
	strcpy(filename_, filename);

	// check if user has asked us to open a directory instead of a rez file
	if (IsDirectory(filename))
	{
		assert(readOnly_ != false);
		if (readOnly_ == false) return false;

		RezFileDirectoryEmulation* rezFile;
		LT_MEM_TRACK_ALLOC(rezFile = new RezFileDirectoryEmulation(this, maxOpenFilesInEmulatedDir_), LT_MEM_TYPE_MISC);
		assert(rezFile != nullptr);
		if (rezFile == nullptr)
		{
			delete [] filename_;
			filename_ = nullptr;
			return false;
		}
		primaryRezFile_ = rezFile;
		rezFilesList_.Insert(rezFile);
		++numRezFiles_;

		if (!rezFile->Open(filename, readOnly, createNew)) return false;
		fileOpened_ = true;

		LT_MEM_TRACK_ALLOC(rootDir_ = new RezDir(this, nullptr, "", 0, 0, GetCurTime(), dirNumHashBins_, typeNumHashBins_), LT_MEM_TYPE_MISC);
		assert(rootDir_ != nullptr);

		// read in data from directory and all sub directories
		ReadEmulationDirectory(rezFile, rootDir_, filename_, false);
		return true;
	}

	// create a new RezFile object
	RezFile* rezFile;
	LT_MEM_TRACK_ALLOC(rezFile = new RezFile(this), LT_MEM_TYPE_MISC);
	assert(rezFile != nullptr);
	if (rezFile == nullptr)
	{
		delete [] filename_;
		filename_ = nullptr;
		return false;
	}
	primaryRezFile_ = rezFile;
	rezFilesList_.Insert(rezFile);
	++numRezFiles_;

	if (!rezFile->Open(filename, readOnly, createNew)) return false;
	fileOpened_ = true;

	if (createNew)
	{
		nextWritePos_ = sizeof(FileMainHeaderStruct);
		mustReWriteDirs_ = true;

		LT_MEM_TRACK_ALLOC(rootDir_ = new RezDir(this, nullptr, "", 0, 0, GetCurTime(), dirNumHashBins_, typeNumHashBins_), LT_MEM_TYPE_MISC);
		assert(rootDir_ != nullptr);
	}
	else
	{
		FileMainHeaderStruct header;
		rezFile->Read(0, 0, sizeof(header), &header);

		nextWritePos_        = header.NextWritePos;
		rootDirPos_          = header.RootDirPos;
		rootDirSize_         = header.RootDirSize;
		rootDirTime_         = header.RootDirTime;
		lastTimeModified_    = header.Time;
		fileFormatVersion_   = header.FileFormatVersion;
		largestKeyArray_     = header.LargestKeyAry;
		largestDirNameSize_  = header.LargestDirNameSize;
		largestRezNameSize_  = header.LargestRezNameSize;
		largestCommentSize_  = header.LargestCommentSize;
		isSorted_            = !!header.IsSorted;

		memcpy(userTitle_, header.UserTitle, RezMgrUserTitleSize);
		userTitle_[RezMgrUserTitleSize] = '\0';
		for (int i = RezMgrUserTitleSize-1; i >= 0; --i)
		{
			if (userTitle_[i] != ' ') break;
			userTitle_[i] = '\0';
		}

		assert(header.CR1 == 0x0d);
		assert(header.LF2 == 0x0a);
		assert(header.EOF1 == 0x1a);
		if (header.CR1 != 0x0d) return false;
		if (header.LF2 != 0x0a) return false;
		if (header.EOF1 != 0x1a) return false;
		if (fileFormatVersion_ != 1) return false;

		LT_MEM_TRACK_ALLOC(rootDir_ = new RezDir(this, nullptr, "", rootDirPos_, rootDirSize_, rootDirTime_, dirNumHashBins_, typeNumHashBins_), LT_MEM_TYPE_MISC);
		assert(rootDir_ != nullptr);

		rootDir_->ReadAllDirs(rezFile, rootDirPos_, rootDirSize_, false);
	}

	return true;
}

bool RezMgr::OpenAdditional(const char* filename, bool overwriteItems)
{
	assert(filename != nullptr);
	assert(rootDir_ != nullptr);
	assert(readOnly_ == true);
	bool readOnly = true;
	bool createNew = false;
	assert(!(readOnly && createNew));

	if (readOnly_ == false) return false;

	// by definition nothing is sorted anymore because we have multiple files
	isSorted_ = false;

	if (filename_ != nullptr) delete [] filename_;
	LT_MEM_TRACK_ALLOC(filename_ = new char[strlen(filename)+1], LT_MEM_TYPE_MISC);
	assert(filename_ != nullptr);
	strcpy(filename_, filename);

	if (IsDirectory(filename))
	{
		RezFileDirectoryEmulation* rezFile;
		LT_MEM_TRACK_ALLOC(rezFile = new RezFileDirectoryEmulation(this, maxOpenFilesInEmulatedDir_), LT_MEM_TYPE_MISC);
		assert(rezFile != nullptr);
		if (rezFile == nullptr)
		{
			delete [] filename_;
			filename_ = nullptr;
			return false;
		}
		rezFilesList_.Insert(rezFile);
		++numRezFiles_;

		if (!rezFile->Open(filename, readOnly, createNew)) return false;
		fileOpened_ = true;

		ReadEmulationDirectory(rezFile, rootDir_, filename_, overwriteItems);
		return true;
	}

	RezFile* rezFile;
	LT_MEM_TRACK_ALLOC(rezFile = new RezFile(this), LT_MEM_TYPE_MISC);
	assert(rezFile != nullptr);
	if (rezFile == nullptr)
	{
		delete [] filename_;
		filename_ = nullptr;
		return false;
	}
	rezFilesList_.Insert(rezFile);
	++numRezFiles_;

	if (!rezFile->Open(filename, readOnly, createNew)) return false;

	FileMainHeaderStruct header;
	rezFile->Read(0, 0, sizeof(header), &header);

	if (header.LargestKeyAry > largestKeyArray_) largestKeyArray_ = header.LargestKeyAry;
	if (header.LargestDirNameSize > largestDirNameSize_) largestDirNameSize_ = header.LargestDirNameSize;
	if (header.LargestRezNameSize > largestRezNameSize_) largestRezNameSize_ = header.LargestRezNameSize;
	if (header.LargestCommentSize > largestCommentSize_) largestCommentSize_ = header.LargestCommentSize;

	assert(header.CR1 == 0x0d);
	assert(header.LF2 == 0x0a);
	assert(header.EOF1 == 0x1a);
	assert(header.FileFormatVersion == 1);

	rootDir_->ReadAllDirs(rezFile, header.RootDirPos, header.RootDirSize, overwriteItems);
	return true;
}

bool RezMgr::ReadEmulationDirectory(RezFileDirectoryEmulation* rezFileEmulation, RezDir* rezDir, const char* paramPath, bool overwriteItems)
{
	assert(rezDir != nullptr);
	assert(this != nullptr);
	assert(paramPath != nullptr);
	assert(fileOpened_);

	// figure out the path to this dir with added backslash
	char path[_MAX_DRIVE+_MAX_DIR+_MAX_FNAME+_MAX_EXT+5];
	strcpy(path, paramPath);
	if (path[strlen(path)-1] != '\\') strcat(path, "\\");

	// figure out the find search string by adding *.* to search for everything
	char findPath[_MAX_DRIVE+_MAX_DIR+_MAX_FNAME+_MAX_EXT+5];
	strcpy(findPath, path);
	strcat(findPath, "*.*");

	_finddata_t fileInfo;
	long findHandle = _findfirst(findPath, &fileInfo);
	if (findHandle >= 0)
	{
		do
		{
			if (strcmp(fileInfo.name, ".") == 0) continue;
			if (strcmp(fileInfo.name, "..") == 0) continue;

			if ((fileInfo.attrib & _A_SUBDIR) == _A_SUBDIR)
			{
				char baseName[_MAX_DRIVE+_MAX_DIR+_MAX_FNAME+_MAX_EXT+5];
				strcpy(baseName, fileInfo.name);
				if (!lowerCaseUsed_) _strupr(baseName);

				char pathName[_MAX_DRIVE+_MAX_DIR+_MAX_FNAME+_MAX_EXT+5];
				strcpy(pathName, path);
				strcat(pathName, baseName);
				strcat(pathName, "\\");

				RezDir* newDir;
				newDir = rezDir->GetDir(baseName);
				if (newDir == nullptr) newDir = rezDir->CreateDir(baseName);

				if (newDir == nullptr)
				{
					assert(false);
					continue;
				}

				ReadEmulationDirectory(rezFileEmulation, newDir, pathName, overwriteItems);
			}
			else
			{
				char fileName[_MAX_DRIVE+_MAX_DIR+_MAX_FNAME+_MAX_EXT+5];
				strcpy(fileName, path);
				strcat(fileName, fileInfo.name);

				char drive[_MAX_DRIVE+1];
				char dir[_MAX_DIR+1];
				char fname[_MAX_FNAME+1+_MAX_EXT+1];
				char ext[_MAX_EXT+1];
				_splitpath(fileName, drive, dir, fname, ext);

				char rezName[_MAX_DRIVE+_MAX_DIR+_MAX_FNAME+_MAX_EXT+5];
				assert(strlen(fname) < _MAX_DRIVE+_MAX_DIR+_MAX_FNAME+_MAX_EXT+5);
				strcpy(rezName, fname);
				_strupr(rezName);

				// figure out the ID for this file (if name is all digits use it as ID number, otherwise
				// assign a number)
				unsigned long rezId;
				{
					int nameLength = (int)strlen(rezName);
					int i;
					for (i = 0; i < nameLength; ++i)
					{
						if ((rezName[i] < '0') || (rezName[i] > '9')) break;
					}
					if (i < nameLength)
					{
						rezId = nextIDNumToUse_;
						++nextIDNumToUse_;
					}
					else
					{
						rezId = atol(rezName);
					}
				}

				unsigned long rezTypeId;
				if (strlen(ext) > 5)
				{
					strncat(rezName, ext, _MAX_FNAME+1+_MAX_EXT);
					fname[_MAX_FNAME] = '\0';
					rezTypeId = 0;
				}
				else
				{
					char sExt[5];
					if (strlen(ext) > 0)
					{
						strcpy(sExt, &ext[1]);
						_strupr(sExt);
						rezTypeId = StrToType(sExt);
					}
					else
					{
						rezTypeId = 0;
					}
				}

				char sType[5];
				TypeToStr(rezTypeId, sType);

				RezType* rezType = rezDir->GetOrMakeType(rezTypeId);
				assert(rezType != nullptr);

				RezItem* rezItem;
				rezItem = rezDir->GetRez(rezName, rezTypeId);
				if (rezItem == nullptr)
				{
					rezItem = rezDir->CreateRezInternal(rezId, rezName, rezType, nullptr);
				}
				else
				{
					if (overwriteItems)
					{
						rezDir->RemoveRezInternal(rezType, rezItem);
						rezItem = rezDir->CreateRezInternal(rezId, rezName, rezType, nullptr);
					}
					else
					{
						rezItem = nullptr;
					}
				}

				if (rezItem != nullptr)
				{
					rezItem->SetTime((unsigned long)fileInfo.time_write);
					rezItem->size_ = fileInfo.size;
					rezItem->rezFile_ = new RezFileSingleFile(this, fileName, rezFileEmulation);
					assert(rezItem->rezFile_ != nullptr);
				}
			}
		} while (_findnext(findHandle, &fileInfo) == 0);

		_findclose(findHandle);
	}

	return true;
}

bool RezMgr::Close(bool compact)
{
	assert(fileOpened_);
	bool retVal;

	if (!readOnly_) Flush();

	retVal = primaryRezFile_->Close();

	rezFilesList_.Delete(primaryRezFile_);
	--numRezFiles_;
	delete primaryRezFile_;
	primaryRezFile_ = nullptr;

	BaseRezFile* rezFile;
	while ((rezFile = rezFilesList_.GetFirst()) != nullptr)
	{
		rezFile->Close();
		rezFilesList_.Delete(rezFile);
		--numRezFiles_;
		delete rezFile;
	}

	if (rootDir_ != nullptr)
	{
		delete rootDir_;
		rootDir_ = nullptr;
	}
	if (filename_ != nullptr)
	{
		delete [] filename_;
		filename_ = nullptr;
	}

	fileOpened_ = false;
	return retVal;
}

RezDir* RezMgr::GetRootDir()
{
	assert(fileOpened_);
	return rootDir_;
}

unsigned long RezMgr::StrToType(const char* s)
{
	assert(s != nullptr);
	if (s == nullptr) return 0;

	unsigned long rezTypeId = 0;
	unsigned char* p = (unsigned char*)&rezTypeId;

	int length = (int)strlen(s);

	if (length > 0) p[length-1] = s[0];
	if (length > 1) p[length-2] = s[1];
	if (length > 2) p[length-3] = s[2];
	if (length > 3) p[length-4] = s[3];
	
	return rezTypeId;
}

void RezMgr::TypeToStr(unsigned long rezTypeId, char* sType)
{
	assert(sType != nullptr);
	if (sType == nullptr) return;
	unsigned char* pType = (unsigned char*)&rezTypeId;
	int length = 0;

	if (pType[3] != '\0') length = 4;
	else if (pType[2] != '\0') length = 3;
	else if (pType[1] != '\0') length = 2;
	else if (pType[0] != '\0') length = 1;

	if (length > 0) sType[0] = pType[length-1];
	if (length > 1) sType[1] = pType[length-2];
	if (length > 2) sType[2] = pType[length-3];
	if (length > 3) sType[3] = pType[length-4];

	sType[length] = '\0';
}

void* RezMgr::Alloc(unsigned long numBytes)
{
	assert(false);
	return nullptr;
}

void RezMgr::Free(void* p)
{
	assert(false);
}

bool RezMgr::DiskError()
{
	return false;
}

bool RezMgr::VerifyFileOpen()
{
	bool retVal = true;
	BaseRezFile* rezFile = rezFilesList_.GetFirst();
	while (rezFile != nullptr)
	{
		if (rezFile->VerifyFileOpen() == false) retVal = false;
		rezFile = rezFile->Next();
	}
	return retVal;
}

void RezMgr::SetHashTableBins(unsigned int nByNameNumHashBins, unsigned int nByIDNumHashBins,
                              unsigned int nDirNumHashBins, unsigned int nTypeNumHashBins)
{
	byNameNumHashBins_ = nByNameNumHashBins;
	byIDNumHashBins_   = nByIDNumHashBins;
	dirNumHashBins_    = nDirNumHashBins;
	typeNumHashBins_   = nTypeNumHashBins;
}

bool RezMgr::Flush()
{
	assert(readOnly_ != true);

	if (readOnly_)
		return false;

	// save the next write pos for the header information
	unsigned long saveWritePos = nextWritePos_;

	// write out all of the directories
	rootDir_->WriteAllDirs(primaryRezFile_, &rootDirPos_, &rootDirSize_);

	// fill out the permant parts of the header
	FileMainHeaderStruct header;
	header.CR1 = 0x0d;
	header.CR2 = 0x0d;
	header.CR3 = 0x0d;
	header.LF1 = 0x0a;
	header.LF2 = 0x0a;
	header.LF3 = 0x0a;
	header.EOF1 = 0x1a;
	
	memset(header.FileType, ' ', RezMgrUserTitleSize);
	strcpy(header.FileType, "RezMgr Version 1 Copyright (C) 1995 MONOLITH INC.");
	header.FileType[strlen(header.FileType)] = ' ';
	
	memset(header.UserTitle, ' ', RezMgrUserTitleSize);
	if (userTitle_[0] != '\0') memcpy(header.UserTitle, userTitle_, strlen(userTitle_));

	header.FileFormatVersion      = 1;
	header.RootDirPos             = rootDirPos_;
	header.RootDirSize            = rootDirSize_;
	header.RootDirTime            = rootDirTime_;
	header.NextWritePos           = saveWritePos;
	header.Time                   = lastTimeModified_;
	header.LargestKeyAry          = largestKeyArray_;
	header.LargestDirNameSize     = largestDirNameSize_;
	header.LargestRezNameSize     = largestRezNameSize_;
	header.LargestCommentSize     = largestCommentSize_;

	primaryRezFile_->Write(0, 0, sizeof(header), &header);
	primaryRezFile_->Flush();
	return true;
}

bool RezDir::WriteAllDirs(BaseRezFile* rezFile, unsigned long* pos, unsigned long* size)
{
	bool retFlag = true;

	// first write out all directories contained in this directory
	RezDirHash* it = hashTableSubDirs_.GetFirst();
	while (it != nullptr)
	{
		RezDir* rezDir = it->GetRezDir();
		assert(rezDir != nullptr);
		if (!rezDir->WriteAllDirs(rezFile, &rezDir->dirPos_, &rezDir->dirSize_))
		{
			retFlag = false;
			break;
		}
		it = it->Next();
	}

	// now write out our own directory block
	*pos = rezMgr_->nextWritePos_;
	if (!WriteDirBlock(rezFile, rezMgr_->nextWritePos_, size)) retFlag = false;
	else rezMgr_->nextWritePos_ += *size;

	return retFlag;
}

bool RezDir::WriteDirBlock(BaseRezFile* rezFile, unsigned long pos, unsigned long* size)
{
	assert(pos > 0);
	unsigned long curPos = pos;
	FileDirEntryHeader header;
	unsigned char zero = 0;

	// write all dir hash table contents out ot file
	{
		header.Type = DirectoryEntry;
		RezDirHash* it = hashTableSubDirs_.GetFirst();
		while (it != nullptr)
		{
			RezDir* rezDir = it->GetRezDir();
			assert(rezDir != nullptr);

			header.Dir.Pos  = rezDir->dirPos_;
			header.Dir.Size = rezDir->dirSize_;
			header.Dir.Time = rezDir->lastTimeModified_;

			curPos += rezFile->Write(curPos, 0, sizeof(header.Type), &header.Type);
			curPos += rezFile->Write(curPos, 0, sizeof(header.Dir.Pos), &header.Dir.Pos);
			curPos += rezFile->Write(curPos, 0, sizeof(header.Dir.Size), &header.Dir.Size);
			curPos += rezFile->Write(curPos, 0, sizeof(header.Dir.Time), &header.Dir.Time);

			if (rezDir->dirName_ == nullptr) curPos += rezFile->Write(curPos, 0, sizeof(zero), &zero);
			else curPos += rezFile->Write(curPos, 0, (unsigned long)strlen(rezDir->dirName_)+1, rezDir->dirName_);

			it = it->Next();
		}
	}

	// write all type hash table contents
	{
		header.Type = ResourceEntry;
		RezTypeHash* it = hashTableTypes_.GetFirst();
		RezItemHashByName* item;
		while (it != nullptr)
		{
			assert(it->GetRezType() != nullptr);

			item = it->GetRezType()->hashTableByName_.GetFirst();
			while (item != nullptr)
			{
				RezItem* rezItem = item->GetRezItem();
				assert(rezItem != nullptr);

				header.Rez.Pos     = rezItem->filePos_;
				header.Rez.Size    = rezItem->size_;
				header.Rez.Time    = rezItem->time_;
				header.Rez.ID      = 0;
				header.Rez.Type    = it->GetRezType()->GetType();
				header.Rez.NumKeys = 0;

				curPos += rezFile->Write(curPos, 0, sizeof(header.Type), &header.Type);
				curPos += rezFile->Write(curPos, 0, sizeof(header.Rez.Pos), &header.Rez.Pos);
				curPos += rezFile->Write(curPos, 0, sizeof(header.Rez.Size), &header.Rez.Size);
				curPos += rezFile->Write(curPos, 0, sizeof(header.Rez.Time), &header.Rez.Time);
				curPos += rezFile->Write(curPos, 0, sizeof(header.Rez.ID), &header.Rez.ID);
				curPos += rezFile->Write(curPos, 0, sizeof(header.Rez.Type), &header.Rez.Type);
				curPos += rezFile->Write(curPos, 0, sizeof(header.Rez.NumKeys), &header.Rez.NumKeys);
				if (rezItem->name_ == nullptr) curPos += rezFile->Write(curPos, 0, sizeof(zero), &zero);
				else curPos += rezFile->Write(curPos, 0, (unsigned long)strlen(rezItem->name_)+1, rezItem->name_);
				curPos += rezFile->Write(curPos, 0, sizeof(zero), &zero);

				item = item->Next();
			}
			it = it->Next();
		}
	}

	rezMgr_->nextWritePos_ = curPos;
	*size = curPos - pos;
	return true;
}

unsigned long RezMgr::GetCurTime()
{
	time_t timeVal;
	timeVal = time(&timeVal);
	return (unsigned long)timeVal;
}

void RezMgr::SetDirSeparators(const char* dirSeparators)
{
	if (dirSeparators_ != nullptr) delete [] dirSeparators_;
	int len = (int)strlen(dirSeparators);
	LT_MEM_TRACK_ALLOC(dirSeparators_ = new char[len+1], LT_MEM_TYPE_MISC);
	strcpy(dirSeparators_, dirSeparators);
}

bool RezDir::IsGoodChar(char c)
{
	if (rezMgr_->dirSeparators_ != nullptr)
	{
		return (strchr(rezMgr_->dirSeparators_, c) == nullptr);
	}
	else
	{
		if (c >= ' ' && c <= '.') return true;
		if (c >= '0' && c <= '9') return true;
		if (c >= 'A' && c <= 'Z') return true;
		if (c >= 'a' && c <= 'z') return true;
		return false;
	}
}

RezDir* RezDir::GetDirFromPath(const char* path)
{
	// strip off leading slash
	int len = (int)strlen(path);
	if (len > 1)
	{
		if (!IsGoodChar(path[0]))
		{
			path = &path[1];
			--len;
		}
	}

	// read the first directory name
	int i = 0;
	char dir[1024];
	while (IsGoodChar(path[i]))
	{
		dir[i] = path[i];
		++i;
		if (i >= 1023) break;
	}
	dir[i] = '\0';

	RezDir* rezDir = GetDir(dir);
	if (!rezDir) return nullptr;

	// if this is the last name in the path string, we're done
	if (path[i] == '\0')
		return rezDir;

	// skip any white space
	while (!IsGoodChar(path[i]))
	{
		++i;
		if (path[i] == '\0') return rezDir;
	}

	// recursively continue
	return rezDir->GetDirFromPath(&path[i]);
}

RezItem* RezDir::GetRezFromDosPath(const char* path)
{
	char dirName[1024];
	char rezName[1024];

	int len = (int)strlen(path);
	if (len >= 1023) return nullptr;

	// strip off leading slash
	if (len > 1)
	{
		if (!IsGoodChar(path[0]))
		{
			path = &path[1];
			--len;
		}
	}

	// strip off the rez name
	int i = len-1;
	while (IsGoodChar(path[i]))
	{
		--i;
		if (i < 0) break;
	}
	if (i == len) return nullptr;

	strncpy(rezName, &path[i+1], 1023);
	rezName[1023] = '\0';

	// if we were only given a rez name, try to get it
	if (i <= 1)
	{
		return GetRezFromDosName(rezName);
	}

	// copy everything except for the rez name
	strncpy(dirName, path, i);
	dirName[i] = '\0';

	// find the directory for this path
	RezDir* rezDir = GetDirFromPath(dirName);
	if (!rezDir) return nullptr;

	// try to get the rez fromthe dir we just got
	return rezDir->GetRezFromDosName(rezName);
}

RezItem* RezDir::GetRezFromPath(const char* path, unsigned long rezTypeId)
{
	char dirName[1024];
	char rezName[1024];

	// strip off leading slash
	int len = (int)strlen(path);
	if (len >= 1023) return nullptr;
	if (len > 1)
	{
		if (!IsGoodChar(path[0]))
		{
			path = &path[1];
			--len;
		}
	}

	// strip off the rez name
	int i = len - 1;
	while (IsGoodChar(path[i]))
	{
		--i;
		if (i < 0) break;
	}
	if (i == len) return nullptr;

	strncpy(rezName, &path[i+1], 1023);
	rezName[1023] = '\0';

	// if we were only give a rez name, try to get it
	if (i <= 1)
	{
		return GetRez(rezName, rezTypeId);
	}

	// copy everything except for the rez name
	strncpy(dirName, path, i);
	dirName[i] = '\0';

	// find the directory for this path
	RezDir* rezDir = GetDirFromPath(dirName);
	if (!rezDir) return nullptr;

	// try to get the rez from the dir we just got
	return rezDir->GetRez(rezName, rezTypeId);
}

RezItem* RezMgr::GetRezFromPath(const char* path, unsigned long rezTypeId)
{
	return GetRootDir()->GetRezFromPath(path, rezTypeId);
}

RezItem* RezMgr::GetRezFromDosPath(const char* path)
{
	return GetRootDir()->GetRezFromDosPath(path);
}

RezDir* RezMgr::GetDirFromPath(const char* path)
{
	return GetRootDir()->GetDirFromPath(path);
}

bool RezMgr::Reset()
{
	if (!IsOpen()) return false;

	Close();
	return Open(filename_);
}

bool RezMgr::IsDirectory(const char* filename)
{
	struct stat buf;
	int result;

	result = stat(filename, &buf);
	if (result != 0) return false;

	return ((buf.st_mode & _S_IFDIR) == _S_IFDIR);
}

RezItem* RezMgr::AllocateRezItem()
{
	RezItem* newItem = nullptr;
	RezItemHashByName* it;

	it = hashRezItemFreeList_.GetFirst();
	if (it != nullptr) newItem = it->GetRezItem();

	// if we are out of free RezItems then make a new chunk and allocate one from there
	if (newItem == nullptr)
	{
		RezItemChunk* newChunk;
		LT_MEM_TRACK_ALLOC(newChunk = new RezItemChunk, LT_MEM_TYPE_MISC);
		if (newChunk == nullptr) return nullptr;

		LT_MEM_TRACK_ALLOC(newChunk->rezItemArray_ = new RezItem[rezItemChunkSize_], LT_MEM_TYPE_MISC);
		if (newChunk->rezItemArray_ == nullptr)
		{
			delete newChunk;
			return nullptr;
		}

		for (unsigned int i = 0; i < rezItemChunkSize_; ++i)
		{
			newChunk->rezItemArray_[i].hashByName_.SetRezItem(&newChunk->rezItemArray_[i]);
			hashRezItemFreeList_.Insert(&newChunk->rezItemArray_[i].hashByName_);
		}

		rezItemChunksList_.Insert(newChunk);
		newItem = hashRezItemFreeList_.GetFirst()->GetRezItem();
	}

	if (newItem != nullptr)
	{
		hashRezItemFreeList_.Delete(&newItem->hashByName_);
	}

	return newItem;
}

void RezMgr::DeAllocateRezItem(RezItem* rezItem)
{
	if (rezItem != nullptr)
	{
		hashRezItemFreeList_.Insert(&rezItem->hashByName_);
	}
}

void RezMgr::SetUserTitle(const char* userTitle)
{
	strncpy(userTitle_, userTitle, RezMgrUserTitleSize);
	userTitle_[RezMgrUserTitleSize] = '\0';
}

}}