#include "RezMgr/RezUtil.hpp"

#include <stdlib.h>
#include <stdio.h>
#include <conio.h>
#include <io.h>
#include <direct.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>

#define kMaxStr 2048

using namespace JupiterEx::RezMgr;

unsigned long int GetFileSize(const char* sFileName)
{
	struct _stat buf;
	int fh, result;
	char buffer[] = "A line to output";

	if ((fh = _open(sFileName, _O_RDONLY)) == -1)
		return 0;

	result = _fstat(fh, &buf);
	_close(fh);

	if (result != 0)
	{
		printf("ERROR! Unable to get file size!\n");
		return 0;
	}

	return buf.st_size;
}

void DisplayHelp()
{
	printf("\nLITHREZ 1.10 (Apr-12-2004) Copyright (C) 2004 Touchdown Entertainment, Inc.\n");
	printf("\nUsage: LITHREZ <commands> <rez file name> [parameters]\n");
	printf("\nCommands: c <rez file name> <root directory to read> [extension[;]] - Create");
	printf("\n          v <rez file name>                          - View");
	printf("\n          x <rez file name> <directory to output to> - Extract");
	printf("\nOptions:  v                                          - Verbose");
	printf("\n          z                                          - Warn zero len");
	printf("\n          l                                          - Lower case ok\n");
	printf("\nExample: LithRez.exe cv foo.rez c:\\foo *.ltb;*.dat;*.dtx");
	printf("\n         (sould create rez file foo.rez from the contenst of the");
	printf("\n          directory \"c:\\foo\" where files with extensions ltb dat and");
	printf("\n          dtx are added, the verbose option would be turned on)\n\n");
}

int main(int argc, char *argv[], char *envp[])
{
	if (argc < 3)
	{
		DisplayHelp();
		return 1;
	}

	char cModeParam = toupper(argv[1][0]);
	if ((argc < 4) && (IsCommandSet('X', argv[1]) || IsCommandSet('C', argv[1])))
	{
		DisplayHelp();
		return 1;
	}

	char sRezFile[kMaxStr];
	strcpy(sRezFile, argv[2]);
	_strupr(sRezFile);
	char sExt[] = "*.*";

	if (argc < 5)
	{
		int nNumItems = RezCompiler(argv[1], argv[2], argv[3], true, sExt);
	}
	else
	{
		int nNumItems = RezCompiler(argv[1], argv[2], argv[3], true, argv[4]);
	}

	printf("Rez File Size = %lu\n", GetFileSize(sRezFile));
	return 0;
}