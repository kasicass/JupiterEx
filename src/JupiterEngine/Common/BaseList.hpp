#pragma once

namespace JupiterEx { namespace Common {

template <class T>
class BaseList;

template <class T>
class BaseListItem
{
public:
	BaseListItem() {}
	virtual ~BaseListItem() {}

	T* Next() { return next_; }
	T* Prev() { return prev_; }

public:
	friend class BaseList<T>;

protected:
	T* next_;
	T* prev_;
};

template<class T>
class BaseList
{
public:
	BaseList() : first_(nullptr), last_(nullptr) {}
	virtual ~BaseList() {}

	void Insert(T* item) { InsertFirst(item); }
	void InsertFirst(T* item);
	void InsertLast(T* item);
	void InsertAfter(T* beforeItem, T* newItem);
	void InsertBefore(T* afterItem, T* newItem);
	void Delete(T* item);

	T* GetFirst() { return first_; }
	T* GetLast() { return last_; }

private:
	T* first_;
	T* last_;
};

template <class T>
void BaseList<T>::InsertFirst(T* item)
{
	item->next_ = first_;
	item->prev_ = nullptr;
	if (first_ != nullptr) first_->prev_ = item;
	else last_ = item;
	first_ = item;
}

template <class T>
void BaseList<T>::InsertLast(T* item)
{
	item->next_ = nullptr;
	item->prev_ = last_;
	if (last_ != nullptr) last_->next_ = item;
	else first_ = item;
	last_ = item;
}

template <class T>
void BaseList<T>::InsertAfter(T* beforeItem, T* newItem)
{
	if (beforeItem == nullptr) InsertFirst(newItem);
	if (beforeItem->next_ != nullptr) beforeItem->next_->prev_ = newItem;
	else last_ = newItem;
	newItem->prev_ = beforeItem;
	newItem->next_ = beforeItem->next_;
	beforeItem->next_ = newItem;
}

template <class T>
void BaseList<T>::InsertBefore(T* afterItem, T* newItem)
{
	if (afterItem == nullptr) InsertLast(newItem);
	if (afterItem->prev_ != nullptr) afterItem->prev_->next_ = newItem;
	else first_ = newItem;
	newItem->next_ = afterItem;
	newItem->prev_ = afterItem->prev_;
	afterItem->prev_ = newItem;
}

template <class T>
void BaseList<T>::Delete(T* item)
{
	if (item->prev_ != nullptr) item->prev_->next_ = item->next_;
	else first_ = item->next_;

	if (item->next_ != nullptr) item->next_->prev_ = item->prev_;
	else last_ = item->prev_;

	// item->next_ = nullptr;
	// item->prev_ = nullptr;
}

}}