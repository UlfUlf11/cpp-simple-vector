#pragma once

#include <cassert>
#include <initializer_list>
#include <algorithm>
#include <stdexcept>
#include <iostream>
#include "array_ptr.h"

class ReserveProxyObj
{
public:
    ReserveProxyObj() = default;
    ReserveProxyObj(const size_t value)
        : capacity_to_reserve_(value)
    {
    }

    size_t GetCapacity() const
    {
        return capacity_to_reserve_;
    }

private:
    size_t capacity_to_reserve_ = 0u;
};

ReserveProxyObj Reserve(size_t capacity_to_reserve)
{
    return ReserveProxyObj(capacity_to_reserve);
}

template <typename Type>
class SimpleVector
{
public:
    using Iterator = Type*;
    using ConstIterator = const Type*;

    SimpleVector() noexcept = default;

    // Создаёт вектор из size элементов, инициализированных значением по умолчанию
    explicit SimpleVector(size_t size) : items_(size), size_(size), capacity_(size)
    {
        std::generate(items_.Get(), items_.Get() + size_, [] { return Type(); });
    }

    // Создаёт вектор из size элементов, инициализированных значением value
    SimpleVector(size_t size, const Type& value) : items_(size), size_(size), capacity_(size)
    {
        std::generate(items_.Get(), items_.Get() + size_, [value] {return value;});
    }

    // Создаёт вектор из std::initializer_list
    SimpleVector(std::initializer_list<Type> init) : items_(init.size()), size_(init.size()), capacity_(init.size())
    {
        std::copy(init.begin(), init.end(), items_.Get());
    }

    //конструктор копирования
    SimpleVector(const SimpleVector& other)
    {
        ArrayPtr<Type> temp(other.size_);
        std::copy(other.begin(), other.end(), temp.Get());
        items_.swap(temp);
        size_ = other.size_;
        capacity_ = other.capacity_;
    }

    //Конструктор с оберткой для Reserve
    SimpleVector(ReserveProxyObj capacity)
    {
        capacity_ = capacity.GetCapacity();
    }

    // Резервирование памяти
    void Reserve(size_t new_capacity)
    {
        if (new_capacity > capacity_)
        {
            ArrayPtr<Type> temp(new_capacity);
            std::copy(begin(), end(), temp.Get());
            items_.swap(temp);
            capacity_ = new_capacity;
        }
    }

    SimpleVector& operator=(const SimpleVector& rhs)
    {
        //если не присваиваем объект самому себе
        if (*this != rhs)
        {
            SimpleVector temp(rhs);
            swap(temp);
        }
        return *this;
    }

    // Возвращает количество элементов в массиве
    size_t GetSize() const noexcept
    {
        return size_;
    }

    // Возвращает вместимость массива
    size_t GetCapacity() const noexcept
    {
        return capacity_;
    }

    // Сообщает, пустой ли массив
    bool IsEmpty() const noexcept
    {
        return size_ == 0;
    }

    // Возвращает ссылку на элемент с индексом index
    Type& operator[](size_t index) noexcept
    {
        assert(index <= size_);
        return items_[index];
    }

    // Возвращает константную ссылку на элемент с индексом index
    const Type& operator[](size_t index) const noexcept
    {
        assert(index <= size_);
        return items_[index];
    }

    // Возвращает константную ссылку на элемент с индексом index
    // Выбрасывает исключение std::out_of_range, если index >= size
    Type& At(size_t index)
    {
        if (index >= size_)
        {
            throw std::out_of_range("out_of_range");
        }
        return items_[index];

    }

    // Возвращает константную ссылку на элемент с индексом index
    // Выбрасывает исключение std::out_of_range, если index >= size
    const Type& At(size_t index) const
    {
        if (index >= size_)
        {
            throw std::out_of_range("out_of_range");
        }
        return items_[index];
    }

    // Обнуляет размер массива, не изменяя его вместимость
    void Clear() noexcept
    {
        size_ = 0u;
    }


    void Resize(size_t new_size)
    {
        if (new_size <= size_)
        {
            size_ = new_size;
        }
        if (new_size > size_ && new_size <= capacity_)
        {
            for (size_t i = size_; i < new_size; ++i)
            {
                items_[i] = {};
            }
            size_ = new_size;
        }
        if (new_size > capacity_)
        {
            SimpleVector new_vector(std::max(new_size, capacity_ * 2));
            for (size_t i = 0; i < size_; ++i)
            {
                new_vector.items_[i] = std::move(items_[i]);
            }
            new_vector.size_ = new_size;
            swap(new_vector);
        }
    }

    // Возвращает итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    Iterator begin() noexcept
    {
        return Iterator{ items_.Get() };
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    Iterator end() noexcept
    {
        return Iterator{ items_.Get() + size_ };
    }

    // Возвращает константный итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator begin() const noexcept
    {
        return ConstIterator{ items_.Get() };
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator end() const noexcept
    {
        return ConstIterator{ items_.Get() + size_ };
    }

    // Возвращает константный итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator cbegin() const noexcept
    {
        return ConstIterator{ items_.Get() };
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator cend() const noexcept
    {
        return ConstIterator{ items_.Get() + size_ };
    }

    // Добавляет элемент в конец вектора
    // При нехватке места увеличивает вдвое вместимость вектора
    void PushBack(const Type& item)
    {
        if (size_ < capacity_)
        {
            items_[size_] = item;
            ++size_;
            return;
        }

        size_t new_capacity = (capacity_ == 0) ? 1 : capacity_ * 2;
        ArrayPtr<Type> temp(new_capacity);
        std::copy(begin(), end(), temp.Get());
        items_.swap(temp);
        items_[size_] = item;
        ++size_;
        capacity_ = new_capacity;
    }

    // Вставляет значение value в позицию pos.
    // Возвращает итератор на вставленное значение
    // Если перед вставкой значения вектор был заполнен полностью,
    // вместимость вектора должна увеличиться вдвое, а для вектора вместимостью 0 стать равной 1
    Iterator Insert(ConstIterator pos, const Type& value)
    {
        assert(pos >= begin());
        assert (pos <= end());

        auto dist = std::distance(cbegin(), pos);

        if (size_ < capacity_)
        {
            std::copy_backward(pos, cend(), end() + 1);

            items_[dist] = value;
            ++size_;
            return items_.Get() + dist;
        }

        size_t new_capacity = (capacity_ == 0) ? 1 : capacity_ * 2;

        ArrayPtr<Type>temp(new_capacity);
        std::copy(cbegin(), pos, temp.Get());
        temp[dist] = value;
        std::copy(pos, cend(), temp.Get() + dist + 1);
        items_.swap(temp);
        ++size_;
        capacity_ = new_capacity;
        return items_.Get() + dist;
    }

    // "Удаляет" последний элемент вектора. Вектор не должен быть пустым
    void PopBack() noexcept
    {
        assert(size_ != 0);
        --size_;
    }

    // Удаляет элемент вектора в указанной позиции
    Iterator Erase(ConstIterator pos)
    {
        assert(pos >= begin());
        assert(pos <= end());
        assert(size_ != 0);

        auto dis = std::distance(cbegin(), pos);
        std::move(begin() + dis + 1, end(), begin() + dis);
        --size_;
        return const_cast<Iterator>(pos);
    }

    // Обменивает значение с другим вектором
    void swap(SimpleVector& other) noexcept
    {
        items_.swap(other.items_);
        std::swap(size_, other.size_);
        std::swap(capacity_, other.capacity_);
    }


    //move-семантика

    SimpleVector(SimpleVector&& other):size_(other.size_), capacity_(other.capacity_)
    {
        items_ = std::move(other.items_);
        other.capacity_ = 0u;
        other.size_ = 0u;
    }

    SimpleVector operator=(SimpleVector&& rhs)
    {
        if (*this != rhs)//проверка что не присваиваем себя себе
        {
            SimpleVector temp = std::move(rhs);
            swap(temp);
        }
        return *this;
    }

    // Добавляет элемент в конец вектора
    // При нехватке места увеличивает вдвое вместимость вектора
    // если capacity = 0, делает ее = 1
    void PushBack(Type&& item)
    {
        if (size_ < capacity_)
        {
            items_[size_] = std::move(item);
            ++size_;
            return;
        }

        size_t new_capacity = (capacity_ == 0) ? 1 : capacity_ * 2;
        ArrayPtr<Type> temp(new_capacity);
        std::move(begin(), end(), temp.Get());
        temp[size_] = std::move(item);
        items_.swap(temp);
        ++size_;
        capacity_ = new_capacity;
    }

    // Обменивает значение с другим вектором
    void swap(SimpleVector&& other) noexcept
    {
        //items_.swap(other.items_);
        std::swap(items_, other.items_);

        size_t temp_size = std::move(other.size_);
        size_t temp_capasity = std::move(other.capacity_);

        other.size_ = std::move(size_);
        other.capacity_ = std::move(capacity_);

        size_ = std::move(temp_size);
        capacity_ = std::move(temp_capasity);
    }

    Iterator Insert(Iterator pos, Type&& value)
    {
        assert(pos >= begin());
        assert (pos <= end());

        auto dist = std::distance(begin(), pos);

        if (size_ < capacity_)
        {
            std::move_backward(pos, end(), end() + 1);
            items_[dist] = std::move(value);
            ++size_;
            return items_.Get() + dist;
        }

        size_t new_capacity = (capacity_ == 0) ? 1 : capacity_ * 2;

        ArrayPtr<Type>temp(new_capacity);
        std::move(begin(), begin() + dist, temp.Get());
        temp[dist] = std::move(value);
        std::move(begin() + dist, end(), temp.Get() + dist + 1);
        items_.swap(temp);
        ++size_;
        capacity_ = new_capacity;
        return items_.Get() + dist;
    }

private:
    ArrayPtr<Type> items_;
    size_t size_ = 0;
    size_t capacity_ = 0;
};

template <typename Type>
inline bool operator==(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs)
{
    if (lhs.GetSize() != rhs.GetSize())
        return false;

    return std::equal(lhs.cbegin(), lhs.cend(), rhs.cbegin());
}

template <typename Type>
inline bool operator!=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs)
{
    return !(lhs == rhs);
}

template <typename Type>
inline bool operator<(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs)
{
    return std::lexicographical_compare(lhs.cbegin(), lhs.cend(), rhs.cbegin(), rhs.cend());
}

template <typename Type>
inline bool operator>(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs)
{
    return rhs < lhs;
}

template <typename Type>
inline bool operator<=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs)
{
    return !(lhs > rhs);
}

template <typename Type>
inline bool operator>=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs)
{
    return !(lhs < rhs);
}
