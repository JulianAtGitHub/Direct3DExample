#pragma once

template <typename T>
class CBaseString {
public:
    typedef T Element;

    const static size_t ELEMENT_SIZE = sizeof(Element);

    CBaseString(void) { Copy(""); }
    CBaseString(const Element *str):mStr(nullptr) { Copy(str); }
    CBaseString(const CBaseString<Element> &str):mStr(nullptr) { Copy(str.Get()); }
    virtual ~CBaseString(void) { if (mStr != nullptr) { delete [] mStr; } }

    inline const Element * Get(void) const { return mStr; }

    inline size_t Length(void) { return Length(mStr); }

    bool operator==(const CBaseString<Element> &other) const {
        return Compare(mStr, other.Get()) == 0;
    }

    CBaseString<Element>& operator=(const CBaseString<Element>& rhs) {
        Copy(rhs.Get());
        return *this;
    }

protected:
    void Copy(const Element *str) {
        if (mStr != nullptr) {
            delete [] mStr;
        }

        size_t size = Length(str) + 1;
        mStr = new char[size];
        memcpy(mStr, str, size * ELEMENT_SIZE);
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

    Element *mStr;
};

template <typename T>
class CHashBaseString : public CBaseString<T> {
public:
    typedef T Element;

    CHashBaseString(void) { Copy(""); }
    CHashBaseString(const Element *str):CBaseString<T>(str), mHash(GenHash(str)) { }
    CHashBaseString(const CHashBaseString<Element> &str):CBaseString<T>(str.Get()), mHash(str.mHash) { }

    inline const uint64_t Hash(void) const { return mHash; }

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