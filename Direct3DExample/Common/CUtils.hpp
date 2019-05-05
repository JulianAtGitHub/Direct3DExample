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

