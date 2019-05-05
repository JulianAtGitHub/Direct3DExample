#pragma once

template <typename K, typename V>
class CTreeNode {
public:
    typedef K Key;
    typedef V Value;

    CTreeNode(const Key &k, 
              const Value &v, 
              CTreeNode *p = nullptr, 
              CTreeNode *l = nullptr, 
              CTreeNode *r = nullptr)
              :key(k)
              ,value(v)
              ,parent(p)
              ,left(l)
              ,right(r) 
              { }

    CTreeNode *parent;
    CTreeNode *left;
    CTreeNode *right;
    Key key;
    Value value;
};

template <typename K, typename V>
class CTreeIterator {
public:
    typedef K Key;
    typedef V Value;
    typedef CTreeNode<Key, Value> Node;

    CTreeIterator(Node *node) { Reset(node); }

    INLINE Key & GetKey(void) { return mNode->key; }
    INLINE const Key & GetKey(void) const { return mNode->key; }

    INLINE Value & GetValue(void) { return mNode->value; }
    INLINE const Value & GetValue(void) const { return mNode->value; }

    INLINE bool IsValid(void) const { return mNode != nullptr; }

    INLINE void Reset(Node *node) {
        mStack.Clear();
        PushLeft(node);
    }

    void Next(void) {
        if (mStack.Depth() == 0) {
            return;
        }

        Node *node = mStack.Top();
        mStack.Pop();
        PushLeft(node->right);
    }

private:
    void PushLeft(Node *node) {
        while (node) {
            mStack.Push(node);
            node = node->left;
        }
        mNode = mStack.Depth() ? mStack.Top() : nullptr;
    }

    CStack<Node *> mStack;
    Node *mNode;
};

template <typename K, typename V, typename L = CLess<K>, typename E = CEqual<K> >
class CBSTree {
public:
    typedef K Key;
    typedef V Value;
    typedef L Less;
    typedef E Equal;
    typedef CTreeNode<Key, Value> Node;
    typedef CTreeIterator<Key, Value> Iterator;
    typedef CReference<Value> ValueRef;

    CBSTree(void): mRoot(nullptr), mCount(0), mIterator(nullptr) {}
    ~CBSTree(void) { Destory(mRoot); }

    INLINE ValueRef Find(const Key &key) {
        Node *node = Search(key);
        return node ? ValueRef(node->value) : ValueRef();
    }

    INLINE void Insert(const Key &key, const Value &value) {
        Attach(key, value);
    }

    INLINE void Remove(const Key &key) {
        Node *node = Detach(key);
        if (node) { delete node; }
    }

    INLINE Iterator & Traverse(void) {
        mIterator.Reset(mRoot);
        return mIterator;
    }

    CBSTree<Key, Value, Less, Equal>& operator=(const CBSTree<Key, Value, Less, Equal>& rhs) {
        Destory(mRoot);
        mRoot = Copy(rhs.mRoot, nullptr);
        mCount = rhs.mCount;
        mIterator.Reset(nullptr);
        mLess = rhs.mLess;
        mEqual = rhs.mEqual;

        return *this;
    }

private:
    Node * Search(const Key &key) {
        Node *root = mRoot;
        while (root && !mEqual(root->key, key)) {
            mLess(root->key, key) ? root = root->right : root = root->left;
        }
        return root;
    }

    void Attach(const Key &key, const Value &value) {
        if (!mRoot) {
            mRoot = new Node(key, value);
            ++ mCount;
            return;
        }

        Node *root = mRoot;
        while (root) {
            // replace value if key is equal
            if (mEqual(root->key, key)) {
                root->value = value;
                return;
            }

            if (mLess(root->key, key)) {
                if (root->right) {
                    root = root->right;
                } else {
                    root->right = new Node(key, value, root);
                    ++ mCount;
                    return;
                }
            } else {
                if (root->left) {
                    root = root->left;
                } else {
                    root->left = new Node(key, value, root);
                    ++ mCount;
                    return;
                }
            }
        }
    }

    Node * Detach(const Key &key) {
        Node *node = Search(key);
        if (!node) { return node; }

        if (node->left && node->right) {
            Node *cand = node->left;
            while(cand->right) { cand = cand->right; }
            if (cand != node->left) {
                if (cand->left) {
                    cand->left->parent = cand->parent;
                }
                cand->parent->right = cand->left;
                cand->left = node->left;
                node->left->parent = cand;
            }
            cand->right = node->right;
            cand->right->parent = cand;
            cand->parent = node->parent;
            // root node do not have parent
            if (node->parent) {
                node == node->parent->left ? node->parent->left = cand : node->parent->right = cand;
            }
        } else if (node->left) {
            node->left->parent = node->parent;
            node == node->parent->left ? node->parent->left = node->left : node->parent->right = node->left;
        } else if (node->right) {
            node->right->parent = node->parent;
            node == node->parent->left ? node->parent->left = node->right : node->parent->right = node->right;
        } else {
            node == node->parent->left ? node->parent->left = nullptr : node->parent->right = nullptr;
        }

        -- mCount;
        return node;
    }

    void Destory(Node *root) {
        if (!root) { return; }
        if (root->left) { Destory(root->left); }
        if (root->right) { Destory(root->right); }
        delete root;
    }

    Node * Copy(Node *node, Node *parent) {
        if (!node) { return nullptr; }
        Node *newNode = new Node(node->key, node->value, parent);
        newNode->left = Copy(node->left, newNode);
        newNode->right = Copy(node->right, newNode);
        return newNode;
    } 

    Node *mRoot;
    uint32_t mCount;
    Iterator mIterator;
    Less mLess;
    Equal mEqual;
};

