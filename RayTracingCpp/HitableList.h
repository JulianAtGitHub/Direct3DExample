#pragma once

#include "Hitable.h"

class HitableList : public Hitable {
public:
    HitableList(Hitable** list, int listSize)
    : mList(list)
    , mListSize(listSize) 
    {

    }
    ~HitableList(void)
    {

    }

    virtual bool Hit(const Ray &ray, float tMin, float tMax, Record &record);

private:
    Hitable   **mList;
    int         mListSize;

};

INLINE bool HitableList::Hit(const Ray& ray, float tMin, float tMax, Record& record) {
    Record temp;
    bool isHit = false;
    float closetHit = tMax;
    for (int i = 0; i < mListSize; ++i) {
        if (mList[i]->Hit(ray, tMin, closetHit, temp)) {
            isHit = true;
            closetHit = temp.t;
            record = temp;
        }
    }
    return isHit;
}

