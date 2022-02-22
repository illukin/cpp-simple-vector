#pragma once

#include "array_ptr.h"

#include <cassert>
#include <cstdlib>
#include <stdexcept>
#include <initializer_list>
#include <iterator>
#include <utility>

using namespace std::string_literals;

class ReserveProxyObj {
private:
  size_t capacity_ = 0;

public:
  ReserveProxyObj() = default;
  explicit ReserveProxyObj(size_t capacity) : capacity_(capacity) {};

  [[nodiscard]] auto GetCapacity() const noexcept {
    return capacity_;
  }
};

ReserveProxyObj Reserve(size_t capacity_to_reserve) {
  return ReserveProxyObj(capacity_to_reserve);
}

template <typename Type>
class SimpleVector {
private:
  ArrayPtr<Type> a_;
  size_t size_ = 0;
  size_t capacity_ = 0;

  void ResizeForOneElement() {
    if (GetSize() == GetCapacity()) {
      Resize(GetSize() + 1);
    } else {
      ++size_;
    }
  }

  Type *GetPosToInsert(const Type* pos) {
    auto distance_to_pos = std::distance(cbegin(), pos);
    ResizeForOneElement();
    auto pos_to_insert = begin() + distance_to_pos;

    if (GetSize() > 1) {
      std::copy_backward(std::make_move_iterator(pos_to_insert),
        std::make_move_iterator(end() - 1), end());
    }

    return pos_to_insert;
  }

public:
  using Iterator = Type*;
  using ConstIterator = const Type*;

  SimpleVector() noexcept = default;

  // Создаёт вектор из size элементов, инициализированных значением по умолчанию
  explicit SimpleVector(size_t size) : SimpleVector(size, Type{}) {}

  // Создаёт вектор из size элементов, инициализированных значением value
  SimpleVector(size_t size, const Type& value)
    : a_(size), size_(size), capacity_(size) {
    std::fill(a_.Get(), a_.Get() + size, value);
  }

  // Создаёт вектор из std::initializer_list
  SimpleVector(std::initializer_list<Type> init)
    : a_(init.size()), size_(init.size()), capacity_(init.size()) {
    std::copy(init.begin(), init.end(), a_.Get());
  }

  SimpleVector(const SimpleVector& other) : a_(other.GetCapacity()),
    size_(other.GetSize()), capacity_(other.GetCapacity()) {
    std::copy(other.begin(), other.end(), begin());
  }

  SimpleVector(SimpleVector&& other) noexcept : a_(other.GetCapacity()),
    size_(other.GetSize()), capacity_(other.GetCapacity()) {
    std::copy(std::make_move_iterator(other.begin()),
      std::make_move_iterator(other.end()), begin());

    std::exchange(other.size_, 0);
    std::exchange(other.capacity_, 0);
  }

  explicit SimpleVector(const ReserveProxyObj obj)
    : a_(obj.GetCapacity()), size_(0), capacity_(obj.GetCapacity()) {}

  SimpleVector& operator=(const SimpleVector& rhs) {
    if (this != &rhs) {
      if (rhs.IsEmpty()) {
        Clear();
      } else {
        auto rhs_copy(rhs);

        swap(rhs_copy);
      }
    }

    return *this;
  }

  void Reserve(size_t new_capacity) {
    if (new_capacity > GetCapacity()) {
      ArrayPtr<Type> new_a(new_capacity);

      std::copy(begin(), end(), new_a.Get());
      a_.swap(new_a);
      capacity_ = new_capacity;
    }
  };

  // Добавляет элемент в конец вектора
  // При нехватке места увеличивает вдвое вместимость вектора
  void PushBack(const Type& item) {
    ResizeForOneElement();
    *(end() - 1) = item;
  }

  // Версия для объектов, которые нельзя копировать, но можно перемещать
  void PushBack(Type&& item) {
    ResizeForOneElement();
    std::exchange(*(end() - 1), std::move(item));
  }

  // Вставляет значение value в позицию pos.
  // Возвращает итератор на вставленное значение
  // Если перед вставкой значения вектор был заполнен полностью,
  // вместимость вектора должна увеличиться вдвое, а для вектора вместимостью 0 стать равной 1
  Iterator Insert(ConstIterator pos, const Type& value) {
    assert(pos >= begin() && pos <= end());

    auto pos_to_insert = GetPosToInsert(pos);

    *pos_to_insert = value;

    return pos_to_insert;
  }

  // Версия для объектов, которые нельзя копировать, но можно перемещать
  Iterator Insert(ConstIterator pos, Type&& value) {
    assert(pos >= begin() && pos <= end());

    auto pos_to_insert = GetPosToInsert(pos);

    std::exchange(*pos_to_insert, std::move(value));

    return pos_to_insert;
  }

  // "Удаляет" последний элемент вектора. Вектор не должен быть пустым
  void PopBack() noexcept {
    assert(!IsEmpty());

    --size_;
  }

  // Удаляет элемент вектора в указанной позиции
  Iterator Erase(ConstIterator pos) {
    assert(pos >= begin() && pos < end());

    auto distance_to_pos = std::distance(cbegin(), pos);
    auto pos_to_erase = begin() + distance_to_pos;

    std::copy(std::make_move_iterator(pos_to_erase + 1),
      std::make_move_iterator(end()), pos_to_erase);
    --size_;

    return pos_to_erase;
  }

  // Обменивает значение с другим вектором
  void swap(SimpleVector& other) noexcept {
    a_.swap(other.a_);
    std::swap(size_, other.size_);
    std::swap(capacity_, other.capacity_);
  }

// Возвращает количество элементов в массиве
  [[nodiscard]] size_t GetSize() const noexcept {
    return size_;
  }

  // Возвращает вместимость массива
  [[nodiscard]] size_t GetCapacity() const noexcept {
    return capacity_;
  }

  // Сообщает, пустой ли массив
  [[nodiscard]] bool IsEmpty() const noexcept {
    return GetSize() == 0;
  }

  // Возвращает ссылку на элемент с индексом index
  Type& operator[](size_t index) noexcept {
    assert(index < GetSize());

    return a_[index];
  }

  // Возвращает константную ссылку на элемент с индексом index
  const Type& operator[](size_t index) const noexcept {
    assert(index < GetSize());

    return a_[index];
  }

  // Возвращает константную ссылку на элемент с индексом index
  // Выбрасывает исключение std::out_of_range, если index >= size
  Type& At(size_t index) {
    if (index >= GetSize()) {
      throw std::out_of_range("index out of range"s);
    }

    return a_[index];
  }

  // Возвращает константную ссылку на элемент с индексом index
  // Выбрасывает исключение std::out_of_range, если index >= size
  const Type& At(size_t index) const {
    if (index >= GetSize()) {
      throw std::out_of_range("index out of range"s);
    }

    return a_[index];
  }

  // Обнуляет размер массива, не изменяя его вместимость
  void Clear() noexcept {
    size_ = 0;
  }

  // Изменяет размер массива.
  // При увеличении размера новые элементы получают значение по умолчанию для типа Type
  void Resize(size_t new_size) {
    if (new_size <= GetCapacity() ) {
      if (new_size > GetSize()) {
        for (auto it = begin() + GetSize(); it != begin() + new_size; ++it) {
          std::exchange(*it, std::move(Type{}));
        }
      }
    } else {
      auto new_capacity = std::max(GetCapacity() * 2, new_size);
      ArrayPtr<Type> new_a(new_capacity);

      std::copy(std::make_move_iterator(begin()), std::make_move_iterator(end()),
        new_a.Get());
      for (auto it = new_a.Get() + GetSize(); it != new_a.Get() + new_size; ++it) {
        std::exchange(*it, std::move(Type{}));
      }

      a_.swap(new_a);
      capacity_ = new_capacity;
    }
    size_ = new_size;
  }

  // Возвращает итератор на начало массива
  // Для пустого массива может быть равен (или не равен) nullptr
  Iterator begin() noexcept {
    return a_.Get();
  }

  // Возвращает итератор на элемент, следующий за последним
  // Для пустого массива может быть равен (или не равен) nullptr
  Iterator end() noexcept {
    return a_.Get() + GetSize();
  }

  // Возвращает константный итератор на начало массива
  // Для пустого массива может быть равен (или не равен) nullptr
  ConstIterator begin() const noexcept {
    return a_.Get();
  }

  // Возвращает итератор на элемент, следующий за последним
  // Для пустого массива может быть равен (или не равен) nullptr
  ConstIterator end() const noexcept {
    return a_.Get() + GetSize();
  }

  // Возвращает константный итератор на начало массива
  // Для пустого массива может быть равен (или не равен) nullptr
  ConstIterator cbegin() const noexcept {
    return a_.Get();
  }

  // Возвращает итератор на элемент, следующий за последним
  // Для пустого массива может быть равен (или не равен) nullptr
  ConstIterator cend() const noexcept {
    return a_.Get() + GetSize();
  }
};

template <typename Type>
inline bool operator==(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
  return &lhs == &rhs || (lhs.GetSize() == rhs.GetSize()
    && std::equal(lhs.begin(), lhs.end(), rhs.begin()));
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
  return rhs <= lhs;
}
