#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <pyerrors.h>
#include <stdbool.h>
#include <set>
#include <memory>
#include <stdexcept>

namespace {

struct Less
{
    bool operator()(PyObject *lhs, PyObject *rhs) const
    {
        int result = PyObject_RichCompareBool(lhs, rhs, Py_LT);
        if (result == -1)
            throw std::runtime_error("Compare two object error");

        if (result)
            return true;

        result = PyObject_RichCompareBool(lhs, rhs, Py_EQ);
        if (result == -1)
            throw std::runtime_error("Compare two object error");

        if (!result)
            return false;

        return lhs < rhs;
    }
};

typedef std::set<PyObject *, Less> stdcxx_multiset;

} // namespace

typedef struct pystdcxx_multiset {
    PyObject_HEAD
    stdcxx_multiset set;
} pystdcxx_multiset;

typedef struct pystdcxx_multiset_iterator {
    PyObject_HEAD
    pystdcxx_multiset *owner;
    stdcxx_multiset::iterator iter;
} pystdcxx_multiset_iterator;

static PyObject *pystdcxx_multiset_iterator_iter(pystdcxx_multiset_iterator *self, PyObject *Py_UNUSED(args))
{
    Py_INCREF(self);
    return reinterpret_cast<PyObject *>(self);
}

static PyObject *pystdcxx_multiset_iterator_iternext(pystdcxx_multiset_iterator *self, PyObject *Py_UNUSED(args))
{
    PyObject *result = NULL;
    if (self->iter != self->owner->set.end()) {
        result = *self->iter ++;
        Py_INCREF(result);
    }

    return result;
}

static void pystdcxx_multiset_iterator_dealloc(pystdcxx_multiset_iterator *self)
{
    self->iter.stdcxx_multiset::iterator::~iterator();
    PyObject_DEL(self);
}

static PyTypeObject pystdcxx_multiset_iterator_type = {
    .ob_base = PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "stdcxx.multiset.iterator",
    .tp_basicsize = sizeof(pystdcxx_multiset_iterator),
    .tp_itemsize = 0,
    .tp_dealloc = (destructor)pystdcxx_multiset_iterator_dealloc,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_doc = PyDoc_STR("Python wrapper for std::set"),
    .tp_iter = (getiterfunc)pystdcxx_multiset_iterator_iter,
    .tp_iternext = (iternextfunc)pystdcxx_multiset_iterator_iternext,
};


static PyObject *pystdcxx_multiset_add(pystdcxx_multiset *self, PyObject *value)
{
    try {
        bool result = self->set.insert(value).second;
        if (result)
            Py_INCREF(value);
        return PyBool_FromLong(result);
    } catch (std::exception &e) {
        PyErr_SetString(PyExc_RuntimeError, e.what());
        return NULL;
    } catch ( ... ) {
        PyErr_SetString(PyExc_RuntimeError, "Unknown error");
        return NULL;
    }
}

static PyObject *pystdcxx_multiset_remove(pystdcxx_multiset *self, PyObject *value)
{
    try {
        bool result = false;
        stdcxx_multiset::iterator iter = self->set.find(value);
        if (iter != self->set.end()) {
            Py_DECREF(*iter);
            self->set.erase(iter);
            result = true;
        }

        return PyBool_FromLong(result);
    } catch (std::exception &e) {
        PyErr_SetString(PyExc_RuntimeError, e.what());
        return NULL;
    } catch ( ... ) {
        PyErr_SetString(PyExc_RuntimeError, "Unknown error");
        return NULL;
    }
}

static PyObject *pystdcxx_multiset_iter(pystdcxx_multiset *self, PyObject *Py_UNUSED(args))
{
    pystdcxx_multiset_iterator *result = PyObject_NEW(pystdcxx_multiset_iterator, &pystdcxx_multiset_iterator_type);
    if (!result)
        return NULL;

    result->owner = self;
    new (&result->iter) stdcxx_multiset::iterator(self->set.begin());
    Py_INCREF(self);

    return reinterpret_cast<PyObject *>(result);
}

static PyObject *pystdcxx_multiset_len(pystdcxx_multiset *self, PyObject *Py_UNUSED(args))
{
    return PyLong_FromLong(self->set.size());
}

static PyObject *pystdcxx_multiset_contains(pystdcxx_multiset *self, PyObject *value)
{
    return PyBool_FromLong(self->set.find(value) == self->set.end());
}

static PyMethodDef pystdcxx_multiset_methods[] = {
    { "add",          (PyCFunction)pystdcxx_multiset_add,      METH_O,       "Add item" },
    { "remove",       (PyCFunction)pystdcxx_multiset_remove,   METH_O,       "Remove item" },
    { "__iter__",     (PyCFunction)pystdcxx_multiset_iter,     METH_NOARGS,  "Iterate over the set" },
    { "__len__",      (PyCFunction)pystdcxx_multiset_len,      METH_NOARGS,  "Get number of element" },
    { "__contains__", (PyCFunction)pystdcxx_multiset_contains, METH_O,       "If the set contains an element" },
    { NULL },
};

static PyObject *pystdcxx_multiset_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    pystdcxx_multiset *self = (pystdcxx_multiset *)PyType_GenericNew(type, args, kwds);
    if (!self)
        return NULL;

    new (&self->set) stdcxx_multiset();

    PyObject *tuple;
    if (!PyArg_ParseTuple(args, "|O", &tuple)) {
        Py_DECREF(self);
        return NULL;
    }

    if (tuple) {
        if (PyList_Check(tuple)) {
            int n = PyList_GET_SIZE(tuple);
            for (int i = 0; i < n; ++i) {
                PyObject *item = PyList_GET_ITEM(tuple, i);
                if (item) {
                    if (self->set.insert(item).second)
                        Py_INCREF(item);
                }
            }
        } else if (PyTuple_Check(tuple)) {
            int n = PyTuple_GET_SIZE(tuple);
            for (int i = 0; i < n; ++i) {
                PyObject *item = PyTuple_GET_ITEM(tuple, i);
                if (item) {
                    if (self->set.insert(item).second)
                        Py_INCREF(item);
                }
            }
        } else {
            Py_DECREF(self);
            PyErr_SetString(PyExc_ValueError, "Require list/tuple type");
            return NULL;
        }
    }

    return (PyObject *)self;
}

static void pystdcxx_multiset_dealloc(pystdcxx_multiset *self)
{
    for (stdcxx_multiset::iterator iter = self->set.begin(); iter != self->set.end(); ++iter)
        Py_DECREF(*iter);

    self->set.~stdcxx_multiset();
    Py_TYPE(self)->tp_free(self);
}

PyTypeObject pystdcxx_multiset_type = {
    .ob_base = PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "stdcxx.multiset",
    .tp_basicsize = sizeof(pystdcxx_multiset),
    .tp_itemsize = 0,
    .tp_dealloc = (destructor)pystdcxx_multiset_dealloc,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_doc = PyDoc_STR("Python wrapper for std::multiset"),
    .tp_iter = (getiterfunc)pystdcxx_multiset_iter,
    .tp_methods = pystdcxx_multiset_methods,
    .tp_new = (newfunc)pystdcxx_multiset_new,
};
