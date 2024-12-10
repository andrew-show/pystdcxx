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

struct py_less
{
    explicit py_less(py_ptr<PyObject> &less): less(less) {}

    bool operator()(const py_ptr<PyObject> &lhs, const py_ptr<PyObject> &rhs) const
    {
        PyObject *f = less.get();
        if (f) {
            py_ptr<PyObject> result(PyObject_CallFunctionObjArgs(f, lhs.get(), rhs.get(), nullptr));
            if (!result.get())
                throw std::runtime_error("Compare two object error");

            return PyObject_IsTrue(result.get());
        } else {
            int result = PyObject_RichCompareBool(lhs.get(), rhs.get(), Py_LT);
            if (result == -1)
                throw std::runtime_error("Compare two object error");

            return result != 0;
        }
    }

    py_ptr<PyObject> &less;
};

static inline bool py_tuple_check(PyObject *tuple)
{
    return PyList_Check(tuple) || PyTuple_Check(tuple);
}

static inline int py_tuple_get_size(PyObject *tuple)
{
    if (PyTuple_Check(tuple))
        return PyTuple_GET_SIZE(tuple);
    else if (PyList_Check(tuple))
        return PyList_GET_SIZE(tuple);
    else
        return -1;
}

static inline PyObject *py_tuple_get_item(PyObject *tuple, int index)
{
    if (PyTuple_Check(tuple))
        return PyTuple_GET_ITEM(tuple, index);
    else if (PyList_Check(tuple))
        return PyList_GET_ITEM(tuple, index);
    else
        return nullptr;
}

template <typename F>
void py_tuple_for_each(PyObject *tuple, F callback)
{
    int n = py_tuple_get_size(tuple);
    for (int i = 0; i < n; ++i) {
        PyObject *item = py_tuple_get_item(tuple, i);
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
    static PyTypeObject *get()
    {
        static PyTypeObject type = {
            .ob_base = PyVarObject_HEAD_INIT(nullptr, 0)
            .tp_name = tp_name(),
            .tp_basicsize = sizeof(T),
            .tp_itemsize = 0,
            .tp_dealloc = (destructor)tp_dealloc(),
            .tp_getattr = (getattrfunc)tp_getattr(),
            .tp_setattr = (setattrfunc)tp_setattr(),
            .tp_repr = (reprfunc)tp_repr(),
            .tp_as_sequence = tp_as_sequence(),
            .tp_as_mapping = tp_as_mapping(),
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

    static void *allocate(PyTypeObject *type) { return allocate_<T>(type, nullptr); }
    static void free(void *p) { free_<T>(p, nullptr); }
    static PyMethodDef *tp_methods() { return tp_methods_<T>(nullptr); }
    static PySequenceMethods *tp_as_sequence() { return tp_as_sequence_<T>(nullptr); }
    static PyMappingMethods *tp_as_mapping() { return tp_as_mapping_<T>(nullptr); }
    static constexpr const char *tp_name() { return tp_name_<T>(nullptr); }
    static constexpr const char *tp_doc() { return tp_doc_<T>(nullptr); }
    static constexpr void (*tp_dealloc())(T *self) { return tp_dealloc_<T>(nullptr); }
    static constexpr PyObject* (*tp_getattr())(T *self, char *name) { return tp_getattr_<T>(nullptr); }
    static constexpr int (*tp_setattr())(T *self, char *attr, PyObject *value) { return tp_setattr_<T>(nullptr); }
    static constexpr PyObject* (*tp_repr())(T *self) { return tp_repr_<T>(nullptr); }
    static constexpr Py_hash_t (*tp_hash())(T *self) { return tp_hash_<T>(nullptr); }
    static constexpr PyObject* (*tp_call())(T *self, PyObject *args, PyObject *kwargs) { return tp_call_<T>(nullptr); }
    static constexpr PyObject* (*tp_str())(T *self) { return tp_str_<T>(nullptr); }
    static constexpr int tp_flags() { return tp_flags_<T>(nullptr); }
    static constexpr int (*tp_traverse())(T *self, visitproc visit, void *arg) { return tp_traverse_<T>(nullptr); }
    static constexpr int (*tp_clear())(T *self) { return tp_clear_<T>(nullptr); }
    static constexpr PyObject* (*tp_richcompare())(T *self, PyObject *other, int op) { return tp_richcompare_<T>(nullptr); }
    static constexpr PyObject* (*tp_iter())(T *self) { return tp_iter_<T>(nullptr); }
    static constexpr PyObject* (*tp_iternext())(T *self) { return tp_iternext_<T>(nullptr); }
    static constexpr PyObject* (*tp_descr_get())(T *self, PyObject *obj, PyObject *type) { return tp_descr_get_<T>(nullptr); }
    static constexpr int (*tp_descr_set())(T *self, PyObject *obj, PyObject *type) { return tp_descr_set_<T>(nullptr); }
    static constexpr int (*tp_init())(T *self, PyObject *args, PyObject *kwds) { return tp_init_<T>(nullptr); }
    static constexpr PyObject* (*tp_new())(PyTypeObject *type, PyObject *args, PyObject *kwds) { return tp_new_<T>(nullptr); }
    static constexpr Py_ssize_t (*sq_length())(T *self) { return sq_length_<T>(nullptr); }
    static constexpr PyObject* (*sq_concat())(T *self, PyObject *args) { return sq_concat_<T>(nullptr); }
    static constexpr PyObject* (*sq_repeat())(T *self, Py_ssize_t count) { return sq_repeat_<T>(nullptr); }
    static constexpr PyObject* (*sq_item())(T *self, Py_ssize_t i) { return sq_item_<T>(nullptr); }
    static constexpr int (*sq_ass_item())(T *self, Py_ssize_t i, PyObject *value) { return sq_ass_item_<T>(nullptr); }
    static constexpr int (*sq_contains())(T *self, PyObject *value) { return sq_contains_<T>(nullptr); }
    static constexpr PyObject* (*sq_inplace_concat())(T *self, PyObject *args) { return sq_inplace_concat_<T>(nullptr); }
    static constexpr PyObject* (*sq_inplace_repeat())(T *self, PyObject *args) { return sq_inplace_repeat_<T>(nullptr); }
    static constexpr Py_ssize_t (*mp_length())(T *self) { return mp_length_<T>(nullptr); }
    static constexpr PyObject* (*mp_subscript())(T *self, PyObject *key) { return mp_subscript_<T>(nullptr); }
    static constexpr int (*mp_ass_subscript())(T *self, PyObject *key, PyObject *value) { return mp_ass_subscript_<T>(nullptr); }

private:
    template<typename O>
    static void *allocate_(PyTypeObject *type, ...) { return PyObject_New(T, type); }

    template<typename O>
    static void *allocate_(PyTypeObject *type, decltype(&O::tp_traverse)) { return PyObject_GC_New(T, type); }

    template<typename O>
    static void free_(void *p, ...) { PyObject_Del(p); }

    template<typename O>
    static void free_(void *p, decltype(&O::tp_traverse)) { PyObject_GC_Del(p); }

    template<typename O>
    static PyMethodDef *tp_methods_(...) { return nullptr; }

    template<typename O>
    static PyMethodDef *tp_methods_(decltype(&O::tp_methods)) { return O::tp_methods(); }

    template<typename O>
    static PySequenceMethods *tp_as_sequence_(...) { return nullptr; }

    template<typename O>
    static PySequenceMethods *tp_as_sequence_(decltype(&O::sq_item))
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
    static PyMappingMethods *tp_as_mapping_(...) { return nullptr; }

    template<typename O>
    static PyMappingMethods *tp_as_mapping_(decltype(&O::mp_subscript))
    {
        static PyMappingMethods methods = {
            .mp_length = (lenfunc)mp_length(),
            .mp_subscript = (binaryfunc)mp_subscript(),
            .mp_ass_subscript = (objobjargproc)mp_ass_subscript(),
        };

        return &methods;
    }

    template<typename O>
    static constexpr const char *tp_name_(...) { return ""; }

    template<typename O>
    static constexpr const char *tp_name_(decltype(&O::tp_name)) { return O::tp_name(); }

    template<typename O>
    static constexpr const char *tp_doc_(...) { return ""; }

    template<typename O>
    static constexpr const char *tp_doc_(decltype(&O::tp_name)) { return O::tp_doc(); }
    
    template<typename O>
    static constexpr void (*tp_dealloc_(...))(O *self) { return nullptr; }

    template<typename O>
    static constexpr void (*tp_dealloc_(decltype(&O::tp_dealloc)))(O *self) { return &O::tp_dealloc; }

    template<typename O>
    static constexpr PyObject* (*tp_getattr_(...))(O *self, char *name) { return nullptr; }

    template<typename O>
    static constexpr PyObject* (*tp_getattr_(decltype(&O::tp_getattr)))(O *self, char *name) { return &O::tp_getattr; }

    template<typename O>
    static constexpr int (*tp_setattr_(...))(O *self, char *attr, PyObject *value) { return nullptr; }

    template<typename O>
    static constexpr int (*tp_setattr_(decltype(&O::tp_setattr)))(O *self, char *attr, PyObject *value) { return &O::tp_setattr; }

    template<typename O>
    static constexpr PyObject* (*tp_repr_(...))(O *self) { return nullptr; }

    template<typename O>
    static constexpr PyObject* (*tp_repr_(decltype(&O::tp_repr)))(O *self) { return &O::tp_repr; }

    template<typename O>
    static constexpr Py_hash_t (*tp_hash_(...))(O *self) { return nullptr; }

    template<typename O>
    static constexpr Py_hash_t (*tp_hash_(decltype(&O::tp_hash)))(O *self) { return &O::tp_hash; }

    template<typename O>
    static constexpr PyObject* (*tp_call_(...))(O *self, PyObject *args, PyObject *kwargs) { return nullptr; }

    template<typename O>
    static constexpr PyObject* (*tp_call_(decltype(&O::tp_call)))(O *self, PyObject *args, PyObject *kwargs) { return &O::tp_call; }

    template<typename O>
    static constexpr PyObject* (*tp_str_(...))(O *self) { return nullptr; }

    template<typename O>
    static constexpr PyObject* (*tp_str_(decltype(&O::tp_str)))(O *self) { return &O::tp_str; }

    template<typename O>
    static constexpr int tp_flags_(...) { return Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE; }

    template<typename O>
    static constexpr int tp_flags_(decltype(&O::tp_traverse)) { return Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC; }

    template<typename O>
    static constexpr int (*tp_traverse_(...))(O *self, visitproc visit, void *arg) { return nullptr; }

    template<typename O>
    static constexpr int (*tp_traverse_(decltype(&O::tp_traverse)))(O *self, visitproc visit, void *arg) { return &O::tp_traverse; }

    template<typename O>
    static constexpr int (*tp_clear_(...))(O *self) { return nullptr; }

    template<typename O>
    static constexpr int (*tp_clear_(decltype(&O::tp_clear)))(O *self) { return &O::tp_clear; }

    template<typename O>
    static constexpr PyObject* (*tp_richcompare_(...))(O *self, PyObject *other, int op) { return nullptr; }

    template<typename O>
    static constexpr PyObject* (*tp_richcompare_(decltype(&O::tp_richcompare)))(O *self, PyObject *other, int op) { return &O::tp_richcompare; }

    template<typename O>
    static constexpr PyObject* (*tp_iter_(...))(O *self) { return nullptr; }

    template<typename O>
    static constexpr PyObject* (*tp_iter_(decltype(&O::tp_iter)))(O *self) { return &O::tp_iter; }

    template<typename O>
    static constexpr PyObject* (*tp_iternext_(...))(O *self) { return nullptr; }

    template<typename O>
    static constexpr PyObject* (*tp_iternext_(decltype(&O::tp_iternext)))(O *self) { return &O::tp_iternext; }

    template<typename O>
    static constexpr PyObject* (*tp_descr_get_(...))(O *self, PyObject *obj, PyObject *type) { return nullptr; }

    template<typename O>
    static constexpr PyObject* (*tp_descr_get_(decltype(&O::tp_descr_get)))(O *self, PyObject *obj, PyObject *type) { return &O::tp_descr_get; }

    template<typename O>
    static constexpr int (*tp_descr_set_(...))(O *self, PyObject *obj, PyObject *type) { return nullptr; }

    template<typename O>
    static constexpr int (*tp_descr_set_(decltype(&O::tp_descr_set)))(O *self, PyObject *obj, PyObject *type) { return &O::tp_descr_set; }

    template<typename O>
    static constexpr int (*tp_init_(...))(O *self, PyObject *args, PyObject *kwds) { return nullptr; }

    template<typename O>
    static constexpr int (*tp_init_(decltype(&O::tp_init)))(O *self, PyObject *args, PyObject *kwds) { return &O::tp_init; }

    template<typename O>
    static constexpr PyObject* (*tp_new_(...))(PyTypeObject *type, PyObject *args, PyObject *kwds) { return nullptr; }

    template<typename O>
    static constexpr PyObject* (*tp_new_(decltype(&O::tp_new)))(PyTypeObject *type, PyObject *args, PyObject *kwds) { return &O::tp_new; }

    template<typename O>
    static constexpr Py_ssize_t (*sq_length_(...))(O *self) { return nullptr; }

    template<typename O>
    static constexpr Py_ssize_t (*sq_length_(decltype(&O::sq_length)))(O *self) { return &O::sq_length; }

    template<typename O>
    static constexpr PyObject* (*sq_concat_(...))(O *self, PyObject *args) { return nullptr; }

    template<typename O>
    static constexpr PyObject* (*sq_concat_(decltype(&O::sq_concat)))(O *self, PyObject *args) { return &O::sq_concat; }

    template<typename O>
    static constexpr PyObject* (*sq_repeat_(...))(O *self, Py_ssize_t count) { return nullptr; }

    template<typename O>
    static constexpr PyObject* (*sq_repeat_(decltype(&O::sq_repeat)))(O *self, Py_ssize_t count) { return &O::sq_repeat; }

    template<typename O>
    static constexpr PyObject* (*sq_item_(...))(O *self, Py_ssize_t i) { return nullptr; }

    template<typename O>
    static constexpr PyObject* (*sq_item_(decltype(&O::sq_item)))(O *self, Py_ssize_t i) { return &O::sq_item; }

    template<typename O>
    static constexpr int (*sq_ass_item_(...))(O *self, Py_ssize_t i, PyObject *value) { return nullptr; }

    template<typename O>
    static constexpr int (*sq_ass_item_(decltype(&O::sq_ass_item)))(O *self, Py_ssize_t i, PyObject *value) { return &O::sq_ass_item; }

    template<typename O>
    static constexpr int (*sq_contains_(...))(O *self, PyObject *value) { return nullptr; }

    template<typename O>
    static constexpr int (*sq_contains_(decltype(&O::sq_contains)))(O *self, PyObject *value) { return &O::sq_contains; }

    template<typename O>
    static constexpr PyObject* (*sq_inplace_concat_(...))(O *self, PyObject *args) { return nullptr; }

    template<typename O>
    static constexpr PyObject* (*sq_inplace_concat_(decltype(&O::sq_inplace_concat)))(O *self, PyObject *args) { return &O::sq_inplace_concat; }

    template<typename O>
    static constexpr PyObject* (*sq_inplace_repeat_(...))(O *self, PyObject *args) { return nullptr; }

    template<typename O>
    static constexpr PyObject* (*sq_inplace_repeat_(decltype(&O::sq_inplace_repeat)))(O *self, PyObject *args) { return &O::sq_inplace_repeat; }

    template<typename O>
    static constexpr Py_ssize_t (*mp_length_(...))(O *self) { return nullptr; }

    template<typename O>
    static constexpr Py_ssize_t (*mp_length_(decltype(&O::mp_length)))(O *self) { return &O::mp_length; }

    template<typename O>
    static constexpr PyObject* (*mp_subscript_(...))(O *self, PyObject *key) { return nullptr; }

    template<typename O>
    static constexpr PyObject* (*mp_subscript_(decltype(&O::mp_subscript)))(O *self, PyObject *key) { return &O::mp_subscript; }

    template<typename O>
    static constexpr int (*mp_ass_subscript_(...))(O *self, PyObject *key, PyObject *value) { return nullptr; }

    template<typename O>
    static constexpr int (*mp_ass_subscript_(decltype(&O::mp_ass_subscript)))(O *self, PyObject *key, PyObject *value) { return &O::mp_ass_subscript; }

};

template<typename T>
class py_object
{
public:
    typedef py_type<T> this_type;

    void *operator new(std::size_t size)
    {
        void *self = this_type::allocate(this_type::get());
        if (!self)
            throw std::bad_alloc();
        return self;
    }

    void *operator new(std::size_t size, PyTypeObject *type)
    {
        void *self = this_type::allocate(type);
        if (!self)
            throw std::bad_alloc();
        return self;
    }

    void operator delete(void *self)
    {
        this_type::free(self);
    }

    static void tp_dealloc(T *self)
    {
        delete self;
    }

protected:
    PyObject_HEAD
};

#endif // PYSTDCXX_UTILS_HPP
