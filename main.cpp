#include <iostream>
#include <vector>
#include <unordered_map>
#include <limits>

#define class struct

#define operator_new(Class)\
    void* operator new(size_t size) {\
        return GCUtils::default_gc->gcalloc<Class>();\
    }\
    void* operator new(size_t size, char* addr) {\
        return addr;\
    }

using namespace std;

template <typename U, typename V>
void print(unordered_map<U, V> map) {
    cout << "[";
    for (auto& entry: map) {
        cout << "(" << entry.first << ", " << entry.second << "), ";
    }
    cout << "]" << endl;
};

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

    public:
        T& operator*();
        T* operator->();
        GCRef(T* ptr);
        GCRef(const GCRef<T>& other);
        GCRef<T>& operator=(const GCRef<T>& other);
        ~GCRef();

        GCRef() :
                ref(numeric_limits<ref_type>::max()),
                size(0)
        {}

        operator bool() const {
            return size != 0;
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
        size_type heap_size;
        size_type pos;
        unordered_map<ref_type, size_type> ref_count;
        unordered_map<size_type, bool> offset;
    public:
        GC(size_type heap_size) :
                heap(new char[heap_size]),
                heap_size(heap_size),
                pos(0)
        {}

        ~GC() {
            delete[] heap;
        }

        template <typename T>
        void* gcalloc() {
            size_type allocated_pos = pos;
            offset[pos] = false;
            pos += sizeof(T);
            return (void*) (heap + allocated_pos);
        }

        template <typename T, typename ... Ts>
        GCRef<T> gcnew(Ts ... ts) {
            char* addr = (char*) gcalloc<T>();
            new (addr) T(ts...);
            return GCRef<T>((T*) addr);
        }

        void dfs(GCObject* object) {
            offset[(ref_type) object - (ref_type) heap] = true;
            for (auto& ref: object->refs()) {
                if (!*ref) {
                    continue;
                }
                if (!offset[ref->ref]) {
                    dfs((GCObject*) &heap[ref->ref]);
                }
            }
        }

        void gc() {
            for (auto& entry: ref_count) {
                if (entry.second > 0) {
                    cout << entry.first << " " << entry.second << endl;
                    dfs((GCObject*) &heap[entry.first]);
                }
            }
            print(offset);
        }

        template <typename T> friend class GCRef;
    };

    template <typename T>
    GCRef<T>::GCRef(T* ptr) :
            ref((ref_type) ptr - (ref_type) GCUtils::default_gc->heap),
            size(sizeof(T))
    {
        GCUtils::default_gc->ref_count[ref]++;
        cout << "Oo " << ref << endl;
    }

    template <typename T>
    GCRef<T>::GCRef(const GCRef<T>& other) {
        if (other) {
            GCUtils::default_gc->ref_count[other.ref]++;
        }
        ref = other.ref;
        size = other.size;
    }

    template <typename T>
    GCRef<T>& GCRef<T>::operator=(const GCRef<T>& other) {
        if (*this) {
            GCUtils::default_gc->ref_count[ref]--;
        }
        if (other) {
            GCUtils::default_gc->ref_count[other.ref]++;
        }
        ref = other.ref;
        size = other.size;
        return *this;
    }

    template <typename T>
    T& GCRef<T>::operator*() {
        return (T&) GCUtils::default_gc->heap[ref];
    }

    template <typename T>
    T* GCRef<T>::operator->() {
        return (T*) &GCUtils::default_gc->heap[ref];
    }

    template <typename T>
    GCRef<T>::~GCRef() {
        if (*this) {
            GCUtils::default_gc->ref_count[ref]--;
        }
    }

    template <typename T>
    void null(GCRef<T>& ref) {
        ref = GCRef<T>();
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

    operator_new(Node)

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

GC gc(1024);
GC* GCUtils::default_gc = &gc;

int main() {

    GCRef<Node> kek = new Node(562);

    GCRef<Node> a = new Node(42);
    GCRef<Node> b = new Node(69);
    GCRef<Node> c = new Node(142);
    GCRef<Node> d = new Node(169);
    GCRef<Node> ab = new Node(a, b, 0);
    GCRef<Node> cd = new Node(c, d, 0);
    GCRef<Node> abcd = new Node(ab, cd, 0);

    print_tree(abcd);

    null(a);
    null(b);
    null(c);
    null(d);
    null(ab);
    null(cd);
    null(kek);

    GCUtils::default_gc->gc();

    return 0;
}