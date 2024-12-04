#ifndef PYSTDCXX_UTILS_HPP
#define PYSTDCXX_UTILS_HPP

#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <pyerrors.h>
#include <memory>
#include <stdexcept>
#include <string>
#include <cassert>

namespace {

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

template<typename T>
struct py_less
{
    explicit py_less(T &self): self(self)
    {}

    bool operator()(const py_ptr<PyObject> &lhs, const py_ptr<PyObject> &rhs) const
    {
        if (self.less.get()) {
            py_ptr<PyObject> result(PyObject_CallFunctionObjArgs(self.less.get(), lhs.get(), rhs.get(), NULL));
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

    T &self;
};

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

} // namespace

#endif // PYSTDCXX_UTILS_HPP
