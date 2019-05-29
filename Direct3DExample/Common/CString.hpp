#pragma once

template <typename T>
class CBaseString {
public:
    typedef T Element;

    const static size_t ELEMENT_SIZE = sizeof(Element);

    CBaseString(void):mStr(nullptr) { Copy("", 0); }
    CBaseString(const Element *str):mStr(nullptr) { Copy(str, Length(str)); }
    CBaseString(const CBaseString<Element> &str):mStr(nullptr) { Copy(str.Get(), str.Length()); }
    virtual ~CBaseString(void) { if (mStr != nullptr) { delete [] mStr; } }

    INLINE const Element * Get(void) const { return mStr; }

    INLINE size_t Length(void) const { return mLenth; }

    bool operator==(const CBaseString<Element> &other) const {
        return Compare(mStr, other.Get()) == 0;
    }

    CBaseString<Element>& operator=(const CBaseString<Element>& rhs) {
        Copy(rhs.Get(), rhs.Length());
        return *this;
    }

    CBaseString<Element>& operator+=(const Element *str) {
        Append(str, Length(str));
        return *this;
    }

    CBaseString<Element>& operator+=(const CBaseString<Element>& str) {
        Append(str);
        return *this;
    }

protected:
    void Copy(const Element *str, size_t length) {
        if (mStr != nullptr) {
            delete [] mStr;
        }

        mLenth = length;
        mSize = length + 1;
        mStr = new Element[mSize];
        memcpy(mStr, str, mSize * ELEMENT_SIZE);
    }

    void Append(const Element *str, size_t length) {
        if (length == 0) {
            return;
        }

        if (mSize - mLenth - 1 >= length) {
            memcpy(mStr + mLenth, str, (length + 1) * ELEMENT_SIZE);
        } else {
            mSize = MAX(mSize, length) >> 1;
            Element *oldStr = mStr;
            mStr = new Element[mSize];
            memcpy(mStr, oldStr, mLenth * ELEMENT_SIZE);
            memcpy(mStr + mLenth, str, (length + 1) * ELEMENT_SIZE);
            delete[] oldStr;
        }
        mLenth += length;
    }

    void Append(const CBaseString<Element>& str) {
        Append(str.Get(), str.Length());
    }

private:
    static size_t Length(const Element *str) {
        const Element *end = str;
        while ( *end ) { ++ end; }
        return end - str; 
    }

    static int Compare(const Element *lhs, const Element *rhs) {
        const Element *lend = lhs, *rend = rhs;
        while ((*lend) == (*rend) && (*lend) && (*rend)) {
            ++ lend, ++rend;
        }
        return (*lend) - (*rend);
    }

    size_t mLenth;
    size_t mSize;   // buffer size
    Element *mStr;
};

template <typename T>
class CHashBaseString : public CBaseString<T> {
public:
    typedef T Element;

    CHashBaseString(void) { Copy(""); }
    CHashBaseString(const Element *str):CBaseString<T>(str), mHash(GenHash(str)) { }
    CHashBaseString(const CHashBaseString<Element> &str):CBaseString<T>(str.Get()), mHash(str.mHash) { }

    INLINE const uint64_t Hash(void) const { return mHash; }

    bool operator==(const CHashBaseString<Element> &other) const {
        return mHash == other.Hash();
    }

    bool operator<(const CHashBaseString<Element> &other) const {
        return mHash < other.Hash();
    }

    CHashBaseString<Element>& operator=(const CHashBaseString<Element>& rhs) {
        Copy(rhs.Get());
        return *this;
    }

protected:
    void Copy(const Element *str) {
        CBaseString<Element>::Copy(str);
        mHash = GenHash(str);
    }

private:
    // Ref from http://www.cse.yorku.ca/~oz/hash.html
    uint64_t GenHash(const Element *str) {
        const uint8_t *bytes = (const uint8_t *)str;
        uint64_t hash = 5381;
        int32_t c;

        while (c = *bytes++) {
            hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
        }

        return hash;
    }
    
    uint64_t mHash;
};

typedef CBaseString<char> CString;

typedef CHashBaseString<char> CHashString;
