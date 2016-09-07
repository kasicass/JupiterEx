#include "JupiterEx.hpp"

using namespace JupiterEx::RezMgr;

void RezFileTest()
{
	RezMgr mgr;
	RezFile file(&mgr);

	file.Open("D:\\a.txt", false, true);
	file.Write(0, 0, 5, "hello");
	file.Close();
}