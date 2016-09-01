#include "RezMgr/RezFile.hpp"
#include "RezMgr/RezMgr.hpp"
#include "Memory/Memory.hpp"
#include "Common/SafeString.hpp"

#include <assert.h>
#include <string.h>

#define SEEKPOS_ERROR 0xFFFFFFFF

namespace JupiterEx { namespace RezMgr {

//------------------------------------------------------------------------------------------
// BaseRezFile

BaseRezFile::BaseRezFile(RezMgr* rezMgr)
{
	assert(rezMgr_ != nullptr);
	rezMgr_ = rezMgr;
}

BaseRezFile::~BaseRezFile()
{
	rezMgr_ = nullptr;
}

//------------------------------------------------------------------------------------------
// RezFile

RezFile::RezFile(RezMgr* rezMgr) : BaseRezFile(rezMgr)
{
	file_ = nullptr;
	filename_ = nullptr;
	lastSeekPos_ = SEEKPOS_ERROR;
}

RezFile::~RezFile()
{
	if (file_ != nullptr) Close();
	if (filename_ != nullptr) delete [] filename_;
}

unsigned long RezFile::Read(unsigned long itemPos, unsigned long itemOffset, unsigned long size, void* data)
{
	assert(file_ != nullptr);
	assert(data != nullptr);
	assert(rezMgr_ != nullptr);

	if (size <= 0) return 0;

	unsigned long seekPos = itemPos + itemOffset;
	if (lastSeekPos_ != seekPos)
	{
		while (fseek(file_, seekPos, SEEK_SET) != 0)
		{
			if (!rezMgr_->DiskError())
			{
				lastSeekPos_ = SEEKPOS_ERROR;
				assert(false && "seek failed!");
				return 0;
			}
		}
	}

	unsigned long ret;
	while ((ret = fread(data, 1, size, file_)) != size)
	{
		if (!rezMgr_->DiskError())
		{
			lastSeekPos_ = SEEKPOS_ERROR;
			assert(false && "fread() failed!");
			return 0;
		}
	}

	lastSeekPos_ = seekPos + ret;
	return ret;
}

unsigned long RezFile::Write(unsigned long itemPos, unsigned long itemOffset, unsigned long size, void* data)
{
	assert(file_ != nullptr);
	assert(data != nullptr);
	assert(readOnly_ != true);
	assert(rezMgr_ != nullptr);

	lastSeekPos_ = SEEKPOS_ERROR;

	if (size <= 0) return 0;

	while (fseek(file_, itemPos+itemOffset, SEEK_SET) != 0)
	{
		if (!rezMgr_->DiskError())
		{
			assert(false && "fseek() failed");
			return 0;
		}
	}

	unsigned long ret;
	while ((ret = fwrite(data, 1, size, file_)) != size)
	{
		if (!rezMgr_->DiskError())
		{
			assert(false && "fwrite() failed!");
			return 0;
		}
	}

	return ret;
}

bool RezFile::Open(const char* filename, bool readOnly, bool createNew)
{
	do
	{
		if (createNew)
		{
			if (readOnly) return false;
			else file_ = fopen(filename, "w+b");
		}
		else
		{
			if (readOnly) file_ = fopen(filename, "rb");
			else file_ = fopen(filename, "r+b");
		}

		if (file_ == nullptr)
		{
			if (!rezMgr_->DiskError()) return false;
		}
	} while (file_ == nullptr);

	readOnly_  = readOnly;

	if (filename_ != nullptr)
	{
		LT_MEM_TRACK_FREE(delete [] filename_);
	}

	size_t length = strlen(filename) + 1;
	LT_MEM_TRACK_ALLOC(filename_ = new char[length], LT_MEM_TYPE_MISC);

	if (filename_ != nullptr)
	{
		LTStrCpy(filename_, filename, length);
	}

	lastSeekPos_ = SEEKPOS_ERROR;
	return true;
}

bool RezFile::Close()
{
	assert(rezMgr_ != nullptr);

	bool ret;
	int check;

	if (file_ == nullptr)
	{
		return false;
	}

	do
	{
		check = fclose(file_);
		if (check == 0)
		{
			ret = true;
		}
		else
		{
			ret = false;
			if (rezMgr_->DiskError()) return false;
		}
	} while (ret == false);

	file_ = nullptr;
	if (filename_ != nullptr)
	{
		LT_MEM_TRACK_FREE(delete [] filename_);
		filename_ = nullptr;
	}
	lastSeekPos_ = SEEKPOS_ERROR;
	return ret;
}

bool RezFile::Flush()
{
	assert(file_ != nullptr);
	assert(rezMgr_ != nullptr);

	lastSeekPos_ = SEEKPOS_ERROR;

	if (file_ == nullptr)
	{
		return false;
	}

	bool ret;
	int check;
	do
	{
		check = fflush(file_);
		if (check == 0)
		{
			ret = true;
		}
		else
		{
			ret = false;
			if (rezMgr_->DiskError()) return false;
		}
	} while (ret == false);
	return ret;
}

bool RezFile::VerifyFileOpen()
{
	lastSeekPos_ = SEEKPOS_ERROR;

	if (file_ == nullptr)
		return false;

	if (ftell(file_) != -1L)
	{
		return true;
	}
	else
	{
		if (!Open(filename_, readOnly_, false))
			return false;
	}

	return true;
}

const char* RezFile::GetFileName()
{
	return filename_;
}

//------------------------------------------------------------------------------------------
// RezFileDirectoryEmulation

RezFileDirectoryEmulation::RezFileDirectoryEmulation(RezMgr* rezMgr, int maxOpenFiles) : BaseRezFile(rezMgr)
{
	assert(maxOpenFiles > 0);
	numOpenFiles_ = 0;
	maxOpenFiles_ = maxOpenFiles;
	readOnly_     = true;
	createNew_    = false;
}

RezFileDirectoryEmulation::~RezFileDirectoryEmulation()
{
	RezFileSingleFile *it;

	while ((it = openFiles_.GetFirst()) != nullptr)
	{
		delete it;
	}

	while ((it = closedFiles_.GetFirst()) != nullptr)
	{
		delete it;
	}
}

unsigned long RezFileDirectoryEmulation::Read(unsigned long itemPos, unsigned long itemOffset, unsigned long size, void* data)
{
	assert(false && "this should never be called");
	return 0;
}

unsigned long RezFileDirectoryEmulation::Write(unsigned long itemPos, unsigned long itemOffset, unsigned long size, void* data)
{
	assert(false && "this should never be called");
	return 0;
}

bool RezFileDirectoryEmulation::Open(const char* filename, bool readOnly, bool createNew)
{
	readOnly_  = readOnly;
	createNew_ = createNew;
	return true;
}

bool RezFileDirectoryEmulation::Close()
{
	RezFileSingleFile* it;
	while ((it = openFiles_.GetFirst()) != nullptr)
	{
		it->ReallyClose();
	}
	return true;
}

bool RezFileDirectoryEmulation::Flush()
{
	return true;
}

bool RezFileDirectoryEmulation::VerifyFileOpen()
{
	return true;
}

const char* RezFileDirectoryEmulation::GetFileName()
{
	return nullptr;
}

//------------------------------------------------------------------------------------------
// RezFileSingleFile

RezFileSingleFile::RezFileSingleFile(RezMgr* rezMgr, const char* filename, RezFileDirectoryEmulation* dirEmulation) : BaseRezFile(rezMgr)
{
	assert(filename != nullptr);
	assert(dirEmulation != nullptr);

	dirEmulation_ = dirEmulation;
	file_ = nullptr;

	LT_MEM_TRACK_ALLOC(filename_ = new char[strlen(filename)+1], LT_MEM_TYPE_MISC);
	assert(filename_ != nullptr);
	strcpy(filename_, filename);

	dirEmulation_->closedFiles_.Insert(this);
}

RezFileSingleFile::~RezFileSingleFile()
{
	if (file_ != nullptr) ReallyClose();
	if (filename_ != nullptr) { LT_MEM_TRACK_FREE(delete [] filename_); }
	dirEmulation_->closedFiles_.Delete(this);
}

unsigned long RezFileSingleFile::Read(unsigned long itemPos, unsigned long itemOffset, unsigned long size, void* data)
{
	assert(data != nullptr);
	assert(dirEmulation_ != nullptr);
	assert(dirEmulation_->rezMgr_ != nullptr);

	if (size <= 0) return 0;

	if (file_ == nullptr) ReallyOpen();

	while (fseek(file_, itemOffset, SEEK_SET) != 0)
	{
		if (dirEmulation_->rezMgr_->DiskError())
		{
			assert(false);
			return 0;
		}
	}

	unsigned long ret;
	while ((ret = fread(data, 1, size, file_)) != size)
	{
		if (dirEmulation_->rezMgr_->DiskError())
		{
			assert(false);
			return 0;
		}
	}

	return ret;
}

unsigned long RezFileSingleFile::Write(unsigned long itemPos, unsigned long itemOffset, unsigned long size, void* data)
{
	assert(data != nullptr);
	assert(dirEmulation_ != nullptr);
	assert(dirEmulation_->readOnly_ != true);
	assert(dirEmulation_->rezMgr_ != nullptr);

	if (size <= 0) return 0;

	if (file_ == nullptr) ReallyOpen();

	while (fseek(file_, itemOffset, SEEK_SET) != 0)
	{
		if (!dirEmulation_->rezMgr_->DiskError())
		{
			assert(false);
			return 0;
		}
	}

	unsigned long ret;
	while ((ret = fwrite(data, 1, size, file_)) != size)
	{
		if (dirEmulation_->rezMgr_->DiskError())
		{
			assert(false);
			return 0;
		}
	}

	return ret;
}

bool RezFileSingleFile::Open(const char* filename, bool readOnly, bool createNew)
{
	assert(false && "this should never be called");
	return false;
}

bool RezFileSingleFile::Close()
{
	assert(false && "this should never be called");
	return false;
}

bool RezFileSingleFile::Flush()
{
	assert(dirEmulation_->rezMgr_ != nullptr);

	if (file_ == nullptr)
		return true;

	bool ret;
	while ((ret = (fflush(file_) == 0)) == false)
	{
		if (!dirEmulation_->rezMgr_->DiskError()) return false;
	}
	return ret;
}

bool RezFileSingleFile::VerifyFileOpen()
{
	assert(false && "this should never be called");
	return false;
}

bool RezFileSingleFile::ReallyOpen()
{
	assert(filename_ != nullptr);
	assert(dirEmulation_ != nullptr);
	assert(dirEmulation_->rezMgr_ != nullptr);
	
	if (file_ != nullptr)
		return true;

	// check if we need to close another file because we have too many open
	if (dirEmulation_->numOpenFiles_ > dirEmulation_->maxOpenFiles_)
	{
		RezFileSingleFile *oneFile = dirEmulation_->openFiles_.GetLast();
		assert(oneFile != nullptr);
		if (oneFile != nullptr) oneFile->ReallyClose();
	}

	do
	{
		if (dirEmulation_->createNew_)
		{
			if (dirEmulation_->readOnly_) return false;
			else file_ = fopen(filename_, "w+b");
		}
		else
		{
			if (dirEmulation_->readOnly_) file_ = fopen(filename_, "rb");
			else file_ = fopen(filename_, "r+b");
		}

		if (file_ == nullptr)
		{
			if (dirEmulation_->rezMgr_->DiskError()) return false;
		}
	} while (file_ == nullptr);

	dirEmulation_->closedFiles_.Delete(this);
	dirEmulation_->openFiles_.InsertFirst(this);
	dirEmulation_->numOpenFiles_ += 1;

	return true;
}

bool RezFileSingleFile::ReallyClose()
{
	assert(dirEmulation_ != nullptr);
	assert(dirEmulation_->rezMgr_ != nullptr);

	if (file_ == nullptr)
return true;

	bool ret;
	while ((ret = (fclose(file_) == 0)) == false)
	{
		if (!dirEmulation_->rezMgr_->DiskError()) return false;
	}

	dirEmulation_->numOpenFiles_ -= 1;
	dirEmulation_->openFiles_.Delete(this);
	dirEmulation_->closedFiles_.Insert(this);

	file_ = nullptr;
	return ret;
}

const char* RezFileSingleFile::GetFileName()
{
	return filename_;
}

}}