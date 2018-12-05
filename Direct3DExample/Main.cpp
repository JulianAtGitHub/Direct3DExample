
#include "pch.h"
#include "Application.h"

static void TestCode(void) {

    CBSTree<int, CString> *mymap = new CBSTree<int, CString>();
    mymap->Insert(43, "test");
    mymap->Insert(3, "test");
    mymap->Insert(23, "test");
    mymap->Insert(32, "test");
    mymap->Insert(-34, "test");
    mymap->Insert(34, "test");
    mymap->Insert(67, "test");
    mymap->Insert(34, "test");
    mymap->Insert(64, "test");
    mymap->Insert(56, "test");
    mymap->Insert(0, "test");
    mymap->Insert(93, "test");
    mymap->Insert(10, "test");
    mymap->Insert(50, "test");
    mymap->Insert(59, "test");
    mymap->Insert(15, "test");

    typedef CBSTree<int, CString>::Iterator Iter;
    for (Iter &iter = mymap->Traverse(); iter.IsValid(); iter.Next()) {
        LogOutput("%d\n", iter.GetKey());
    }

    CBSTree<int, CString> mymap2;
    mymap2 = *mymap;
    for (Iter &iter = mymap2.Traverse(); iter.IsValid(); iter.Next()) {
        LogOutput("%d\n", iter.GetKey());
    }

    CReference<CString> d1 = mymap->Find(93);
    CReference<CString> d2 = mymap->Find(11);

    mymap->Remove(23);
    mymap->Remove(32);
    mymap->Remove(93);
    mymap->Remove(64);

    delete mymap;
}

_Use_decl_annotations_
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    TestCode();

    Application app(1280, 720, "Direct3D Example");
    app.Run(hInstance, nCmdShow);
}
