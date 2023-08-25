#pragma once
#include <vector>
#include <algorithm>
#include <iterator>


struct TArray
{

};


template <class T>
class Array
{
private:
	TArray m_BaseArray;
	std::vector<T> m_Data;

public:

	Array()
	{
		m_Data = std::vector<T>();
	}
	Array(size_t size)
	{
		m_Data = std::vector<T>(size);
	}
	Array(const std::vector<T>& vector)
	{
		m_Data = vector;
	}
	Array(const std::initializer_list<T>& vector)
	{
		m_Data = vector;
	}

	void Emplace(T item)
	{
		return m_Data.emplace_back(item);
	}
	void Add(T item)
	{
		m_Data.push_back(item);
	}

	void Insert(int index, T& item)
	{
		m_Data.insert(m_Data.begin() + index, item);
	}
	
	void SwapElements(int indexA, int indexB)
	{
		std::iter_swap(m_Data.begin() + indexA, m_Data.begin() + indexB);
	}

	inline T& Get(int index)
	{
		return m_Data[index];
	}
	inline T& operator[](int index)
	{
		return m_Data[index];
	}
	inline const T& operator[](int index) const
	{
		return m_Data[index];
	}

	int IndexOf(const T& item)
	{
		auto it = std::find(m_Data.begin(), m_Data.end(), item);

		if (it != m_Data.end()) return it - m_Data.begin();
		else return -1;
	}

	void RemoveAt(int index)
	{
		m_Data.erase(m_Data.begin() + index);
	}
	void Remove(const T& item)
	{
		RemoveAt(IndexOf(item));
	}

	bool Contains(const T& item) const
	{
		return std::find(m_Data.begin(), m_Data.end(), item) != m_Data.end();
	}

	Array<T> operator+(Array<T>& other)
	{
		std::vector<T> AB;
		AB.reserve(m_Data.size() + other.GetData().size()); // preallocate memory
		AB.insert(AB.end(), m_Data.begin(), m_Data.end());
		AB.insert(AB.end(), other.GetData().begin(), other.GetData().end());
		return { AB };
	}
	void operator+=(Array<T>& other)
	{
		*this = *this + other;
	}

	void Clear()
	{
		m_Data.clear();
	}

	inline int Size() const
	{
		return m_Data.size();
	}
	inline int Last() const
	{
		return m_Data.size() - 1;
	}

	std::vector<T>& GetData()
	{
		return m_Data;
	}

	void SetData(std::vector<T>& data)
	{
		this->m_Data = data;
	}

	// Iterators

	typename std::vector<T>::iterator begin() { return m_Data.begin(); }
	typename std::vector<T>::const_iterator begin() const { return m_Data.begin(); }
	typename std::vector<T>::const_iterator cbegin() const { return m_Data.cbegin(); }
	typename std::vector<T>::iterator end() { return m_Data.end(); }
	typename std::vector<T>::const_iterator end() const { return m_Data.end(); }
	typename std::vector<T>::const_iterator cend() const { return m_Data.cend(); }

};