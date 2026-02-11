#pragma once
#include <iostream>
#include <vector>
#include <deque>
#include <memory>

/* Shared pointers */
template<typename T>
class Ref {
public:
	Ref() = default;
	Ref(std::shared_ptr<T> ptr) : m_ptr(ptr) {}

	T* operator->() { return this->m_ptr.get(); }
	const T* operator->() const { return this->m_ptr.get(); }

	T& operator*() { return *this->m_ptr; }
	const T& operator*() const { return *this->m_ptr; }

	explicit operator bool() const { return this->m_ptr != nullptr; }

	std::shared_ptr<T> Get() const { return this->m_ptr; }

	template<typename U>
	Ref<U> 
	As() const {
		return std::static_pointer_cast<U>(this->m_ptr);
	}
private:
	std::shared_ptr<T> m_ptr;
};

template<typename T>
using WeakRef = std::weak_ptr<T>;

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

using String = std::string;