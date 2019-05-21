#pragma once

template <typename T>
class CReference {
public:
    typedef T Item;

    CReference(void):mItem(nullptr) {}
    CReference(Item &item):mItem(&item) {}

    INLINE bool IsValid(void) const { return mItem != nullptr; }
    INLINE Item & Get(void) { return *mItem; }
    INLINE const Item & Get(void) const { return *Item; }

private:
    Item *mItem;
};

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

