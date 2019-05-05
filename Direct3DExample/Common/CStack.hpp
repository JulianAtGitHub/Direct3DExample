#pragma once

template <typename T>
class CStack {
public:
    typedef T Element;

    const static size_t ELEMENT_SIZE = sizeof(Element);
    const static uint32_t DEFAULT_CAPACITY = 8;

    CStack(uint32_t capacity = DEFAULT_CAPACITY)
        :mData(nullptr)
        ,mDepth(0)
        ,mCapacity(capacity)
    {
        if (!mCapacity) { mCapacity = 1; }
        mData = new Element[mCapacity];
    }

    ~CStack(void) { delete [] mData; }

    INLINE uint32_t Depth(void) const { return mDepth; }

    INLINE Element & Top(void) { return mData[mDepth - 1]; }
    INLINE const Element & Top(void) const { return mData[mDepth - 1]; }

    INLINE void Clear(void) { mDepth = 0; }

    INLINE void Pop(void) { if (mDepth > 0) { -- mDepth; } }

    void Push(Element &element) {
        if (mDepth == mCapacity) {
            Reserve(mCapacity * 2);
        }
        mData[mDepth] = element;
        ++ mDepth;
    }

    void Reserve(uint32_t capacity) {
        if (mCapacity >= capacity) { return; }

        Element *newData = new Element[capacity];
        if (mDepth) {
            memcpy(newData, mData, mDepth * ELEMENT_SIZE);
        }
        delete [] mData;
        mData = newData;
        mCapacity = capacity;
    }

    CStack<Element>& operator=(const CStack<Element>& rhs) {
        if (mData) {
            delete [] mData;
        }

        mCapacity = rhs.mCapacity;
        mDepth = rhs.mDepth;
        mData = new Element[mCapacity];
        if (mDepth) {
            memcpy(mData, rhs.mData, mDepth * ELEMENT_SIZE);
        }

        return *this;
    }

private:
    Element *mData;
    uint32_t mDepth;
    uint32_t mCapacity;
};