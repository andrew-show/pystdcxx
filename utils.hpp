#ifndef PYSTDCXX_UTILS_HPP
#define PYSTDCXX_UTILS_HPP

#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <pyerrors.h>
#include <memory>
#include <stdexcept>
#include <string>
#include <cassert>

template <typename T>
class py_ptr
{
public:
    py_ptr(): p_(nullptr)
    {
    }

    explicit py_ptr(T *p, bool incref=false): p_(p)
    {
        if (incref)
            Py_INCREF(p_);
    }

    py_ptr(const py_ptr &rhs): p_(rhs.p_)
    {
        if (p_)
            Py_INCREF(p_);
    }

    py_ptr(py_ptr &&rhs): p_(rhs.p_)
    {
        rhs.p_ = nullptr;
    }

    ~py_ptr()
    {
        if (p_)
            Py_DECREF(p_);
    }

    py_ptr &operator=(const py_ptr &rhs)
    {
        reset(rhs.p_);
        return *this;
    }

    T *get() const
    {
        return p_;
    }

    T *operator->() const
    {
        return p_;
    }

    T **operator&()
    {
        if (p_)
            throw std::logic_error("Get address of none NULL pointer");
        return &p_;
    }

    void reset(T *p=nullptr)
    {
        if (p)
            Py_INCREF(p);

        if (p_)
            Py_DECREF(p_);

        p_ = p;
    }

    T *release()
    {
        T *p = p_;
        p_ = nullptr;
        return p;
    }

private:
    T *p_;
};

static inline bool py_less(PyObject *less, PyObject *lhs, PyObject *rhs)
{
    if (less) {
        py_ptr<PyObject> result(PyObject_CallFunctionObjArgs(less, lhs, rhs, NULL));
        if (!result.get())
            throw std::runtime_error("Compare two object error");

        return PyObject_IsTrue(result.get());
    } else {
        int result = PyObject_RichCompareBool(lhs, rhs, Py_LT);
        if (result == -1)
            throw std::runtime_error("Compare two object error");

        return result != 0;
    }
}

template <typename F>
void py_tuple_for_each(PyObject *tuple, F callback)
{
    int n = PyTuple_GET_SIZE(tuple);
    for (int i = 0; i < n; ++i) {
        PyObject *item = PyTuple_GET_ITEM(tuple, i);
        if (item)
            callback(item);
    }
}

template <typename F>
void py_list_for_each(PyObject *list, F callback)
{
    int n = PyList_GET_SIZE(list);
    for (int i = 0; i < n; ++i) {
        PyObject *item = PyList_GET_ITEM(list, i);
        if (item)
            callback(item);
    }
}

static inline std::string py_repr(PyObject *ob)
{
    py_ptr<PyObject> unicode(PyObject_Repr(ob));
    if (!unicode.get())
        throw std::runtime_error("Get object representation error");

    const char *str = PyUnicode_AsUTF8(unicode.get());
    if (!str)
        throw std::runtime_error("Get UTF8 string of object representation error");

    return std::string(str);
}

template<typename T>
class py_type
{
public:
    static PyTypeObject type;

    static PyTypeObject *get()
    {
        static PyTypeObject type = {
            .ob_base = PyVarObject_HEAD_INIT(NULL, 0)
            .tp_name = tp_name(),
            .tp_basicsize = sizeof(T),
            .tp_itemsize = 0,
            .tp_dealloc = (destructor)tp_dealloc(),
            .tp_getattr = (getattrfunc)tp_getattr(),
            .tp_setattr = (setattrfunc)tp_setattr(),
            .tp_repr = (reprfunc)tp_repr(),
            .tp_as_sequence = tp_as_sequence(),
            .tp_hash = (hashfunc)tp_hash(),
            .tp_call = (ternaryfunc)tp_call(),
            .tp_str = (reprfunc)tp_str(),
            .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,
            .tp_doc = tp_doc(),
            .tp_traverse = (traverseproc)tp_traverse(),
            .tp_clear = (inquiry)tp_clear(),
            .tp_richcompare = (richcmpfunc)tp_richcompare(),
            .tp_iter = (getiterfunc)tp_iter(),
            .tp_iternext = (iternextfunc)tp_iternext(),
            .tp_methods = tp_methods(),
            .tp_descr_get = (descrgetfunc)tp_descr_get(),
            .tp_descr_set = (descrsetfunc)tp_descr_set(),
            .tp_init = (initproc)tp_init(),
            .tp_new = (newfunc)tp_new(),
        };

        return &type;
    }

    static PyMethodDef *tp_methods() { return get_tp_methods<T>(nullptr); }
    static PySequenceMethods *tp_as_sequence() { return get_tp_as_sequence<T>(nullptr); }
    static constexpr const char *tp_name() { return get_tp_name<T>(nullptr); }
    static constexpr const char *tp_doc() { return get_tp_doc<T>(nullptr); }
    static constexpr void (*tp_dealloc())(T *self) { return get_tp_dealloc<T>(nullptr); }
    static constexpr PyObject* (*tp_getattr())(T *self, char *name) { return get_tp_getattr<T>(nullptr); }
    static constexpr int (*tp_setattr())(T *self, char *attr, PyObject *value) { return get_tp_setattr<T>(nullptr); }
    static constexpr PyObject* (*tp_repr())(T *self) { return get_tp_repr<T>(nullptr); }
    static constexpr Py_hash_t (*tp_hash())(T *self) { return get_tp_hash<T>(nullptr); }
    static constexpr PyObject* (*tp_call())(T *self, PyObject *args, PyObject *kwargs) { return get_tp_call<T>(nullptr); }
    static constexpr PyObject* (*tp_str())(T *self) { return get_tp_str<T>(nullptr); }
    static constexpr int tp_flags() { return get_tp_flags<T>(nullptr); }
    static constexpr int (*tp_traverse())(T *self, visitproc visit, void *arg) { return get_tp_traverse<T>(nullptr); }
    static constexpr int (*tp_clear())(T *self) { return get_tp_clear<T>(nullptr); }
    static constexpr PyObject* (*tp_richcompare())(T *self, PyObject *other, int op) { return get_tp_richcompare<T>(nullptr); }
    static constexpr PyObject* (*tp_iter())(T *self) { return get_tp_iter<T>(nullptr); }
    static constexpr PyObject* (*tp_iternext())(T *self) { return get_tp_iternext<T>(nullptr); }
    static constexpr PyObject* (*tp_descr_get())(T *self, PyObject *obj, PyObject *type) { return get_tp_descr_get<T>(nullptr); }
    static constexpr int (*tp_descr_set())(T *self, PyObject *obj, PyObject *type) { return get_tp_descr_set<T>(nullptr); }
    static constexpr int (*tp_init())(T *self, PyObject *args, PyObject *kwds) { return get_tp_init<T>(nullptr); }
    static constexpr PyObject* (*tp_new())(PyTypeObject *type, PyObject *args, PyObject *kwds) { return get_tp_new<T>(nullptr); }
    static constexpr Py_ssize_t (*sq_length())(T *self) { return get_sq_length<T>(nullptr); }
    static constexpr PyObject* (*sq_concat())(T *self, PyObject *args) { return get_sq_concat<T>(nullptr); }
    static constexpr PyObject* (*sq_repeat())(T *self, Py_ssize_t count) { return get_sq_repeat<T>(nullptr); }
    static constexpr PyObject* (*sq_item())(T *self, Py_ssize_t i) { return get_sq_item<T>(nullptr); }
    static constexpr int (*sq_ass_item())(T *self, Py_ssize_t i, PyObject *value) { return get_sq_ass_item<T>(nullptr); }
    static constexpr int (*sq_contains())(T *self, PyObject *value) { return get_sq_contains<T>(nullptr); }
    static constexpr PyObject* (*sq_inplace_concat())(T *self, PyObject *args) { return get_sq_inplace_concat<T>(nullptr); }
    static constexpr PyObject* (*sq_inplace_repeat())(T *self, PyObject *args) { return get_sq_inplace_repeat<T>(nullptr); }

private:
    template<typename O>
    static PyMethodDef *get_tp_methods(...) { return nullptr; }

    template<typename O>
    static PyMethodDef *get_tp_methods(decltype(&O::tp_methods)) { return O::tp_methods(); }

    template<typename O>
    static PySequenceMethods *get_tp_as_sequence(...) { return nullptr; }

    template<typename O>
    static PySequenceMethods *get_tp_as_sequence(decltype(&O::sq_length))
    {
        static PySequenceMethods methods = {
            .sq_length = (lenfunc)sq_length(),
            .sq_concat = (binaryfunc)sq_concat(),
            .sq_repeat = (ssizeargfunc)sq_repeat(),
            .sq_item = (ssizeargfunc)sq_item(),
            .sq_ass_item = (ssizeobjargproc)sq_ass_item(),
            .sq_contains = (objobjproc)sq_contains(),
            .sq_inplace_concat = (binaryfunc)sq_inplace_concat(),
            .sq_inplace_repeat = (ssizeargfunc)sq_inplace_repeat(),
        };

        return &methods;
    }

    template<typename O>
    static constexpr const char *get_tp_name(...) { return ""; }

    template<typename O>
    static constexpr const char *get_tp_name(decltype(&O::tp_name)) { return O::tp_name(); }

    template<typename O>
    static constexpr const char *get_tp_doc(...) { return ""; }

    template<typename O>
    static constexpr const char *get_tp_doc(decltype(&O::tp_name)) { return O::tp_doc(); }

    template<typename O>
    static constexpr void (*get_tp_dealloc(...))(O *self) { return nullptr; }

    template<typename O>
    static constexpr void (*get_tp_dealloc(decltype(&O::tp_dealloc)))(O *self) { return &O::tp_dealloc; }

    template<typename O>
    static constexpr PyObject* (*get_tp_getattr(...))(O *self, char *name) { return nullptr; }

    template<typename O>
    static constexpr PyObject* (*get_tp_getattr(decltype(&O::tp_getattr)))(O *self, char *name) { return &O::tp_getattr; }

    template<typename O>
    static constexpr int (*get_tp_setattr(...))(O *self, char *attr, PyObject *value) { return nullptr; }

    template<typename O>
    static constexpr int (*get_tp_setattr(decltype(&O::tp_setattr)))(O *self, char *attr, PyObject *value) { return &O::tp_setattr; }

    template<typename O>
    static constexpr PyObject* (*get_tp_repr(...))(O *self) { return nullptr; }

    template<typename O>
    static constexpr PyObject* (*get_tp_repr(decltype(&O::tp_repr)))(O *self) { return &O::tp_repr; }

    template<typename O>
    static constexpr Py_hash_t (*get_tp_hash(...))(O *self) { return nullptr; }

    template<typename O>
    static constexpr Py_hash_t (*get_tp_hash(decltype(&O::tp_hash)))(O *self) { return &O::tp_hash; }

    template<typename O>
    static constexpr PyObject* (*get_tp_call(...))(O *self, PyObject *args, PyObject *kwargs) { return nullptr; }

    template<typename O>
    static constexpr PyObject* (*get_tp_call(decltype(&O::tp_call)))(O *self, PyObject *args, PyObject *kwargs) { return &O::tp_call; }

    template<typename O>
    static constexpr PyObject* (*get_tp_str(...))(O *self) { return nullptr; }

    template<typename O>
    static constexpr PyObject* (*get_tp_str(decltype(&O::tp_str)))(O *self) { return &O::tp_str; }

    template<typename O>
    static constexpr int get_tp_flags(...) { return Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE; }

    template<typename O>
    static constexpr int get_tp_flags(decltype(&O::tp_traverse)) { return Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC; }

    template<typename O>
    static constexpr int (*get_tp_traverse(...))(O *self, visitproc visit, void *arg) { return nullptr; }

    template<typename O>
    static constexpr int (*get_tp_traverse(decltype(&O::tp_traverse)))(O *self, visitproc visit, void *arg) { return &O::tp_traverse; }

    template<typename O>
    static constexpr int (*get_tp_clear(...))(O *self) { return nullptr; }

    template<typename O>
    static constexpr int (*get_tp_clear(decltype(&O::tp_clear)))(O *self) { return &O::tp_clear; }

    template<typename O>
    static constexpr PyObject* (*get_tp_richcompare(...))(O *self, PyObject *other, int op) { return nullptr; }

    template<typename O>
    static constexpr PyObject* (*get_tp_richcompare(decltype(&O::tp_richcompare)))(O *self, PyObject *other, int op) { return &O::tp_richcompare; }

    template<typename O>
    static constexpr PyObject* (*get_tp_iter(...))(O *self) { return nullptr; }

    template<typename O>
    static constexpr PyObject* (*get_tp_iter(decltype(&O::tp_iter)))(O *self) { return &O::tp_iter; }

    template<typename O>
    static constexpr PyObject* (*get_tp_iternext(...))(O *self) { return nullptr; }

    template<typename O>
    static constexpr PyObject* (*get_tp_iternext(decltype(&O::tp_iternext)))(O *self) { return &O::tp_iternext; }

    template<typename O>
    static constexpr PyObject* (*get_tp_descr_get(...))(O *self, PyObject *obj, PyObject *type) { return nullptr; }

    template<typename O>
    static constexpr PyObject* (*get_tp_descr_get(decltype(&O::tp_descr_get)))(O *self, PyObject *obj, PyObject *type) { return &O::tp_descr_get; }

    template<typename O>
    static constexpr int (*get_tp_descr_set(...))(O *self, PyObject *obj, PyObject *type) { return nullptr; }

    template<typename O>
    static constexpr int (*get_tp_descr_set(decltype(&O::tp_descr_set)))(O *self, PyObject *obj, PyObject *type) { return &O::tp_descr_set; }

    template<typename O>
    static constexpr int (*get_tp_init(...))(O *self, PyObject *args, PyObject *kwds) { return nullptr; }

    template<typename O>
    static constexpr int (*get_tp_init(decltype(&O::tp_init)))(O *self, PyObject *args, PyObject *kwds) { return &O::tp_init; }

    template<typename O>
    static constexpr PyObject* (*get_tp_new(...))(PyTypeObject *type, PyObject *args, PyObject *kwds) { return nullptr; }

    template<typename O>
    static constexpr PyObject* (*get_tp_new(decltype(&O::tp_new)))(PyTypeObject *type, PyObject *args, PyObject *kwds) { return &O::tp_new; }

    template<typename O>
    static constexpr Py_ssize_t (*get_sq_length(...))(O *self) { return nullptr; }

    template<typename O>
    static constexpr Py_ssize_t (*get_sq_length(decltype(&O::sq_length)))(O *self) { return &O::sq_length; }

    template<typename O>
    static constexpr PyObject* (*get_sq_concat(...))(O *self, PyObject *args) { return nullptr; }

    template<typename O>
    static constexpr PyObject* (*get_sq_concat(decltype(&O::sq_concat)))(O *self, PyObject *args) { return &O::sq_concat; }

    template<typename O>
    static constexpr PyObject* (*get_sq_repeat(...))(O *self, Py_ssize_t count) { return nullptr; }

    template<typename O>
    static constexpr PyObject* (*get_sq_repeat(decltype(&O::sq_repeat)))(O *self, Py_ssize_t count) { return &O::sq_repeat; }

    template<typename O>
    static constexpr PyObject* (*get_sq_item(...))(O *self, Py_ssize_t i) { return nullptr; }

    template<typename O>
    static constexpr PyObject* (*get_sq_item(decltype(&O::sq_item)))(O *self, Py_ssize_t i) { return &O::sq_item; }

    template<typename O>
    static constexpr int (*get_sq_ass_item(...))(O *self, Py_ssize_t i, PyObject *value) { return nullptr; }

    template<typename O>
    static constexpr int (*get_sq_ass_item(decltype(&O::sq_ass_item)))(O *self, Py_ssize_t i, PyObject *value) { return &O::sq_ass_item; }

    template<typename O>
    static constexpr int (*get_sq_contains(...))(O *self, PyObject *value) { return nullptr; }

    template<typename O>
    static constexpr int (*get_sq_contains(decltype(&O::sq_contains)))(O *self, PyObject *value) { return &O::sq_contains; }

    template<typename O>
    static constexpr PyObject* (*get_sq_inplace_concat(...))(O *self, PyObject *args) { return nullptr; }

    template<typename O>
    static constexpr PyObject* (*get_sq_inplace_concat(decltype(&O::sq_inplace_concat)))(O *self, PyObject *args) { return &O::sq_inplace_concat; }

    template<typename O>
    static constexpr PyObject* (*get_sq_inplace_repeat(...))(O *self, PyObject *args) { return nullptr; }

    template<typename O>
    static constexpr PyObject* (*get_sq_inplace_repeat(decltype(&O::sq_inplace_repeat)))(O *self, PyObject *args) { return &O::sq_inplace_repeat; }
};

template<typename T>
class py_base
{
public:
    PyObject_HEAD

    typedef py_type<T> this_type;

    void *operator new(std::size_t size)
    {
        void *self;
        if (this_type::tp_flags() & Py_TPFLAGS_HAVE_GC)
            self = PyObject_New(T, this_type::get());
        else
            self = PyObject_GC_New(T, this_type::get());
            
        if (!self)
            throw std::bad_alloc();
        return self;
    }

    void operator delete(void *self)
    {
        if (this_type::tp_flags() & Py_TPFLAGS_HAVE_GC)
            PyObject_Del(self);
        else
            PyObject_GC_Del(self);
    }

    static void tp_dealloc(T *self)
    {
        printf("dealloc %p\n", self);
        delete self;
    }
};

#endif // PYSTDCXX_UTILS_HPP
