#include "BaseHash.hpp"

namespace JupiterEx { namespace Common {

BaseHashItem* BaseHashItem::Next()
{
	BaseHashItem *item;
	item = BaseListItem::Next();
	if (item == nullptr)
	{
		unsigned int bin = currentBin_ + 1;
		while (bin < parentTable_->numBins_)
		{
			item = parentTable_->binArray_[bin].GetFirst();
			if (item != nullptr) break;
			++bin;
		}
	}
	return item;
}

BaseHashItem* BaseHashItem::Prev()
{
	BaseHashItem *item;
	item = BaseListItem::Prev();
	if (item == nullptr)
	{
		unsigned int bin = currentBin_;
		while (bin > 0)
		{
			--bin;
			item = parentTable_->binArray_[bin].GetLast();
			if (item != nullptr) break;
		}
	}
	return item;
}

BaseHashTable::BaseHashTable()
{
	numBins_  = 0;
	binArray_ = nullptr;
}

BaseHashTable::BaseHashTable(unsigned int numBins)
{
	numBins_  = numBins;
	binArray_ = new HashBin[numBins]; 
}

BaseHashTable::~BaseHashTable()
{
	if (binArray_ != nullptr) delete [] binArray_;
}

void BaseHashTable::Insert(BaseHashItem *item)
{
	item->parentTable_ = this;
	unsigned int currentBin = item->HashFunc();
	// ASSERT(currentBin < numBins_);
	item->currentBin_ = currentBin;
	binArray_[currentBin].InsertFirst(item);
}

void BaseHashTable::Delete(BaseHashItem *item)
{
	// ASSERT(item->currentBin_ < numBins_);
	binArray_[item->currentBin_].Delete(item);
}

BaseHashItem* BaseHashTable::GetFirst()
{
	unsigned int bin = 0;
	BaseHashItem *item;
	do
	{
		item = binArray_[bin].GetFirst();
		++bin;
	} while ((item == nullptr) && (bin < numBins_));
	return item;
}

BaseHashItem* BaseHashTable::GetLast()
{
	unsigned int bin = numBins_ - 1;
	BaseHashItem *item;
	do
	{
		item = binArray_[bin].GetLast();
		if (bin > 0) --bin;
		else break;
	} while (item == nullptr);
	return item;
}

BaseHashItem* BaseHashTable::GetFirstInBin(unsigned int bin)
{
	// ASSERT(bin < numBins_);
	return binArray_[bin].GetFirst();
}

}}