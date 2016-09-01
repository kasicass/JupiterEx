#include "RezHash.hpp"

#if 0
namespace JupiterEx { namespace RezMgr {

//
// RezItemHashByName / RezItemHashTableByName
//

unsigned int RezItemHashByName::HashFunc()
{
	if (rezItem_ == nullptr) return 0;
	else return GetParentTable()->HashFunc(rezItem_->GetName());
}

}}
#endif