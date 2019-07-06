#pragma once

template <typename T>
struct CEqual {
    INLINE int operator()(const T &l, const T &r) const {
        return l == r;
    }
}; 

template <typename T>
struct CLess {
    INLINE int operator()(const T &l, const T &r) const {
        return l < r;
    }
};

template <typename T>
INLINE T AlignUpWithMask(T value, size_t mask) {
    return (T)(((size_t)value + mask) & ~mask);
}

template <typename T> 
INLINE T AlignDownWithMask(T value, size_t mask) {
    return (T)((size_t)value & ~mask);
}

template <typename T> 
INLINE T AlignUp(T value, size_t alignment) {
    return AlignUpWithMask(value, alignment - 1);
}

template <typename T> 
INLINE T AlignDown(T value, size_t alignment) {
    return AlignDownWithMask(value, alignment - 1);
}

template <typename T> 
INLINE bool IsAligned( T value, size_t alignment ) {
    return 0 == ((size_t)value & (alignment - 1));
}

template <typename T> 
INLINE bool IsPowerOfTwo(T value) {
    return 0 == (value & (value - 1));
}

template <typename T> 
INLINE T DivideByMultiple( T value, size_t alignment ) {
    return (T)((value + alignment - 1) / alignment);
}



