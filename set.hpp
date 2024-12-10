#ifndef PYSTDCXX_SET_HPP
#define PYSTDCXX_SET_HPP

#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <pyerrors.h>
#include <set>
#include "utils.hpp"

class pystdcxx_set: public py_object<pystdcxx_set>
{
private:

public:
    pystdcxx_set(): version(0), set(py_less(less))
    {
        PyObject_GC_Track(this);
    }

    ~pystdcxx_set()
    {
        PyObject_GC_UnTrack(this);
    }

    static const char *tp_name() { return "pystdcxx.set"; }
    static const char *tp_doc() { return "Python wrapper for std::set"; }
    static PyMethodDef *tp_methods();
    static PyObject *tp_new(PyTypeObject *type, PyObject *args, PyObject *kwds);
    static int tp_init(pystdcxx_set *self, PyObject *args, PyObject *kwds);
    static int tp_traverse(pystdcxx_set *self, visitproc visit, void *arg);
    static int tp_clear(pystdcxx_set *self);
    static PyObject *tp_repr(pystdcxx_set *self);
    static PyObject *tp_iter(pystdcxx_set *self);
    static Py_ssize_t sq_length(pystdcxx_set *self);
    static int sq_contains(pystdcxx_set *self, PyObject *value);
    static PyObject *sq_inplace_concat(pystdcxx_set *self, PyObject *tuple);
    static PyObject *add(pystdcxx_set *self, PyObject *value);
    static PyObject *remove(pystdcxx_set *self, PyObject *value);
    static PyObject *clear(pystdcxx_set *self, PyObject *args);
    static PyObject *reverse(pystdcxx_set *self, PyObject *args);
    static PyObject *find(pystdcxx_set *self, PyObject *value);
    static PyObject *popitem(pystdcxx_set *self, PyObject *args, PyObject *kwds);

private:
    typedef std::set<py_ptr<PyObject>, py_less> stdcxx_set;

    class iterator: public py_object<iterator>
    {
    public:
        iterator(pystdcxx_set *owner, stdcxx_set::iterator first, stdcxx_set::iterator last):
            owner(owner, true),
            version(owner->version),
            first(first),
            last(last)
        {
        }

        static const char *tp_name() { return "pystdcxx.set_iterator"; }
        static const char *tp_doc() { return "Python wrapper for std::set::iterator"; }
        static PyObject *tp_iter(iterator *self);
        static PyObject *tp_iternext(iterator *self);

    private:
        py_ptr<pystdcxx_set> owner;
        uint32_t version;
        stdcxx_set::iterator first, last;
    };

    class reverse_iterator: public py_object<reverse_iterator>
    {
    public:
        reverse_iterator(pystdcxx_set *owner, stdcxx_set::reverse_iterator first, stdcxx_set::reverse_iterator last):
            owner(owner, true),
            version(owner->version),
            first(first),
            last(last)
        {
        }

        static const char *tp_name() { return "pystdcxx.set_reverse_iterator"; }
        static const char *tp_doc() { return "Python wrapper for std::set::reverse_iterator"; }
        static PyObject *tp_iter(reverse_iterator *self);
        static PyObject *tp_iternext(reverse_iterator *self);

    private:
        py_ptr<pystdcxx_set> owner;
        uint32_t version;
        stdcxx_set::reverse_iterator first, last;
    };

    unsigned int version;
    stdcxx_set set;
    py_ptr<PyObject> less;    
};

#endif // PYSTDCXX_SET_HPP
