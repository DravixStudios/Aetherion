#pragma once
#include <iostream>
#include <vector>
#include <memory>

/* Shared pointers */
template<typename T>
using Ref = std::shared_ptr<T>;

template<typename T>
using WeakRef = std::weak_ptr<T>;

template<typename T, typename... Args>
Ref<T> 
CreateRef(Args&&... args) {
	return std::make_shared<T>(std::forward<Args>(args)...);
}

/* Common aliases */
template<typename T>
using Vector = std::vector<T>;

using String = std::string;