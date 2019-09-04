#include "pch.h"

static std::mt19937 rang;
static std::uniform_real_distribution<float> rangDist(0.0f); // dist from [0.0 ~ 1.0)

float RandomUnit(void) {
    return rangDist(rang);
}

XMVECTOR RandomInUnitDisk(void) {
    static XMVECTOR one = {1.0f, 1.0f, 0.0f, 0.0f};
    XMVECTOR p;
    do {
        p = {RandomUnit(), RandomUnit(), 0.0f, 0.0f};
        p = p * 2.0f - one;
    } while(XMVectorGetX(XMVector3Dot(p, p)) >= 1.0f);
    return p;
}

XMVECTOR RandomInUnitSphere(void) {
    XMVECTOR p;
    do {
        p = {RandomUnit(), RandomUnit(), RandomUnit(), 0.0f};
        p = p * 2.0f - g_XMOne3;
    } while(XMVectorGetX(XMVector3Dot(p, p)) >= 1.0f);
    return p;
}
