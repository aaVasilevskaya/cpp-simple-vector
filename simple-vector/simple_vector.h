#pragma once

#include <stdexcept>
#include <initializer_list>
#include <iostream>
#include <utility>
#include <algorithm>
#include <cassert>

#include "array_ptr.h"

class ReserveProxyObj {
public:
    explicit ReserveProxyObj(size_t capacity) : capacity_(capacity) {}
    size_t getCapacity() const { return capacity_; }

private:
    size_t capacity_;
};

ReserveProxyObj Reserve(size_t capacity_to_reserve) {
    return ReserveProxyObj(capacity_to_reserve);
}

template <typename Type>
class SimpleVector {
public:
    using Iterator = Type*;
    using ConstIterator = const Type*;

    SimpleVector() noexcept = default;

    // Создаёт вектор из size элементов, инициализированных значением по умолчанию
    explicit SimpleVector(size_t size)
    :capacity_(size), size_(size), vector_(capacity_) {
        for(size_t i = 0; i < size_; i++){
            vector_[i] = Type();
        }
    }

    // Создаёт вектор из size элементов, инициализированных значением value
    SimpleVector(size_t size, const Type& value)
    : capacity_(size), size_(size), vector_(capacity_) {
        std::fill(this->begin(), this->end(), value);
    }

    // Создаёт вектор из std::initializer_list
    SimpleVector(std::initializer_list<Type> init)
    :capacity_(init.size()),
     size_(init.size()),
     vector_(capacity_){
        size_t i =0;
        for(auto iter = init.begin(); iter != init.end(); iter++, i++){
            vector_[i] = Type(*iter);
        }  
    }

    SimpleVector(const SimpleVector& other)
    : capacity_(other.capacity_),
      size_(other.size_),
      vector_(capacity_){
        std::copy(other.begin(), other.end(), begin());
    }

    SimpleVector(SimpleVector&& other)
    : capacity_(other.capacity_),
      size_(other.size_),
      vector_(std::move(other.vector_)){
        other.size_ = 0;
        other.capacity_ = 0;
    }

    SimpleVector(const ReserveProxyObj& reserve_proxy) {
        Reserve(reserve_proxy.getCapacity());
    }

    void Reserve(size_t new_capacity){
        if(capacity_ < new_capacity){
            ResizeCapacity(new_capacity);
        }
    }

    size_t GetSize() const noexcept {
        return size_;
    }

    size_t GetCapacity() const noexcept {
        return capacity_;
    }

    bool IsEmpty() const noexcept {
        return (size_ == 0);
    }

    // Возвращает ссылку на элемент с индексом index
    Type& operator[](size_t index) noexcept {
        return vector_[index];
    }

    // Возвращает константную ссылку на элемент с индексом index
    const Type& operator[](size_t index) const noexcept {
        return vector_[index];
    }

    // Возвращает константную ссылку на элемент с индексом index
    // Выбрасывает исключение std::out_of_range, если index >= size
    Type& At(size_t index) {
        if(index >= size_) throw std::out_of_range("wrong index");
        return vector_[index];
    }

    // Возвращает константную ссылку на элемент с индексом index
    // Выбрасывает исключение std::out_of_range, если index >= size
    const Type& At(size_t index) const {
        if(index >= size_) throw std::out_of_range("wrong index");
        return vector_[index];
    }

    // Обнуляет размер массива, не изменяя его вместимость
    void Clear() noexcept {
        size_ = 0;
    }

    // Изменяет размер массива.
    // При увеличении размера новые элементы получают значение по умолчанию для типа Type
    void Resize(size_t new_size) {
        if(new_size < capacity_){
            size_ = new_size;
        }else{
            ResizeCapacity(std::max(new_size, size_*2));
            size_ = new_size;
        }
    }

    // Возвращает итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    Iterator begin() noexcept {
       return Iterator(&vector_[0]);
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    Iterator end() noexcept {
        return Iterator(&vector_[0] + size_);
    }

    // Возвращает константный итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator begin() const noexcept {
        return ConstIterator(&vector_[0]);
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator end() const noexcept {
        return ConstIterator(&vector_[0] + size_);
    }

    // Возвращает константный итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator cbegin() const noexcept {
       return ConstIterator(&vector_[0]);
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator cend() const noexcept {
        return ConstIterator(&vector_[0] + size_);
    }

    SimpleVector& operator=(const SimpleVector& rhs) {
        if (this != &rhs) { 
            SimpleVector<Type> tmp(rhs); 
            swap(tmp);
        }
        return *this;
    }

    SimpleVector& operator=(const SimpleVector&& rhs){
        if (this != &rhs) { 
            SimpleVector<Type> tmp(rhs); 
            swap(tmp);
        }
        return *this;
    }

    // Добавляет элемент в конец вектора
    // При нехватке места увеличивает вдвое вместимость вектора
    void PushBack(const Type& item) {
        if(size_ == capacity_){
            ResizeCapacity(size_ == 0 ? 1 : size_ * 2);
        }
        vector_[size_] = item;
        size_++;
    }

    void PushBack(Type&& item) {
        if(size_ == capacity_){
            ResizeCapacity(size_ == 0 ? 1 : size_ * 2);
        }
        vector_[size_] = std::move(item);
        size_++;
    }

    // Вставляет значение value в позицию pos.
    // Возвращает итератор на вставленное значение
    // Если перед вставкой значения вектор был заполнен полностью,
    // вместимость вектора должна увеличиться вдвое, а для вектора вместимостью 0 стать равной 1
    Iterator Insert(ConstIterator pos, const Type& value) {
        size_t index = static_cast<size_t>(std::distance(cbegin(), pos));
        assert(index <= size_);

        if(size_ == capacity_){
            ResizeCapacity(size_ == 0 ? 1 : size_ * 2);
        }
        std::copy_backward(&vector_[index], end(), end()+1);
        vector_[index] = value;
        size_++;
        return Iterator(&vector_[index]);
    }

    Iterator Insert(ConstIterator pos,Type&& value) {
        size_t index = static_cast<size_t>(std::distance(cbegin(), pos));
        assert(index < size_);

        if(size_ == capacity_){
            ResizeCapacity(size_ == 0 ? 1 : size_ * 2);
        }
        std::move_backward(&vector_[index], end(), end()+1);
        vector_[index] = std::move(value);
        size_++;
        return Iterator(&vector_[index]);
    }

    // "Удаляет" последний элемент вектора. Вектор не должен быть пустым
    void PopBack() noexcept {
        assert(!IsEmpty());
        size_--;
    }

    // Удаляет элемент вектора в указанной позиции
    Iterator Erase(ConstIterator pos) {
        assert(pos >= cbegin() && pos < cend());
        Type* erased_pos = const_cast<Type*>(pos);
        std::move(erased_pos + 1, this->end(), erased_pos);
        size_--;
        return erased_pos;
    }
    
    // Обменивает значение с другим вектором
    void swap(SimpleVector& other) noexcept {
        other.vector_.swap(this->vector_);
        std::swap(other.capacity_, this->capacity_);
        std::swap(other.size_, this->size_);
    }

private:

    void ResizeCapacity(size_t new_capacity) {
        ArrayPtr<Type> new_vector(new_capacity);
        std::generate(&new_vector[0], &new_vector[new_capacity], [](){ return Type(); });
        std::move(&vector_[0], &vector_[size_], &new_vector[0]);
        vector_.swap(new_vector);
        capacity_ = new_capacity;
    }

    size_t capacity_ = 0;
    size_t size_ = 0;
    ArrayPtr<Type> vector_;
};

template <typename Type>
inline bool operator==(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return std::equal(lhs.begin(), lhs.end(), rhs.begin());
}

template <typename Type>
inline bool operator!=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(lhs == rhs);
}

template <typename Type>
inline bool operator<(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

template <typename Type>
inline bool operator<=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(rhs < lhs);
}

template <typename Type>
inline bool operator>(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return rhs < lhs;
}

template <typename Type>
inline bool operator>=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(lhs < rhs);
} 