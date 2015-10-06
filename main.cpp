#include <iostream>
#include <vector>
#include <unordered_map>
#include <stdlib.h>

using namespace std;

namespace GarbageCollection {

    typedef uint64_t ref_type;
    typedef uint64_t size_type;

    // class list
    template <typename T> class GCRef;
    class GCObject;
    class GC;
    class GCUtils;

    class GCUtils {
    public:
        static GC* default_gc;
    };

    template <typename T>
    class GCRef {
        ref_type ref;
        size_type size;
        unsigned char id;
        GC* gc;

        GCRef(GC* gc, ref_type ref) :
                ref(ref),
                size(sizeof(T)),
                id(0),
                gc(gc)
        {}
    public:

        T& operator*();
        T* operator->();

        GCRef() :
                ref(0),
                size(0),
                id(0),
                gc(nullptr)
        {}

        GCRef(void* ptr) :
                ref((ref_type) ptr),
                size(sizeof(T)),
                id(0),
                gc(GCUtils::default_gc)
        {}

        operator bool() {
            return gc != nullptr;
        }

        friend class GC;
    };

    class GCObject {
    public:
        virtual vector<GCRef<GCObject>*> refs() = 0;
        virtual ~GCObject() {}
    };

    class GC {
        char* heap;
        char* oldgen;
        size_type heap_size;
        size_type oldgen_size;
        size_type pos;
    public:
        GC(size_type heap_size, size_type oldgen_size) :
                heap(new char[heap_size]),
                oldgen(new char[oldgen_size]),
                heap_size(heap_size),
                oldgen_size(oldgen_size),
                pos(0)
        {}

        ~GC() {
            delete[] heap;
            delete[] oldgen;
        }

        template <typename T, typename ... Ts>
        GCRef<T> gcnew(Ts ... ts) {
            size_type allocated_pos = pos;
            new (heap + pos) T(ts...);
            pos += sizeof(T);
            return GCRef<T>(this, allocated_pos);
        }

        template <typename T> friend class GCRef;
    };

    template <typename T>
    T& GCRef<T>::operator*() {
        switch (id) {
            case 0: return (T&) gc->heap[ref];
            default: return (T&) gc->oldgen[ref];
        }
    }

    template <typename T>
    T* GCRef<T>::operator->() {
        switch (id) {
            case 0: return (T*) &gc->heap[ref];
            default: return (T*) &gc->oldgen[ref];
        }
    }

}

using namespace GarbageCollection;

class Node : GCObject {
public:
    GCRef<Node> left;
    GCRef<Node> right;
    int c;

    Node(GCRef<Node> left, GCRef<Node> right, int c) :
            left(left),
            right(right),
            c(c)
    {}

    Node(int c) :
            left(),
            right(),
            c(c)
    {}

    ~Node() {

    }

    /*template <typename ... Ts>
    void* operator new(Ts ... ts) {
        vector<int>* kek = (vector<int>*) malloc(sizeof(vector<int>));
        new (kek) vector<int>(ts...);
        return kek;
    }*/

    void* operator new(size_t size) {
        return (void*) 42;
    }

    void* operator new(size_t size, char* addr) {
        return (void*) 0;
    }

    vector<GCRef<GCObject>*> refs() override {
        return vector<GCRef<GCObject>*>(
                {(GCRef<GCObject>*) &left,
                 (GCRef<GCObject>*) &right}
        );
    }
};

void print_tree(GCRef<Node> root) {
    static string level = "";
    if (!root) {
        return;
    }
    cout << level << "|---" << root->c << endl;
    level += "|   ";
    print_tree(root->left);
    print_tree(root->right);
    level.pop_back();
    level.pop_back();
    level.pop_back();
    level.pop_back();
}


GC gc(1024, 1024);
GC* GCUtils::default_gc = &gc;

int main() {

    Node* kek = new Node();
    cout << (uint64_t) kek << endl;

    GCRef<Node> a = gc.gcnew<Node>(42);
    GCRef<Node> b = gc.gcnew<Node>(69);
    GCRef<Node> c = gc.gcnew<Node>(142);
    GCRef<Node> d = gc.gcnew<Node>(169);
    GCRef<Node> ab = gc.gcnew<Node>(a, b, 0);
    GCRef<Node> cd = gc.gcnew<Node>(c, d, 0);
    GCRef<Node> abcd = gc.gcnew<Node>(ab, cd, 0);
    print_tree(abcd);
    return 0;
}