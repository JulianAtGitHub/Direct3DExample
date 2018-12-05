#pragma once

template <typename T>
class CReference {
public:
    typedef T Item;

    CReference(void):mItem(nullptr) {}
    CReference(Item &item):mItem(&item) {}

    inline bool IsValid(void) const { return mItem != nullptr; }
    inline Item & Get(void) { return *mItem; }
    inline const Item & Get(void) const { return *Item; }

private:
    Item *mItem;
};

template <typename T>
struct CEqual {
    inline int operator()(const T &l, const T &r) const {
        return l == r;
    }
}; 

template <typename T>
struct CLess {
    inline int operator()(const T &l, const T &r) const {
        return l < r;
    }
};

