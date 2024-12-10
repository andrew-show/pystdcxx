#ifndef PYSTDCXX_MAP_HPP
#define PYSTDCXX_MAP_HPP

#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <pyerrors.h>
#include <map>
#include "utils.hpp"

class pystdcxx_map: public py_object<pystdcxx_map>
{
private:

public:
    pystdcxx_map(): version(0), map(py_less(less))
    {
        PyObject_GC_Track(this);
    }

    ~pystdcxx_map()
    {
        PyObject_GC_UnTrack(this);
    }

    static const char *tp_name() { return "pystdcxx.map"; }
    static const char *tp_doc() { return "Python wrapper for std::map"; }
    static PyMethodDef *tp_methods();
    static PyObject *tp_new(PyTypeObject *type, PyObject *args, PyObject *kwds);
    static int tp_init(pystdcxx_map *self, PyObject *args, PyObject *kwds);
    static int tp_traverse(pystdcxx_map *self, visitproc visit, void *arg);
    static int tp_clear(pystdcxx_map *self);
    static PyObject *tp_repr(pystdcxx_map *self);
    static PyObject *tp_iter(pystdcxx_map *self);
    static Py_ssize_t sq_length(pystdcxx_map *self);
    static int sq_contains(pystdcxx_map *self, PyObject *value);
    static PyObject *sq_inplace_concat(pystdcxx_map *self, PyObject *tuple);
    static Py_ssize_t mp_length(pystdcxx_map *self);
    static PyObject *mp_subscript(pystdcxx_map *self, PyObject *key);
    static int mp_ass_subscript(pystdcxx_map *self, PyObject *key, PyObject *value);
    static PyObject *add(pystdcxx_map *self, PyObject *value);
    static PyObject *remove(pystdcxx_map *self, PyObject *value);
    static PyObject *clear(pystdcxx_map *self, PyObject *args);
    static PyObject *reverse(pystdcxx_map *self, PyObject *args);
    static PyObject *find(pystdcxx_map *self, PyObject *value);
    static PyObject *popitem(pystdcxx_map *self, PyObject *args, PyObject *kwds);

private:
    typedef std::map<py_ptr<PyObject>, py_ptr<PyObject>, py_less> stdcxx_map;

    class iterator: public py_object<iterator>
    {
    public:
        iterator(pystdcxx_map *owner, stdcxx_map::iterator first, stdcxx_map::iterator last):
            owner(owner, true),
            version(owner->version),
            first(first),
            last(last)
        {
        }

        static const char *tp_name() { return "pystdcxx.map_iterator"; }
        static const char *tp_doc() { return "Python wrapper for std::map::iterator"; }
        static PyObject *tp_iter(iterator *self);
        static PyObject *tp_iternext(iterator *self);

    private:
        py_ptr<pystdcxx_map> owner;
        uint32_t version;
        stdcxx_map::iterator first, last;
    };

    class reverse_iterator: public py_object<reverse_iterator>
    {
    public:
        reverse_iterator(pystdcxx_map *owner, stdcxx_map::reverse_iterator first, stdcxx_map::reverse_iterator last):
            owner(owner, true),
            version(owner->version),
            first(first),
            last(last)
        {
        }

        static const char *tp_name() { return "pystdcxx.map_reverse_iterator"; }
        static const char *tp_doc() { return "Python wrapper for std::map::reverse_iterator"; }
        static PyObject *tp_iter(reverse_iterator *self);
        static PyObject *tp_iternext(reverse_iterator *self);

    private:
        py_ptr<pystdcxx_map> owner;
        uint32_t version;
        stdcxx_map::reverse_iterator first, last;
    };

    unsigned int version;
    stdcxx_map map;
    py_ptr<PyObject> less;    
};

#endif // PYSTDCXX_MAP_HPP
