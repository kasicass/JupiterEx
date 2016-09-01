#pragma once

#include <stdio.h>
#include "Common/BaseList.hpp"

namespace JupiterEx { namespace RezMgr {

class RezMgr;
class BaseRezFile;
class RezFileSingleFile;

class BaseRezFileList : public Common::BaseList<BaseRezFile>
{
};

class RezFileSingleFileList : public Common::BaseList<BaseRezFile>
{
public:
	RezFileSingleFile* GetFirst() { return reinterpret_cast<RezFileSingleFile*>(BaseList::GetFirst()); }
	RezFileSingleFile* GetLast() { return reinterpret_cast<RezFileSingleFile*>(BaseList::GetLast()); }
};

class BaseRezFile : public Common::BaseListItem<BaseRezFile>
{
public:
	BaseRezFile(RezMgr* rezMgr);
	virtual ~BaseRezFile();

	virtual unsigned long Read(unsigned long itemPos, unsigned long itemOffset, unsigned long size, void* data) = 0;
	virtual unsigned long Write(unsigned long itemPos, unsigned long itemOffset, unsigned long size, void* data) = 0;
	virtual bool Open(const char* filename, bool readOnly, bool createNew) = 0;
	virtual bool Close() = 0;
	virtual bool Flush() = 0;
	virtual bool VerifyFileOpen() = 0;
	virtual const char* GetFileName() = 0;

protected:
	RezMgr* rezMgr_;
};

class RezFile : public BaseRezFile
{
public:
	RezFile(RezMgr* rezMgr);
	virtual ~RezFile();

	virtual unsigned long Read(unsigned long itemPos, unsigned long itemOffset, unsigned long size, void* data) override;
	virtual unsigned long Write(unsigned long itemPos, unsigned long itemOffset, unsigned long size, void* data) override;
	virtual bool Open(const char* filename, bool readOnly, bool createNew) override;
	virtual bool Close() override;
	virtual bool Flush() override;
	virtual bool VerifyFileOpen() override;
	virtual const char* GetFileName() override;

private:
	FILE *file_;
	char *filename_;
	bool readOnly_;
	unsigned long lastSeekPos_;
};

class RezFileDirectoryEmulation : public BaseRezFile
{
public:
	RezFileDirectoryEmulation(RezMgr* rezMgr, int maxOpenFiles);
	virtual ~RezFileDirectoryEmulation();

	virtual unsigned long Read(unsigned long itemPos, unsigned long itemOffset, unsigned long size, void* data) override;
	virtual unsigned long Write(unsigned long itemPos, unsigned long itemOffset, unsigned long size, void* data) override;
	virtual bool Open(const char* filename, bool readOnly, bool createNew) override;
	virtual bool Close() override;
	virtual bool Flush() override;
	virtual bool VerifyFileOpen() override;
	virtual const char* GetFileName() override;

private:
	friend class RezFileSingleFile;

	RezFileSingleFileList openFiles_;
	RezFileSingleFileList closedFiles_;
	int numOpenFiles_;
	int maxOpenFiles_;
	bool readOnly_;
	bool createNew_;
};

class RezFileSingleFile : public BaseRezFile
{
public:
	RezFileSingleFile(RezMgr *rezMgr, const char *filename, RezFileDirectoryEmulation *dirEmulation);
	virtual ~RezFileSingleFile();

	virtual unsigned long Read(unsigned long itemPos, unsigned long itemOffset, unsigned long size, void* data) override;
	virtual unsigned long Write(unsigned long itemPos, unsigned long itemOffset, unsigned long size, void* data) override;
	virtual bool Open(const char* filename, bool readOnly, bool createNew) override;
	virtual bool Close() override;
	virtual bool Flush() override;
	virtual bool VerifyFileOpen() override;
	virtual const char* GetFileName() override;

private:
	friend class RezFileDirectoryEmulation;

	bool ReallyOpen();
	bool ReallyClose();
	char* filename_;
	FILE* file_;
	RezFileDirectoryEmulation* dirEmulation_;
};

}}