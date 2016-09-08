#pragma once

#include "RezMgr/RezMgr.hpp"

namespace JupiterEx { namespace RezMgr {

// Takes a command string that can be passed into the RezCompiler, and determines
// if a specified flag is set
bool IsCommandSet(char flag, const char* command);

// Takes exact same parameters as the ZMGR utility
// Returns the number of resources added, updated, or extracted
//
// cmdLine refers to the command line options to control the compiler
// There can consist of
// c - Create
// v - View
// x - Extract
// f - freshen
// s - sort
// i - information
// v - Verbose
// z - Warn zero len
// l - Lower case ok
//
// so strings can look like cl, cv, c, etc.
//
// rezFilename is the filename of the Rez file that will be created
// targetDir is the name of the root directory of the resource hierarcky
int RezCompiler(const char* cmdLine, const char* rezFilename, const char* targetDir = nullptr,
				bool isLithRez = false, const char* fileSpec = "*.*");

}}