#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <pyerrors.h>
#include <set>
#include "utils.hpp"

struct pystdcxx_set;

typedef std::set<py_ptr<PyObject>, py_less<pystdcxx_set>> stdcxx_set;

struct pystdcxx_set_iterator
{
    PyObject_HEAD
    py_ptr<pystdcxx_set> owner;
    uint32_t version;
    stdcxx_set::iterator first, last;

    static PyTypeObject this_type;

    void *operator new(std::size_t size)
    {
        void *self = PyObject_New(pystdcxx_set_iterator, &this_type);
        if (!self)
            throw std::bad_alloc();
        return self;
    }

    void operator delete(void *self)
    {
        PyObject_Del(self);
    }

    static void tp_dealloc(pystdcxx_set_iterator *self)
    {
        delete self;
    }

    static PyObject *tp_iter(pystdcxx_set_iterator *self, PyObject *Py_UNUSED(args))
    {
        Py_INCREF(self);
        return reinterpret_cast<PyObject *>(self);
    }

    static PyObject *tp_iternext(pystdcxx_set_iterator *self, PyObject *Py_UNUSED(args))
    {
        if (self->version != self->owner->version) {
            PyErr_SetString(PyExc_RuntimeError, "Can't change set while iterating");
            return NULL;
        }

        if (self->first == self->last)
            return NULL;

        PyObject *result = self->first->get();
        ++self->first;

        return result;
    }
};

struct pystdcxx_set
{
    PyObject_HEAD
    unsigned int version;
    stdcxx_set set;
    py_ptr<PyObject> less;

    static PySequenceMethods tp_as_sequence;
    static PyTypeObject this_type;

    void *operator new(std::size_t size)
    {
        void *self = PyObject_GC_New(pystdcxx_set, &this_type);
        if (!self)
            throw std::bad_alloc();
        return self;
    }

    void operator delete(void *self)
    {
        PyObject_GC_Del(self);
    }

    pystdcxx_set(): version(0), set(py_less<pystdcxx_set>(*this))
    {}

    ~pystdcxx_set()
    {
    }

    static PyObject *tp_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
    {
        try {
            pystdcxx_set *self = new pystdcxx_set();
            PyObject_GC_Track(self);
            return reinterpret_cast<PyObject *>(self);
        } catch ( ... ) {
            if (!PyErr_Occurred())
                PyErr_SetString(PyExc_RuntimeError, "Create set object failure");
            return NULL;
        }
    }

    static int tp_init(pystdcxx_set *self, PyObject *args, PyObject *kwds)
    {
        PyObject *tuple = nullptr, *less = nullptr;
        static const char *kwlist[] = { "tuple", "less", NULL };
        if (!PyArg_ParseTupleAndKeywords(args, kwds, "|O$O", const_cast<char **>(kwlist), &tuple, &less))
            return -1;

        if (less) {
            if (Py_IsNone(less)) {
            } else if (PyCallable_Check(less)) {
                self->less = py_ptr<PyObject>(less, true);
            } else {
                PyErr_SetString(PyExc_ValueError, "less argument should be callable type");
                return -1;
            }
        }

        if (tuple) {
            try {
                if (PyList_Check(tuple)) {
                    py_list_for_each(tuple, [self] (PyObject *item) {
                        self->set.insert(py_ptr<PyObject>(item, true));
                    });
                } else if (PyTuple_Check(tuple)) {
                    py_tuple_for_each(tuple, [self] (PyObject *item) {
                        self->set.insert(py_ptr<PyObject>(item, true));
                    });
                } else {
                    PyErr_SetString(PyExc_ValueError, "Require list/tuple type");
                    return -1;
                }
            } catch (std::exception &e) {
                if (!PyErr_Occurred())
                    PyErr_SetString(PyExc_RuntimeError, e.what());
                return -1;
            }
        }

        return 0;
    }

    static void tp_dealloc(pystdcxx_set *self)
    {
        PyObject_GC_UnTrack(self);
        delete self;
    }

    static int tp_traverse(pystdcxx_set *self, visitproc visit, void *arg)
    {
        if (self->less.get())
            Py_VISIT(self->less.get());

        for (stdcxx_set::iterator iter = self->set.begin(); iter != self->set.end(); ++iter)
            Py_VISIT(iter->get());

        return 0;
    }

    static int tp_clear(pystdcxx_set *self)
    {
        stdcxx_set set(std::move(self->set));
        py_ptr<PyObject> less(self->less);
        return 0;
    }

    static PyObject *tp_repr(pystdcxx_set *self)
    {
        try {
            std::string repr("{ ");
            const char *comma = "";

            for (stdcxx_set::iterator iter = self->set.begin(); iter != self->set.end(); ++iter) {
                repr += comma;
                repr += py_repr(iter->get());
                comma = ", ";
            }

            repr += " }";
            return PyUnicode_DecodeUTF8(repr.c_str(), repr.size(), "ignore");
        } catch (std::exception &e) {
            if (!PyErr_Occurred())
                PyErr_SetString(PyExc_RuntimeError, e.what());
            return NULL;
        }
    }

    static PyObject *tp_iter(pystdcxx_set *self, PyObject *Py_UNUSED(args))
    {
        return new pystdcxx_set_iterator(self, self->set.begin(), self->set.end());
    }

    static Py_ssize_t sq_len(pystdcxx_set *self, PyObject *Py_UNUSED(args))
    {
        return self->set.size();
    }

    static int sq_contains(pystdcxx_set *self, PyObject *value)
    {
        return self->set.find(py_ptr<PyObject>(value, true)) != self->set.end();
    }

    static PyObject *sq_inplace_concat(pystdcxx_set *self, PyObject *value)
    {
        size_t size = self->set.size();

        try {
            if (PyList_Check(value)) {
                py_list_for_each(value, [self] (PyObject *item) {
                    self->set.insert(py_ptr<PyObject>(item, true));
                });
            } else if (PyTuple_Check(value)) {
                py_tuple_for_each(value, [self] (PyObject *item) {
                    self->set.insert(py_ptr<PyObject>(item, true));
                });
            } else if (!PyObject_IsInstance(value, (PyObject *)&this_type)){
                self->set.insert(py_ptr<PyObject>(value, true));
            } else {
                PyErr_SetString(PyExc_ValueError, "Require list/tuple type");
                return NULL;
            }
        } catch (std::exception &e) {
            if (size != self->set.size())
                ++self->version;
            if (!PyErr_Occurred())
                PyErr_SetString(PyExc_RuntimeError, e.what());
            return NULL;
        }

        if (size != self->set.size())
            ++self->version;

        Py_INCREF(self);
        return reinterpret_cast<PyObject *>(self);
    }

    static PyObject *add(pystdcxx_set *self, PyObject *value)
    {
        try {
            bool result = self->set.insert(py_ptr<PyObject>(value, true)).second;
            if (result)
                ++self->version;
            return PyBool_FromLong(result);
        } catch (std::exception &e) {
            if (!PyErr_Occurred())
                PyErr_SetString(PyExc_RuntimeError, e.what());
            return NULL;
        } catch ( ... ) {
            if (!PyErr_Occurred())
                PyErr_SetString(PyExc_RuntimeError, "Unknown error");
            return NULL;
        }
    }

    static PyObject *remove(pystdcxx_set *self, PyObject *value)
    {
        try {
            size_t result = self->set.erase(py_ptr<PyObject>(value, true));
            if (result)
                ++self->version;
            return PyBool_FromLong(result);
        } catch (std::exception &e) {
            if (!PyErr_Occurred())
                PyErr_SetString(PyExc_RuntimeError, e.what());
            return NULL;
        } catch ( ... ) {
            if (!PyErr_Occurred())
                PyErr_SetString(PyExc_RuntimeError, "Unknown error");
            return NULL;
        }
    }

    static PyObject *clear(pystdcxx_set *self, PyObject *Py_UNUSED(args))
    {
        self->set.clear();
        Py_RETURN_NONE;
    }

    static PyObject *find(pystdcxx_set *self, PyObject *value)
    {
        return new pystdcxx_set_iterator(self, self->set.find(py_ptr<PyObject>(value, true)), self->set.end());
    }

};


PyTypeObject pystdcxx_set_iterator::this_type = {
    .ob_base = PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "stdcxx.set_iterator",
    .tp_basicsize = sizeof(pystdcxx_set_iterator),
    .tp_itemsize = 0,
    .tp_dealloc = (destructor)pystdcxx_set_iterator::tp_dealloc,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_doc = PyDoc_STR("Python wrapper for std::set"),
    .tp_iter = (getiterfunc)pystdcxx_set_iterator::tp_iter,
    .tp_iternext = (iternextfunc)pystdcxx_set_iterator::tp_iternext,
};



PyMethodDef pystdcxx_set::tp_methods[] = {
    { "add",          (PyCFunction)pystdcxx_set::add,      METH_O,       "Add item" },
    { "remove",       (PyCFunction)pystdcxx_set::remove,   METH_O,       "Remove item" },
    { "clear",        (PyCFunction)pystdcxx_set::clear,    METH_NOARGS,  "Clear all items" },
    { "find",         (PyCFunction)pystdcxx_set::find,     METH_O,       "Find a item and return an iterator" },
    { NULL },
};

PySequenceMethods pystdcxx_set::tp_as_sequence = {
    .sq_length = (lenfunc)pystdcxx_set::sq_len,
    .sq_contains = (objobjproc)pystdcxx_set::sq_contains,
    .sq_inplace_concat = (binaryfunc)pystdcxx_set::sq_inplace_concat,
};



PyTypeObject pystdcxx_set::this_type = {
    .ob_base = PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "stdcxx.set",
    .tp_basicsize = sizeof(pystdcxx_set),
    .tp_itemsize = 0,
    .tp_dealloc = (destructor)pystdcxx_set::tp_dealloc,
    .tp_repr = (reprfunc)pystdcxx_set::tp_repr,
    .tp_as_sequence = &pystdcxx_set::tp_as_sequence,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,
    .tp_doc = PyDoc_STR("Python wrapper for std::set"),
    .tp_traverse = (traverseproc)pystdcxx_set::tp_traverse,
    .tp_clear = (inquiry)pystdcxx_set::tp_clear,
    .tp_iter = (getiterfunc)pystdcxx_set::tp_iter,
    .tp_methods = pystdcxx_set::tp_methods,
    .tp_init = (initproc)pystdcxx_set::tp_init,
    .tp_new = (newfunc)pystdcxx_set::tp_new,
};
