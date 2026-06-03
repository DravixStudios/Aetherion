#pragma once
#include <iostream>
#include <vector>
#include <map>
#include <unordered_map>
#include <deque>
#include <memory>

/* Pointer type aliases */
template<typename T>
using WeakRef = std::weak_ptr<T>;

template<typename T>
using SharedPtr = std::shared_ptr<T>;

template<typename T>
using UniquePtr = std::unique_ptr<T>;

/* Shared pointers */
template<typename T>
class Ref {
public:
	Ref() = default;
	Ref(SharedPtr<T> ptr) : m_ptr(ptr) {}
	Ref(nullptr_t) : m_ptr(nullptr) {}

	T* operator->() { return this->m_ptr.get(); }
	const T* operator->() const { return this->m_ptr.get(); }

	T& operator*() { return *this->m_ptr; }
	const T& operator*() const { return *this->m_ptr; }
	
	Ref<T>& operator=(nullptr_t) {
		this->m_ptr = nullptr;
		return *this;
	}

	explicit operator bool() const { return this->m_ptr != nullptr; }
	bool IsValid() const { return this->m_ptr != nullptr; }

	SharedPtr<T> Get() const { return this->m_ptr; }

	template<typename U>
	Ref<U> 
	As() const {
		return std::static_pointer_cast<U>(this->m_ptr);
	}

private:
	SharedPtr<T> m_ptr;
};


template<typename T, typename... Args>
Ref<T> 
CreateRef(Args&&... args) {
	return Ref<T>(std::make_shared<T>(std::forward<Args>(args)...));
}

/* Common aliases */
template<typename T>
using Vector = std::vector<T>;

template<typename T>
using Deque = std::deque<T>;

template<typename K, typename V>
using Map = std::map<K, V>;

template<typename K, typename V>
using HashMap = std::unordered_map<K, V>;

using String = std::string;

using Byte = unsigned char;

/* Fixed string struct */
template<size_t N>
struct FixedString {
	char data[N] = { };

	FixedString() = default;

	FixedString(const char* pcData) {
		size_t len = std::min(std::strlen(pcData), N - 1);
		this->writtenBytes = len;
		std::strncpy(data, pcData, len);
		this->data[N - 1] = '\0';
	}

	FixedString(const String& str) {
		*this = str.c_str();
	}

	FixedString& operator=(const char* pcData) {
		std::strncpy(data, pcData, N - 1);
		this->data[N - 1] = '\0';
		return *this;
	}

	FixedString& operator=(const String& str) {
		return *this = str.c_str();
	}

	String string() const {
		return String(this->data);
	}

	operator const char* () const { return data; }


	size_t Length() const {
		return this->writtenBytes;
	}

private:
	size_t writtenBytes = 0;
};

using Name = FixedString<64>;
using Text = FixedString<128>;
using Path = FixedString<256>;