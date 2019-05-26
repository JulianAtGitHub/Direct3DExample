#pragma once

template <typename T>
class CQueue {
public:
    typedef T Element;

    CQueue(void): mHead(nullptr), mTail(nullptr), mCount(0) { }
    ~CQueue(void) { Clear(); }

    INLINE uint32_t Count(void) const { return mCount; }

    INLINE Element & Front(void) { return const_cast<Element&>(mHead->value); }
    INLINE const Element & Front(void) const { return mHead->value; }

    void Push(const Element &element) { 
        Node *node = new Node(element);
        if (!mHead) {
            mHead = node;
            mTail = node;
        } else {
            mTail->next = node;
            node->prev = mTail;
            mTail = node;
        }
        ++ mCount;
    }

    void Pop(void) {
        if (!mHead) { return; }

        Node *node = mHead;
        mHead = mHead->next;
        delete node;
        -- mCount;

        if (!mHead) { mTail = nullptr; }
    }

    void Clear(void) {
        while (mHead) {
            Node *node = mHead;
            mHead = mHead->next;
            delete node;
        }
        mTail = nullptr;
        mCount = 0;
    }

private:
    struct Node {
        Node(const Element &element): value(element), prev(nullptr), next(nullptr) { }

        const Element value;
        Node *prev;
        Node *next;
    };

    Node *mHead;
    Node *mTail;
    uint32_t mCount;
};