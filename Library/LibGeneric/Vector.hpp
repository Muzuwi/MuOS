#pragma once
#include <stddef.h>
#include <LibGeneric/InitializerList.hpp>

#ifdef __is_kernel_build__
#include <Kernel/Debug/kassert.hpp>
#endif

namespace gen {
    template<class T>
    class vector {
		T* m_base;
		size_t m_size;
	public:
		vector();
		vector(const vector<T> &vec);
		vector(std::initializer_list<T> list);
		~vector();
		void push_back(T value);

		void clear();
		void resize(size_t new_size);

		T& operator[](size_t index) const;
		void for_each(void (*callback)(T&));
		void for_each(void (*callback)(const T&)) const;
		size_t size() const;
		bool contains(T value) const;
		gen::vector<T> filter(bool (*call)(const T&));
	};
}

template<class T>
gen::vector<T>::vector() {
	m_base = nullptr;
	m_size = 0;
}

template<class T>
gen::vector<T>::vector(const vector<T> &vec) {
	m_base = new T[vec.m_size];
	m_size = vec.m_size;

	for(size_t i = 0; i < m_size; i++) {
		m_base[i] = vec[i];
	}
}

template<class T>
gen::vector<T>::vector(std::initializer_list<T> list) {
	m_size = list.size();
	m_base = new T[m_size];

	size_t i = 0;
	std::initializer_list<int>::iterator iterator;
	for(iterator = list.begin(); iterator != list.end(); ++iterator, ++i) {
		m_base[i] = *iterator;
	}
}

template<class T>
gen::vector<T>::~vector() {
	delete[] m_base;
	m_base = nullptr;
	m_size = 0;
}

template<class T>
void gen::vector<T>::push_back(T value) {
	if(!m_base) {
		m_base = new T(value);
		m_size = 1;
		return;
	}
	resize(m_size + 1);
	m_base[m_size-1] = value;
}


template<class T>
void gen::vector<T>::clear() {
	delete[] m_base;
	m_base = nullptr;
	m_size = 0;
}

template<class T>
void gen::vector<T>::resize(size_t new_size) {
	if(new_size <= m_size) return;

	T* old = m_base;
	m_base = new T[new_size];

	for(size_t i = 0; i < m_size; i++) {
		m_base[i] = old[i];
	}

	delete[] old;

	m_size = new_size;
}

template<class T>
T& gen::vector<T>::operator[](size_t index) const{
	assert(index < m_size);
	return m_base[index];
}

template<class T>
void gen::vector<T>::for_each(void (*callback)(T&)) {
	if(!callback) return;
	for(size_t i = 0; i < m_size; i++) {
		callback(m_base[i]);
	}
}

template<class T>
void gen::vector<T>::for_each(void (*callback)(const T&)) const {
	if(!callback) return;
	for(size_t i = 0; i < m_size; i++) {
		callback(m_base[i]);
	}
}

template<class T>
size_t gen::vector<T>::size() const {
	return m_size;
}

template<class T>
bool gen::vector<T>::contains(T value) const {
	for(size_t i = 0; i < m_size; i++) {
		if(m_base[i] == value) return true;
	}
	return false;
}

template<class T>
gen::vector<T> gen::vector<T>::filter(bool (*call)(const T&)) {
	if(!call) return gen::vector<T>();

	gen::vector<T> res;

	for(size_t i = 0; i < m_size; i++) {
		if(call(m_base[i]))
			res.push_back(m_base[i]);
	}

	return res;
}
