#ifndef HEADER_VECTOR_H
#define HEADER_VECTOR_H

#include <cassert>
#include "memory.h"
#include "types.h"

// Dynamic array with optional small-buffer optimization.
// I didn't bother setting stack storage size in bytes, I find it simpler by element if you know what you are doing
template <typename T, size_t SVO_SIZE = 16>
class Vector {
public:

    Vector() : m_heap_storage(nullptr), m_capacity(0), m_size(0) {
    }

    template <size_t S>
    Vector(const Vector<T, S> & p_other) {
        *this = p_other;
    }

    ~Vector() {
        hard_clear();
    }

    inline size_t size() const {
        return m_size;
    }

    inline bool is_empty() const {
        return m_size == 0;
    }

    inline size_t capacity() const {
        return m_capacity;
    }

    bool contains(const T p_value) const {
        T *d = data();
        for(size_t i = 0; i < m_size; ++i) {
            if(d[i] == p_value)
                return true;
        }
        return false;
    }

    bool find(const T p_value, size_t &out_index, size_t p_from = 0) const {
        T *d = data();
        assert(p_from < size());
        for (size_t i = p_from; i < m_size; ++i) {
            if (d[i] == p_value) {
                out_index = i;
                return true;
            }
        }
        return false;
    }

    bool unordered_remove(const T p_value) {
        size_t i;
        if (find(p_value, i)) {
            unordered_remove_at(i);
            return true;
        }
        return false;
    }

    void unordered_remove_at(size_t i) {
        assert(i < size());
        size_t last = size() - 1;
        m_data[i] = m_data[last];
        pop_back();
    }

    void fill(const T p_value) {
        T *d = data();
        for (size_t i = 0; i < m_size; ++i) {
            d[i] = value;
        }
    }

    void fill_range(const T p_value, size_t p_begin, size_t p_size) {
        size_t end = p_begin + p_size;
        assert(end <= m_size);
        T *d = data();
        for (size_t i = p_begin; i < end; ++i) {
            d[i] = p_value;
        }
    }

    void resize_no_init(size_t p_size) {

        if (p_size == 0) {
            // Capacity won't shrink in this case,
            // you need to call shrink explicitely for this to happen
            clear();

        } else {

            if (p_size < m_size) {

                // If has an important destructor
                T *d = data();
                for (size_t i = p_size; i < m_size; ++i) {
                    d[i].~T();
                }

            } else if (p_size > m_capacity) {
                resize_capacity(p_size);
            }

            m_size = p_size;
        }
    }

    void resize(size_t p_size, const T p_fill_value) {

        if (p_size == 0) {
            // Capacity won't shrink in this case,
            // you need to call shrink explicitely for this to happen
            clear();

        } else {

            // TODO Will construction and destruction be optimized out if they do nothing?

            if (p_size < m_size) {

                // If has an important destructor
                T *d = data();
                for (size_t i = p_size; i < m_size; ++i) {
                    d[i].~T();
                }

            } else if (p_size > m_size) {

                if (p_size > m_capacity)
                    resize_capacity(p_size);

                T *d = data();
                for (size_t i = m_size; i < p_size; ++i) {
                    d[i].T(p_fill_value);
                }
            }

            m_size = p_size;
        }
    }

    void clear() {

        // If has an important destructor
        T *d = data();
        for (size_t i = 0; i < m_size; ++i) {
            d[i].~T();
        }

        m_size = 0;
    }

    void hard_clear() {
        clear();
        m_capacity = 0;
        if (is_using_heap()) {
            memfree(m_heap_storage);
            m_heap_storage = nullptr;
        }
    }

    void reserve(size_t p_capacity) {
        if (p_capacity > m_capacity) {
            resize_capacity(p_capacity);
        }
    }

    inline T *data() const {
        return is_using_heap() ? m_heap_storage : (T*)m_stack_storage;
    }

    inline bool is_using_heap() const {
        return m_capacity >= SVO_SIZE;
    }

    void push_back(const T p_value) {
        // Note: we take by value to prevent the case where we resize the capacity,
        // because the reference could be coming from the same storage

        if (m_size == m_capacity) {
            resize_capacity(m_capacity + (m_capacity / 2) + 1);
        }

        T *d = data();

        d[m_size].T(p_value);
        ++m_size;
    }

    void pop_back() {
        assert(m_size != 0);
        --m_size;
        T *d = data();
        d[m_size].~T();
    }

    void shrink() {
        if(m_capacity != m_size) {
            resize_capacity(m_size);
        }
    }

    const T &back() const {
        assert(m_size > 0);
        T *d = data();
        return d[m_size - 1];
    }

    T &back() {
        assert(m_size > 0);
        T *d = data();
        return d[m_size - 1];
    }

    const T & operator[](size_t p_index) const {
        assert(p_index < m_size);
        T *d = data();
        return d[p_index];
    }

    T & operator[](size_t p_index) {
        assert(p_index < m_size);
        T *d = data();
        return d[p_index];
    }

    template <size_t S>
    Vector<T, SVO_SIZE> & operator=(const Vector<T, S> &p_other) {

        clear();

        T *src = p_other.data();
        T *dst = data();

        for(size_t i = 0; i < p_other.size(); ++i) {
            dst[i].T(src[i]);
        }
    }

    // TODO Move semantics?
    void grab(Vector<T, SVO_SIZE> &p_other) {
        hard_clear();

        // TODO Don't bother copying the stack storage when not needed

        // Shallow-copy all members by value
        m_heap_storage = p_other.m_heap_storage;
        memcpy(m_stack_storage, p_other.m_stack_storage, SVO_SIZE * sizeof(T));
        m_capacity = p_other.m_capacity;
        m_size = p_other.m_size;

        // Leave the other vector as empty
        p_other.m_capacity = 0;
        p_other.m_size = 0;
        p_other.m_heap_storage = nullptr;
    }

private:
    void resize_capacity(size_t p_capacity) {
        // memcpy is used because we only move data.
        // In the worst case, pointers to elements themselves will be invalid,
        // but you should expect that if you use a vector with items by value

        if (is_using_heap()) {

            if (p_capacity <= SVO_SIZE) {
                // Move to stack
                // Be careful about the order since storages are in the same union type
                T *d = m_heap_storage;
                memcpy(m_stack_storage, d, p_capacity * sizeof(T));
                memfree(d);

            } else {
                m_heap_storage = static_cast<T*>(memrealloc(m_heap_storage, p_capacity * sizeof(T)));
            }

        } else {

            if (p_capacity > SVO_SIZE) {
                // Move to heap
                T *d = static_cast<T*>(memalloc(p_capacity * sizeof(T)));
                memcpy(d, m_stack_storage, m_capacity * sizeof(T));
                m_heap_storage = d;
            }
            // Else nothing to do with stack storage
        }

        m_capacity = p_capacity;
    }

    union {
        uint8_t m_stack_storage[SVO_SIZE * sizeof(T)];
        T *m_heap_storage;
    };

    size_t m_capacity;
    size_t m_size;

};

#endif // HEADER_VECTOR_H
