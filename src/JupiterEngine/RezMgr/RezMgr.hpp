#pragma once

#include "RezFile.hpp"
#include "RezHash.hpp"

namespace JupiterEx { namespace RezMgr {

#define RezMgrUserTitleSize  60

class RezType;
class RezDir;
class RezMgr;

// -----------------------------------------------------------------------------------------
// RezItem

class RezItem
{
public:
	const char* GetName() { return name_; }
	unsigned long GetType();
	unsigned long GetSize() { return size_; }
	const char* GetPath(char* buf, unsigned long bufSize);
	const char* GetDir();
	RezDir* GetParentDir() { return parentDir_; }
	unsigned long GetTime() { return time_; }

	bool Get(unsigned char* bytes);
	bool Get(void* bytes) { return Get((unsigned char*)bytes); }
	bool Get(unsigned char* bytes, unsigned long startOffset, unsigned long length);
	bool Get(void* bytes, unsigned long startOffset, unsigned long length) { return Get((unsigned char*)bytes, startOffset, length); }

	unsigned char* Load();
	bool UnLoad();
	bool IsLoaded();

	unsigned long GetSeekPos() { return currPos_; }
	bool Seek(unsigned long offset);
	unsigned long Read(unsigned char* bytes, unsigned long length, unsigned long seekPos = REZ_SEEKPOS_ERROR);
	unsigned long Read(void* bytes, unsigned long length, unsigned long seekPos = REZ_SEEKPOS_ERROR) { return Read((void*)bytes, length, seekPos); }
	bool EndOfRes();
	char GetChar();

	void SetTime(unsigned long time) { time_ = time; }

	// functions that can be used to gain direct read access to the rez file (DANGER!!!!!)
	const char* DirectRead_GetFullRezName();
	unsigned long DirectRead_GetFileOffset() { return filePos_; }

	unsigned char* Create(unsigned long size);
	bool Save();

private:
	RezItem();

	void InitRezItem(RezDir* parentDir, const char* name, unsigned long id, RezType* type, const char* desc,
					 unsigned long size, unsigned long filePos, unsigned long time, unsigned long numKeys,
					 unsigned long* keyArray, BaseRezFile* rezFile);
	void TermRezItem();

	friend class RezType;
	friend class RezDir;
	friend class RezMgr;

	void MarkCurTime();             // Mark the current modification time as now for this resource

private:
	char*              name_;
	RezType*           type_;
	unsigned long      time_;       // The last time the data in the resource was updated (does not include keys or description)
	unsigned long      size_;       // The size in bytes of the data in this resource
	RezDir*            parentDir_;  // Pointer to the directory struct in memory that this resource is in
	unsigned long      filePos_;    // File position in the resource file for this resources data (note, this is relative to dataPos_ in the directory)
	unsigned long      currPos_;    // Current seek position within this resource
	RezItemHashByName  hashByName_; // Hash element for by name hash table
	BaseRezFile*       rezFile_;    // Pointer to class that controls the base low level resource file that is associated with this resource
	unsigned char*     data_;       // Pointer to the data for this resource (if NULL then not in memory)
};

//------------------------------------------------------------------------------------------
// RezType

class RezType
{
public:
	unsigned long GetType() { return typeId_; }

private:
	RezType(unsigned long typeId, RezDir* parentDir, unsigned int nByIDNumHashBins, unsigned int nByNameNumHashBins);
	RezType(unsigned long typeId, RezDir* parentDir, unsigned int nByNameNumHashBins); // non ID hash table version
	~RezType();

	friend class RezItem;
	friend class RezDir;
	friend class RezMgr;

	unsigned long          typeId_;
	RezTypeHash            hashElementType_;
	RezItemHashTableByID   hashTableByID_;
	RezItemHashTableByName hashTableByName_;
	RezDir*                parentDir_;
};

//------------------------------------------------------------------------------------------
// RezDir

class RezDir
{
public:
	const char* GetDirName()   { return dirName_; }
	RezDir*     GetParentDir() { return parentDir_; }
	RezMgr*     GetParentMgr() { return rezMgr_; }

	RezItem*    GetRez(const char* rezName, unsigned long typeId);      // get a resource item in this directory by name
	RezItem*    GetRezFromDosName(const char* rezNameDOS);              // get a resource item from an old style dos file name which includes the type in the extension
	RezItem*    GetRezFromPath(const char* path, unsigned long typeId); // get a resource item from a full path and type 
	RezItem*    GetRezFromDosPath(const char* pathDOS);                 // get a resource item from a full path and extension

	bool Load(bool loadAllSubDirs = false);
	bool UnLoad(bool UnLoadAllSubDirs = false);
	bool IsLoaded() { return (memBlock_ != nullptr); }

	RezDir* GetDir(const char* dirName);
	RezDir* GetDirFromPath(const char* path);
	RezDir* GetFirstSubDir();
	RezDir* GetNextSubDir(RezDir* currDir);

	RezType* GetRezType(unsigned long typeId);
	RezType* GetFirstType();
	RezType* GetNextType(RezType* rezType);

	RezItem* GetFirstItem(RezType* rezType);
	RezItem* GetNextItem(RezItem* rezItem);

	RezDir* CreateDir(const char* dirName);
	RezItem* CreateRez(unsigned long rezId, const char* rezName, unsigned long typeId);
	unsigned long GetTime() { return lastTimeModified_; }

private:
	RezDir(RezMgr* rezMgr, RezDir* parentDir, const char* dirName, unsigned long dirPos,
		unsigned long dirSize, unsigned long time, unsigned int nDirNumHashBins, unsigned int nTypeNumHashBins);
	~RezDir();

	friend class RezItem;
	friend class RezType;
	friend class RezMgr;

	bool     ReadAllDirs(BaseRezFile* rezFile, unsigned long pos, unsigned long size, bool overwriteItems);  // Recursivly read all directories in this dir into memory
	bool     ReadDirBlock(BaseRezFile* rezFile, unsigned long pos, unsigned long size, bool overwriteItems); // Reads in directory block for this directory
	RezType* GetOrMakeType(unsigned long typeId);                                                            // Gets the type if it exists, creates it if it does not
	bool     IsGoodChar(char c);                                                                             // Determines if the given character is non-white space and non-seperator
	RezItem* CreateRezInternal(unsigned long rezId, const char* rezName, RezType* rezType, BaseRezFile* rezFile);
	bool     RemoveRezInternal(RezType* rezType, RezItem* rezItem);
	bool WriteAllDirs(BaseRezFile* rezFile, unsigned long* pos, unsigned long* size);
	bool WriteDirBlock(BaseRezFile* rezFile, unsigned long pos, unsigned long* size);

private:
	char* dirName_;
	unsigned long dirPos_;                // Position in directory data block in file
	unsigned long dirSize_;               // Size of the directory data block
	unsigned long itemsPos_;              // Position of resource items data for this directory
	unsigned long itemsSize_;             // Size of resource items data for this directory
	unsigned long lastTimeModified_;      // The last time that the data in a resource file in this directory was modified (does not include data in sub directories)
	RezMgr* rezMgr_;                      
	RezDir* parentDir_;
	RezDirHash hashElementDir_;
	RezDirHashTable hashTableSubDirs_;
	RezTypeHashTable hashTableTypes_;
	unsigned char* memBlock_;             // Pointer to memory block (used if all resources allocated at once)
};

//------------------------------------------------------------------------------------------
// RezMgr

class RezMgr
{
public:
	RezMgr();
	RezMgr(const char* filename, bool readOnly = true, bool createNew = false);
	~RezMgr();

	bool Open(const char* filename, bool readOnly = true, bool createNew = false);  // Open the current resource file
	bool OpenAdditional(const char* filename, bool overwriteItems = false);         // Open an additional resource file (the file is ReadOnly and not New by definition)
	bool Close(bool compact = false);                                               // Closes the current resource file (if compact is true also compacts the resource file)
	RezDir* GetRootDir();                                                           // Returns the root directory in the resource file
	bool IsOpen() { return fileOpened_; }                                           // Returns true if resource file is open, false if not
	bool VerifyFileOpen();                                                           // Checks if the files is actually open and try's to open it again if it is not
	unsigned long GetTime() { return lastTimeModified_; }                           // Last time anything in resource file was modified
	bool Reset();
	bool IsSorted() { return isSorted_; }

	RezItem* GetRezFromPath(const char* path, unsigned long typeId);
	RezItem* GetRezFromDosPath(const char* path);
	RezDir* GetDirFromPath(const char* path);

	void SetDirSeparators(const char* dirSeparators);

	virtual void* Alloc(unsigned long numBytes);   // used for all memory allocation in resource manager (currently no used or implemented!)
	virtual void Free(void* p);
	virtual bool DiskError();                      // called whenever a disk error occurs (if user returns true RezMgr trys again)

	unsigned long StrToType(const char *s);        // Convert an ascii string to a rez type
	void TypeToStr(unsigned long typeId, char* s); // Convert a rez type to an ascii string

	// control for how ID numbers work
	void SetRenumberIDCollisions(bool flag);  // defaults to true (if set to false duplicated ID's will be handled according to the overwritItems flag)
	void SetNextIDNumber(unsigned long id);   // defaults to 2000000000 and is incremented for every collision or resource in a raw directory that is loaded

	// lower case support (should call set right after constructor but before open)
	bool GetLowerCasedUsed() { return lowerCaseUsed_; }
	bool SetLowerCaseUsed(bool lowerCaseUsed) { lowerCaseUsed_ = lowerCaseUsed; }

	// support for accessing resources by ID (should call set right after constructor but before open)
	bool GetItemByIDUsed() { return itemByIDUsed_; }
	void SetItemByIDUsed(bool itemByIDUsed) { itemByIDUsed_ = itemByIDUsed; }

	// set the number of bin values for creating hash tables (should call set right after constructor but before open)
	void SetHashTableBins(unsigned int nByNameNumHashBins, unsigned int nByIDNumHashBins,
						  unsigned int nDirNumHashBins, unsigned int nTypeNumHashBins);

	// functions that user should not typically use
	void ForceIsSortedFlag(bool flag) { isSorted_ = flag; }
	void SetMaxOpenFilesInEmulatedDir(int numFiles) { maxOpenFilesInEmulatedDir_ = numFiles; }

	void SetUserTitle(const char* userTitle);
	char* GetUserTitle() { return userTitle_; }

	void SetNextIDNumToUse(unsigned long nNextIDNumToUse) { nextIDNumToUse_ = nNextIDNumToUse; }
	unsigned long GetNextIDNumToUse() { return nextIDNumToUse_; }

private:
	friend class RezDir;
	friend class RezType;
	friend class RezItem;

	class RezItemChunk : public Common::BaseListItem<RezItemChunk>
	{
	public:
		RezItem* rezItemArray_;
	};

	class RezItemChunkList : public Common::BaseList<RezItemChunk>
	{
	};

	RezItem* AllocateRezItem();
	void DeAllocateRezItem(RezItem* item);

	unsigned long GetCurTime();
	bool IsDirectory(const char* filename);
	bool ReadEmulationDirectory(RezFileDirectoryEmulation* rezFileEmulation, RezDir* dir, const char* paramPath, bool overwriteItems);
	bool Flush();

private:
	char* dirSeparators_;           // Separator characters between directories (if NULL(default) use built in method)
	bool isSorted_;                 // if TRUE the data is sorted and compacted by directory if FALSE then it is not
	bool fileOpened_;               // if TRUE the resource file is opened
	BaseRezFileList rezFilesList_;  // List of low level resource files
	unsigned long numRezFiles_;     // The number of rez files that are open
	BaseRezFile* primaryRezFile_;   // Pointer the main RezFile (one opend with open!)
	bool renumberIDCollisions_;     // If TRUE then ID's of resources that collide will simply be re-numbered
	unsigned long nextIDNumToUse_;  // Next ID number to use for allocating collisions and assigning to directories
	int maxOpenFilesInEmulatedDir_; // Maximum number of files that can be open at one time in a emulated fir

	// MOST OF THE REST OF THE VARIABLES BELOW ONLY APPLY TO THE FIRST RESOURCE FILE IN THE rezFilesList_ LIST
	unsigned long rootDirPos_;           // The seek position in the file where the root directory is located
	unsigned long rootDirSize_;          // The size of the root directory
	unsigned long rootDirTime_;          // The last time the root dir was modified
	unsigned long nextWritePos_;         // The next position int he file to write data out to
	bool          readOnly_;             // If TRUE then the resource file is only opsned for reading
	RezDir*       rootDir_;              // Pointer to the root directory structure in the resource
	unsigned long lastTimeModified_;     // The last time that any data in any resource in this resource file was modified (does not include key values and descriptions)
	bool          mustReWriteDirs_;      // If TRUE we must write out the directories on close
	unsigned long fileFormatVersion_;    // the file format version number, only 1 is possible here right now
	unsigned long largestKeyArray_;      // Size of the largest key array in the resource file
	unsigned long largestDirNameSize_;   // Size of the largest directory name in the resource file (including 0 terminator)
	unsigned long largestRezNameSize_;   // Size of the largest resource name in the resource file (includding 0 terminator)
	unsigned long largestCommentSize_;   // Size of the largest comment in the resource file (including 0 terminator)
	char *        filename_;             // Original file name user passed in to open the file
	bool          lowerCaseUsed_;        // If TRUE then lower case may be present in file and directory names (DEFAULT IS FALSE)
	bool          itemByIDUsed_;         // If TRUE then Rez items can be accessed by their ID, if false then they can not be (DEFAULT IS FALSE)
	unsigned int  byNameNumHashBins_;    // number of hash bins in the ItemByName hash table
	unsigned int  byIDNumHashBins_;      // number of hash bins in the ItemByID hash table
	unsigned int  dirNumHashBins_;       // number of hash bins in the Directory hash table
	unsigned int  typeNumHashBins_;      // number of hash bins in the Type hash table
	RezItemHashTableByName hashRezItemFreeList_; // free list of RezItem's using the hash table element inside the RezItem
	RezItemChunkList rezItemChunksList_; // list of RezItem chunks
	unsigned int  rezItemChunkSize_;     // number os rez items to allocate at once in a chunk
	char          userTitle_[RezMgrUserTitleSize+1]; // user title information found in file header
};

}}