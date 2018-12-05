#pragma once

template <typename T>
class CList {
public:
    typedef T Element;

    const static size_t ELEMENT_SIZE = sizeof(Element);
    const static uint32_t DEFAULT_CAPACITY = 16;

    CList(uint32_t capacity = DEFAULT_CAPACITY)
    :mData(nullptr)
    ,mCount(0)
    ,mCapacity(capacity)
    {
        if (!mCapacity) { mCapacity = 1; }
        mData = new Element[mCapacity];
    }

    ~CList(void) { delete [] mData; }

    inline const Element * Data(void) const { return mData; }
    inline uint32_t Count(void) const { return mCount; }
    inline uint32_t Capacity(void) const { return mCapacity; }

    inline const Element & At(uint32_t index) const { return mData[index]; }
    inline Element & At(uint32_t index) { return mData[index]; }

    inline const Element & Last(void) const { return mData[mCount - 1]; }
    inline Element & Last(void) { return mData[mCount - 1]; }

    inline void Clear(void) { mCount = 0; }

    inline void PopBack(void) { if (mCount > 0) { -- mCount; } }

    void PushBack(Element &element) {
        if (mCount == mCapacity) {
            Reserve(mCapacity * 2);
        }
        mData[mCount] = element;
        ++ mCount;
    }

    void Reserve(uint32_t capacity) {
        if (mCapacity >= capacity) { return; }

        Element *newData = new Element[capacity];
        if (mCount) {
            memcpy(newData, mData, mCount * ELEMENT_SIZE);
        }
        delete [] mData;
        mData = newData;
        mCapacity = capacity;
    }

    CList<Element>& operator=(const CList<Element>& rhs) {
        if (mData) {
            delete [] mData;
        }

        mCapacity = rhs.mCapacity;
        mCount = rhs.mCount;
        mData = new Element[mCapacity];
        if (mCount) {
            memcpy(mData, rhs.mData, mCount * ELEMENT_SIZE);
        }

        return *this;
    }

private:
    Element *mData;
    uint32_t mCount;
    uint32_t mCapacity;
};