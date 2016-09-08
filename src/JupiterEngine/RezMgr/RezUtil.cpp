#include "RezMgr/RezMgr.hpp"

#include <conio.h>
#include <io.h>
#include <direct.h>

#include <string.h>
#include <assert.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#define LithTechUserTitle "LithTech Resource File"
#define kMaxStr 2048
#define zprintf printf

namespace JupiterEx { namespace RezMgr {

// if we are running the special LithRez version
bool g_IsLithRez = false;

RezMgr* g_Mgr = nullptr;

bool g_Verbose = false;
bool g_CheckZeroLen = false;

long g_DirCount = 0;
long g_RezCount = 0;
long g_ErrCount = 0;
long g_WarnCount = 0;
bool g_LowerCaseUsed = false;
bool g_ExitOnDiskError = false;

class ZMgrRezMgr : public RezMgr
{
public:
	virtual bool DiskError() override;
};

bool ZMgrRezMgr::DiskError()
{
	zprintf("ERROR! Disk error has occured (possibly drive is full).\n");
	g_ErrCount++;
	return false;
}

static bool ExtCheck(const char* sExtensions, const char*  sExt)
{
	const unsigned int ExtSize = 255;
	char szExtensions[ExtSize+1];
	szExtensions[0] = '\0';
	strncpy(szExtensions, sExtensions, ExtSize);
	szExtensions[ExtSize] = '\0';

	char *p = strtok(szExtensions, ";");
	while (p)
	{
		if (strcmp(p, "*.*") == 0)
			return true;

		if (_stricmp(p, sExt) == 0)
			return true;

		p = strtok(NULL, ";");
	}

	return false;
}

// Extracts a directory full of resources into files
static void ExtractDir(RezDir* rezDir, const char* paramPath)
{
	assert(rezDir != nullptr);
	assert(paramPath != nullptr);

	if (g_Verbose) zprintf("\nExtracting resource directory %s to %s\n", rezDir->GetDirName(), paramPath);

	// increment directories processed counter
	g_DirCount++;

	// figure out the path to this dir with added backslash
	char sPath[kMaxStr];
	strcpy(sPath, paramPath);
	if (sPath[strlen(sPath)-1] != '\\') strcat(sPath, "\\");

	// create the directory (just in cast it doesn't exist)
	_mkdir(sPath);

	// search through all types in this dir
	RezType* rezType = rezDir->GetFirstType();
	while (rezType != nullptr)
	{
		// create the string version of this type
		char sType[5];
		g_Mgr->TypeToStr(rezType->GetType(), sType);

		// search through all resource of this type
		RezItem* rezItem = rezDir->GetFirstItem(rezType);
		while (rezItem != nullptr)
		{
			// read in item data from resource
			unsigned char* pData = nullptr;
			if (rezItem->GetSize() > 0) pData = rezItem->Load();
			if ((pData != nullptr) || (rezItem->GetSize() == 0))
			{
				// figure out file name and path for data file
				char sFileName[kMaxStr];
				strcpy(sFileName, sPath);
				strcat(sFileName, rezItem->GetName());
				strcat(sFileName, ".");
				strcat(sFileName, sType);

				if (g_Verbose) zprintf("Extracting: Type = %-4s Name = %-12s Size = %-8i\n", sType, rezItem->GetName(), (int)rezItem->GetSize());
			
				g_RezCount++;

				FILE* fp = fopen(sFileName, "wb");
				if (fp != NULL)
				{
					if (rezItem->GetSize() > 0)
					{
						if (fwrite(pData, rezItem->GetSize(), 1, fp) != 1)
						{
							zprintf("ERROR! Unable to write file: %s\n", sFileName);
							g_ErrCount++;
						}
					}

					fclose(fp);
				}
				else
				{
					zprintf("ERROR! Unable to create file: %s\n", sFileName);
					g_ErrCount++;
				}

				rezItem->UnLoad();
			}
			else
			{
				zprintf("ERROR! Unable to load resource. Name = %s\n", rezItem->GetName());
				g_ErrCount++;
				continue;
			}

			rezItem = rezDir->GetNextItem(rezItem);
		}

		rezType = rezDir->GetNextType(rezType);
	}

	// search through all directories in this directory and recursivly call ExtractDir
	RezDir* subDirItem = rezDir->GetFirstSubDir();
	while (subDirItem != nullptr)
	{
		char sDir[kMaxStr];
		strcpy(sDir, sPath);
		strcat(sDir, subDirItem->GetDirName());
		strcat(sDir, "\\");

		ExtractDir(subDirItem, sDir);

		subDirItem = rezDir->GetNextSubDir(subDirItem);
	}
}

// Display the contents of a resource file
static void ViewDir(RezDir* pDir, const char* sParamPath)
{
	assert(pDir != nullptr);
	assert(sParamPath != nullptr);

	zprintf("\nDirectory %s :\n", pDir->GetDirName());

	g_DirCount++;

	// figure out the path to this dir with added backslash
	char sPath[kMaxStr];
	strcpy(sPath, sParamPath);
	if (sPath[strlen(sPath)-1] != '\\') strcat(sPath, "\\");

	// search through all types in this dir
	RezType* pType = pDir->GetFirstType();
	while (pType != nullptr)
	{
		char sType[5];
		g_Mgr->TypeToStr(pType->GetType(), sType);

		RezItem* pItem = pDir->GetFirstItem(pType);
		while (pItem != nullptr)
		{
			zprintf("  Type = %-4s Name = %-12s Size = %-8i\n", sType, pItem->GetName(), (int)pItem->GetSize());

			g_RezCount++;

			pItem = pDir->GetNextItem(pItem);
		}

		pType = pDir->GetNextType(pType);
	}

	// search through all directories in this directory and recursivly call ViewDir
	RezDir* pLoopDir = pDir->GetFirstSubDir();
	while (pLoopDir != nullptr)
	{
		char sDir[kMaxStr];
		strcpy(sDir, sPath);
		strcat(sDir, pLoopDir->GetDirName());
		strcat(sDir, "\\");

		ViewDir(pLoopDir, sDir);

		pLoopDir = pDir->GetNextSubDir(pLoopDir);
	}
}

// Transfers a directory full of files into the resource file
static void TransferDir(RezDir* pDir, const char* sParamPath, const char* sExts)
{
	assert(pDir != nullptr);
	assert(sParamPath != nullptr);
	_finddata_t fileinfo;
	unsigned long nMiscID = 10000000;

	if (g_Verbose) zprintf("\nCreating resource directory %s from %s\n", pDir->GetDirName(), sParamPath);

	g_DirCount++;

	// figure out the path to this dir with added backslash
	char sPath[kMaxStr];
	strcpy(sPath, sParamPath);
	if (sPath[strlen(sPath)-1] != '\\') strcat(sPath, "\\");

	// figure otu the find search string by adding *.* to search for everthing
	char sFindPath[kMaxStr];
	strcpy(sFindPath, sPath);
	strcat(sFindPath, "*.*");

	// being search for everything in this directory using findfirst and findnext
	long nFindHandle = _findfirst(sFindPath, &fileinfo);
	if (nFindHandle >= 0)
	{
		do
		{
			if (strcmp(fileinfo.name, ".") == 0) continue;
			if (strcmp(fileinfo.name, "..") == 0) continue;

			if (strlen(fileinfo.name) == 0)
			{
				zprintf("WARNING! Skipping file encountered with no base name.\n");
				g_WarnCount++;
				continue;
			}

			// if this is a subdirectory
			if ((fileinfo.attrib & _A_SUBDIR) == _A_SUBDIR)
			{
				// figure out the base name
				char sBaseName[kMaxStr];
				strcpy(sBaseName, fileinfo.name);
				if (!g_LowerCaseUsed) _strupr(sBaseName);

				// figure out the path name we are working on
				char sPathName[kMaxStr];
				strcpy(sPathName, sPath);
				strcat(sPathName, sBaseName);
				strcat(sPathName, "\\");

				RezDir* pNewDir = pDir->CreateDir(sBaseName);
				if (pNewDir == nullptr)
				{
					zprintf("ERROR! Unable to create directory. Name = %s\n", sBaseName);
					g_ErrCount++;
					continue;
				}

				TransferDir(pNewDir, sPathName, sExts);
			}
			else
			{
				// figure out the file name we are working on
				char sFileName[kMaxStr];
				strcpy(sFileName, sPath);
				strcat(sFileName, fileinfo.name);

				if (g_CheckZeroLen)
				{
					if (fileinfo.size <= 0)
					{
						zprintf("WARNING! Zero length file %s\n", sFileName);
						g_WarnCount++;
					}
				}

				// split the file name up into its parts
				char drive[_MAX_DRIVE+1];
				char dir[_MAX_DIR+1];
				char fname[_MAX_FNAME+1];
				char ext[_MAX_EXT+1];
				_splitpath(sFileName, drive, dir, fname, ext);

				char extCheck[_MAX_EXT+2] = "*";
				strcat(extCheck, ext);

				if (!ExtCheck(sExts, extCheck))
					continue;

				// figure out the Name for this file
				char sName[kMaxStr];
				assert((_MAX_FNAME+_MAX_EXT) <= kMaxStr);
				strcpy(sName, fname);

				bool bExtensionTooLong = false;
				if (strlen(ext) > 5)
				{
					bExtensionTooLong = true;
					strcat(sName, ext);

					zprintf("WARNING! Filename %s extension too long. Extension will be contained in name.\n", sFileName);
					g_WarnCount++;
				}

				if (!g_LowerCaseUsed) _strupr(sName);

				unsigned long nID;
				{
					int i;
					int nNameLen = (int)strlen(sName);
					for (i = 0; i < nNameLen; ++i)
					{
						if ((sName[i] < '0') || (sName[i] > '9')) break;
					}
					if (i < nNameLen)
					{
						nID = nMiscID;
						nMiscID++;
					}
					else
					{
						nID = atol(sName);
					}
				}

				// figure out the Type for this file
				char sExt[5];
				unsigned long nType;
				if (!bExtensionTooLong)
				{
					if (strlen(ext) > 0)
					{
						strcpy(sExt, &ext[1]);
						_strupr(sExt);
						nType = g_Mgr->StrToType(sExt);
					}
					else
					{
						nType = 0;
					}
				}
				else
				{
					nType = 0;
				}

				// convert type back to string
				char sType[5];
				g_Mgr->TypeToStr(nType, sType);

				if (g_Verbose) zprintf("Adding: Type = %-4s Name = %-12s Size = %-8i ID = %-8i\n", sType, sName, (int)fileinfo.size, (int)nID);

				RezItem* pItem = pDir->CreateRez(nID, sName, nType);
				if (pItem == nullptr)
				{
					zprintf("ERROR! Unable to create resource from file %s. Rez Name = %s, ID = %i\n", sFileName, sName, (int)nID);
					g_ErrCount++;
					continue;
				}

				g_RezCount++;
				pItem->SetTime((unsigned long)fileinfo.time_write);

				unsigned char* pData = pItem->Create(fileinfo.size);
				FILE* fp = fopen(sFileName, "rb");
				if (fp != nullptr)
				{
					if (fileinfo.size > 0)
					{
						if (fread(pData, fileinfo.size, 1, fp) == 1)
						{
							pItem->Save();
						}
						else
						{
							zprintf("ERROR! Unable to read file: %s\n", sFileName);
							g_ErrCount++;
						}
					}

					fclose(fp);
				}
				else
				{
					zprintf("ERROR! Unable to open file: %s\n", sFileName);
					g_ErrCount++;
				}

				pItem->UnLoad();
			}
		} while (_findnext(nFindHandle, &fileinfo) == 0);

		_findclose(nFindHandle);
	}
}

// Freshen a directory full of files into the resource file
static void FreshenDir(RezDir* pDir, const char* sParamPath)
{
	assert(pDir != nullptr);
	assert(sParamPath != nullptr);
	_finddata_t fileinfo;

	if (g_Verbose) zprintf("\nFreshening resource directory %s from %s\n", pDir->GetDirName(), sParamPath);

	g_DirCount++;

	// figure out the path to this dir with added backslash
	char sPath[kMaxStr];
	strcpy(sPath, sParamPath);
	if (sPath[strlen(sPath)-1] != '\\') strcat(sPath, "\\");

	// figure out the find search string by add *.* to search for everything
	char sFindPath[kMaxStr];
	strcpy(sFindPath, sPath);
	strcat(sFindPath, "*.*");

	// being search for everything in this directory using findfirst and findnext
	long nFindHandle = _findfirst(sFindPath, &fileinfo);
	if (nFindHandle >= 0)
	{
		do
		{
			if (strcmp(fileinfo.name, ".") == 0) continue;
			if (strcmp(fileinfo.name, "..") == 0) continue;

			if (strlen(fileinfo.name) == 0)
			{
				zprintf("WARNING! Skipping file encountered with no base name.\n");
				g_WarnCount++;
				continue;
			}

			if ((fileinfo.attrib & _A_SUBDIR) == _A_SUBDIR)
			{
				char sBaseName[kMaxStr];
				strcpy(sBaseName, fileinfo.name);
				if (!g_LowerCaseUsed) _strupr(sBaseName);

				char sPathName[kMaxStr];
				strcpy(sPathName, sPath);
				strcat(sPathName, sBaseName);
				strcat(sPathName, "\\");

				RezDir* pNewDir = pDir->GetDir(sBaseName);
				if (pNewDir == nullptr)
				{
					pNewDir = pDir->CreateDir(sBaseName);
					if (pNewDir == nullptr)
					{
						zprintf("ERROR! Unable to create directory. Name = %s\n", sBaseName);
						g_ErrCount++;
						continue;
					}
				}

				FreshenDir(pNewDir, sPathName);
			}
			else
			{
				char sFileName[kMaxStr];
				strcpy(sFileName, sPath);
				strcat(sFileName, fileinfo.name);

				if (g_CheckZeroLen)
				{
					if (fileinfo.size <= 0)
					{
						zprintf("WARNING! Zero length file %s\n", sFileName);
						g_WarnCount++;
					}
				}

				char drive[_MAX_DRIVE+1];
				char dir[_MAX_DIR+1];
				char fname[_MAX_FNAME+1];
				char ext[_MAX_EXT+1];
				_splitpath(sFileName, drive, dir, fname, ext);

				char sName[kMaxStr];
				assert(strlen(fname) < kMaxStr);
				strcpy(sName, fname);

				bool bExtensionTooLong = false;
				if (strlen(ext) > 5)
				{
					bExtensionTooLong = true;
					strcat(sName, ext);
					zprintf("WARNING! Filename %s extrension too long. Extension will be contained in name.\n", sFileName);
					g_WarnCount++;
				}
				if (!g_LowerCaseUsed) _strupr(sName);

				// figure out the Type for this file
				char sExt[5];
				unsigned long nType;
				if (!bExtensionTooLong)
				{
					if (strlen(ext) > 0)
					{
						strcpy(sExt, &ext[1]);
						_strupr(sExt);
						nType = g_Mgr->StrToType(sExt);
					}
					else
					{
						nType = 0;
					}
				}
				else
				{
					nType = 0;
				}

				// figure out the ID for this file (if name is all digits use it as ID number, otherwise assign a number)
				unsigned long nID;
				{
					int nNameLen = strlen(sName);
					int i;
					for (i = 0; i < nNameLen; ++i)
					{
						if ((sName[i] < '0') || (sName[i] > '9')) break;
					}
					if (i < nNameLen)
					{
						nID = g_Mgr->GetNextIDNumToUse();
						g_Mgr->SetNextIDNumToUse(nID + 1);
					}
					else
					{
						nID = atol(sName);
					}
				}

				// convert type back to string
				char sType[5];
				g_Mgr->TypeToStr(nType, sType);

				// see if this resource exists alread, and if so does it need to be updated
				RezItem* pItem = pDir->GetRez(sName, nType);
				unsigned long nFileTime = (unsigned long)fileinfo.time_write;
				if (pItem != nullptr)
				{
					// check timestamp to see if we need to update this resource (if not on to the next file!)
					if (nFileTime <= pItem->GetTime()) continue;
					if (g_Verbose) zprintf("Update: Type = %-4s Name = %-12s Size = %-8i ID = %-8i\n", sType, sName, (int)fileinfo.size, (int)nID);
				}
				else
				{
					if (g_Verbose) zprintf("Adding: Type = %-4s Name = %-12s Size = %-8i ID = %-8i\n", sType, sName, (int)fileinfo.size, (int)nID);
				
					pItem = pDir->CreateRez(nID, sName, nType);
					if (pItem == nullptr)
					{
						zprintf("ERROR! Unable to create resource form file %s. Rez Name = %s ID = %i\n", sFileName, sName, (int)nID);
						g_ErrCount++;
						continue;
					}
				}

				g_RezCount++;
				unsigned char* pData = pItem->Create(fileinfo.size);
				pItem->SetTime(nFileTime);

				FILE* fp = fopen(sFileName, "rb");
				if (fp != nullptr)
				{
					if (fileinfo.size > 0)
					{
						if (fread(pData, fileinfo.size, 1, fp) == 1)
						{
							pItem->Save();
						}
						else
						{
							zprintf("ERROR! Unable to read file: %s\n", sFileName);
							g_ErrCount++;
						}
					}

					fclose(fp);
				}
				else
				{
					zprintf("ERROR! Unable to open file: %s\n", sFileName);
					g_ErrCount++;
				}

				pItem->UnLoad();
			}
		} while (_findnext(nFindHandle, &fileinfo) == 0);

		_findclose(nFindHandle);
	}
}

static void NotifyErrWarn()
{
	if (g_ErrCount > 0) zprintf("\n%i ERRORS HAVE OCCURED!!!\n", g_ErrCount);
	if (g_WarnCount > 0) zprintf("\n%i WARNINGS HAVE OCCURED!!!\n", g_WarnCount);
}

static bool CheckLithHeader(RezMgr* pMgr)
{
	// if the LithRez flag is not set then don't even check we are OK
	if (!g_IsLithRez) return true;

	// check the header
	char* sCheckTitle = pMgr->GetUserTitle();
	if (sCheckTitle != nullptr)
	{
		if (strcmp(sCheckTitle, LithTechUserTitle) == 0)
		{
			return true;
		}
	}

	// if the header check failed then check for special shogo & blood2 files
	RezDir* pDir = pMgr->GetRootDir();
	if (pDir->GetRezFromDosPath("cshell.dll") != nullptr) return true;
	if (pDir->GetRezFromDosPath("cres.dll") != nullptr) return true;
	if (pDir->GetRezFromDosPath("sres.dll") != nullptr) return true;
	if (pDir->GetRezFromDosPath("object.lto") != nullptr) return true;
	if (pDir->GetRezFromDosPath("patch.txt") != nullptr) return true;
	if (pDir->GetRezFromDosPath("sounds\\dirtypesounds.") != nullptr) return true;
	if (pDir->GetRezFromDosPath("blood2.dep") != nullptr) return true;
	if (pDir->GetRezFromDosPath("riot.dep") != nullptr) return true;

	zprintf("ERROR! Not a LithTech resource file!\n");
	return false;
}

bool IsCommandSet(char cFlag, const char* pszCommand)
{
	int nCommandLen = (int)strlen(pszCommand);
	char cUpperFlag = toupper(cFlag);
	for (int i = 0; i < nCommandLen; ++i)
	{
		if (cUpperFlag == toupper(pszCommand[i]))
		{
			return true;
		}
	}
	return false;
}

int RezCompilter(const char* sCmd, const char* sRezFile, const char* sTargetDir, bool bLithRez, const char* sFilespec)
{
	assert(sCmd != nullptr);
	assert(sRezFile != nullptr);

	// initialize global variables
	g_Mgr             = nullptr;
	g_Verbose         = false;
	g_DirCount        = 0;
	g_RezCount        = 0;
	g_ErrCount        = 0;
	g_WarnCount       = 0;
	g_LowerCaseUsed   = false;
	g_IsLithRez       = bLithRez;

	g_Verbose         = IsCommandSet('V', sCmd);
	g_CheckZeroLen    = IsCommandSet('Z', sCmd);
	g_LowerCaseUsed   = IsCommandSet('L', sCmd);

	// default the command to information, but check for others
	char Command = 'I';
	if (IsCommandSet('V', sCmd)) Command = 'V';
	if (IsCommandSet('X', sCmd)) Command = 'X';
	if (IsCommandSet('C', sCmd)) Command = 'C';
	if (IsCommandSet('F', sCmd)) Command = 'F';
	if (IsCommandSet('S', sCmd)) Command = 'S';

	switch (Command)
	{
	case 'X': // extract
		{
			if (sTargetDir == nullptr)
			{
				zprintf("ERROR! Target directory missing.\n");
				g_ErrCount++;
				return 0;
			}

			ZMgrRezMgr Mgr;
			Mgr.SetItemByIDUsed(true);
			Mgr.Open(sRezFile);
			g_Mgr = &Mgr;

			if (!CheckLithHeader(g_Mgr)) break;

			RezDir* pDir = Mgr.GetRootDir();
			zprintf("\nExtracting rez file %s to directory %s\n", sRezFile, sTargetDir);

			ExtractDir(pDir, sTargetDir);

			if (g_Verbose) zprintf("\n");
			zprintf("Finished extracting %i directories %i resources\n", g_DirCount, g_RezCount);

			NotifyErrWarn();
			Mgr.Close();
		}
		break;

	case 'V': // view
		{
			ZMgrRezMgr Mgr;
			Mgr.SetItemByIDUsed(true);
			Mgr.Open(sRezFile);
			g_Mgr = &Mgr;

			if (!CheckLithHeader(g_Mgr)) break;

			RezDir* pDir = Mgr.GetRootDir();

			zprintf("\nView resource file %s\n", sRezFile);

			ViewDir(pDir, "");

			zprintf("\n");
			zprintf("File contains %i directories %i resources\n", g_DirCount, g_RezCount);

			NotifyErrWarn();
			Mgr.Close();
		}
		break;

	case 'C': // create
		{
			if (sTargetDir == nullptr)
			{
				zprintf("ERROR! Target directory missing.\n");
				g_ErrCount++;
				return 0;
			}

			ZMgrRezMgr Mgr;
			Mgr.SetItemByIDUsed(true);
			if (!Mgr.Open(sRezFile, false, true))
			{
				return  0;
			}
			g_Mgr = &Mgr;

			if (g_IsLithRez)
			{
				g_Mgr->SetUserTitle(LithTechUserTitle);
			}

			RezDir* pDir = Mgr.GetRootDir();

			zprintf("\nCreating rez file %s from directory %s\n", sRezFile, sTargetDir);

			TransferDir(pDir, sTargetDir, sFilespec);

			if (g_Verbose) zprintf("\n");
			zprintf("Finished creating %i directories %i resources\n", g_DirCount, g_RezCount);

			Mgr.ForceIsSortedFlag(true);
			NotifyErrWarn();
			Mgr.Close();
		}
		break;

	case 'F': // freshen command
		{
			if (g_IsLithRez) break;

			if (sTargetDir == nullptr)
			{
				zprintf("ERROR! Target directory missing.\n");
				g_ErrCount++;
				return 0;
			}

			ZMgrRezMgr Mgr;
			Mgr.SetItemByIDUsed(true);
			Mgr.Open(sRezFile, false);
			g_Mgr = &Mgr;

			if (!CheckLithHeader(g_Mgr)) break;

			RezDir* pDir = Mgr.GetRootDir();

			zprintf("\nFreshening rez file %s from directory %s\n", sRezFile, sTargetDir);

			FreshenDir(pDir, sTargetDir);

			if (g_Verbose) zprintf("\n");
			zprintf("Finished freshening %i directories %i resources\n", g_DirCount, g_RezCount);

			NotifyErrWarn();
			Mgr.Close();
		}
		break;

	case 'S':
		{
			if (g_IsLithRez) break;

			ZMgrRezMgr Mgr;
			Mgr.SetItemByIDUsed(true);
			Mgr.Open(sRezFile);

			if (!CheckLithHeader(g_Mgr)) break;

			zprintf("\nSorting %s\n", sRezFile);
			Mgr.Close(true);
		}
		break;

	case 'I':
		{
			if (g_IsLithRez) break;

			ZMgrRezMgr Mgr;
			Mgr.SetItemByIDUsed(true);
			if (!Mgr.Open(sRezFile))
			{
				zprintf("\nFailed to open file %s\n", sRezFile);
				break;
			}

			zprintf("\n Rez Fiel %s\n", sRezFile);
			if (Mgr.IsSorted()) zprintf("Is Sorted = TRUE\n");
			else zprintf("Is Sorted = FALSE\n");
			zprintf("\n");

			NotifyErrWarn();
			Mgr.Close();
		}
		break;

	default:
		return 0;
		break;
	}

	return g_RezCount;
}

}}