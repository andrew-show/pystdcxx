#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <pyerrors.h>
#include <set>
#include "utils.hpp"


typedef std::multiset<py_ptr<PyObject>, PyObjectLess> stdcxx_multiset;

typedef struct pystdcxx_multiset {
    PyObject_HEAD
    bool initialized;
    unsigned int version;
    stdcxx_multiset set;
} pystdcxx_multiset;

typedef struct pystdcxx_multiset_iterator {
    PyObject_HEAD
    py_ptr<pystdcxx_multiset> owner;
    uint32_t version;
    stdcxx_multiset::iterator first, last;
} pystdcxx_multiset_iterator;

static PyObject *pystdcxx_multiset_iterator_iter(pystdcxx_multiset_iterator *self, PyObject *Py_UNUSED(args))
{
    Py_INCREF(self);
    return reinterpret_cast<PyObject *>(self);
}

static PyObject *pystdcxx_multiset_iterator_iternext(pystdcxx_multiset_iterator *self, PyObject *Py_UNUSED(args))
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

static void pystdcxx_multiset_iterator_dealloc(pystdcxx_multiset_iterator *self)
{
    self->first.stdcxx_multiset::iterator::~iterator();
    self->last.stdcxx_multiset::iterator::~iterator();
    self->owner.~py_ptr<pystdcxx_multiset>();

    PyObject_Del(self);
}

static PyTypeObject pystdcxx_multiset_iterator_type = {
    .ob_base = PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "stdcxx.multiset_iterator",
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
        self->set.insert(py_ptr<PyObject>(value, true));
        ++self->version;
        Py_RETURN_NONE;
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
        size_t result = self->set.erase(py_ptr<PyObject>(value, true));
        if (result)
            ++self->version;
        return PyLong_FromLong(result);
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

    new (std::addressof(result->owner)) py_ptr<pystdcxx_multiset>(self, true);
    new (std::addressof(result->first)) stdcxx_multiset::iterator(self->set.begin());
    new (std::addressof(result->last)) stdcxx_multiset::iterator(self->set.end());
    result->version = self->version;

    return reinterpret_cast<PyObject *>(result);
}

static Py_ssize_t pystdcxx_multiset_len(pystdcxx_multiset *self, PyObject *Py_UNUSED(args))
{
    return self->set.size();
}

static int pystdcxx_multiset_contains(pystdcxx_multiset *self, PyObject *value)
{
    return self->set.find(py_ptr<PyObject>(value, true)) != self->set.end();
}

extern PyTypeObject pystdcxx_multiset_type;

static PyObject *pystdcxx_multiset_inplace_concat(pystdcxx_multiset *self, PyObject *value)
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
        } else if (!PyObject_IsInstance(value, (PyObject *)&pystdcxx_multiset_type)){
            self->set.insert(py_ptr<PyObject>(value, true));
        } else {
            PyErr_SetString(PyExc_ValueError, "Require list/tuple type");
            return NULL;
        }
    } catch (std::exception &e) {
        if (size != self->set.size())
            ++self->version;
        PyErr_SetString(PyExc_RuntimeError, e.what());
        return NULL;
    }

    if (size != self->set.size())
        ++self->version;

    Py_INCREF(self);
    return reinterpret_cast<PyObject *>(self);
}

static PyObject *pystdcxx_multiset_repr(pystdcxx_multiset *self)
{
    std::string result("{ ");

    for (stdcxx_multiset::iterator iter = self->set.begin(); iter != self->set.end(); ++iter) {
        if (result.length() > 2)
            result += ", ";
        result += py_object_repr(iter->get());
    }

    result += " }";
    return PyUnicode_DecodeUTF8(result.c_str(), result.size(), "ignore");
}

static PyMethodDef pystdcxx_multiset_methods[] = {
    { "add",          (PyCFunction)pystdcxx_multiset_add,      METH_O,       "Add item" },
    { "remove",       (PyCFunction)pystdcxx_multiset_remove,   METH_O,       "Remove item" },
    { NULL },
};

static PySequenceMethods pystdcxx_multiset_sequence_methods = {
    .sq_length = (lenfunc)pystdcxx_multiset_len,
    .sq_contains = (objobjproc)pystdcxx_multiset_contains,
    .sq_inplace_concat = (binaryfunc)pystdcxx_multiset_inplace_concat,
};

static PyObject *pystdcxx_multiset_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    py_ptr<pystdcxx_multiset> self((pystdcxx_multiset *)PyType_GenericNew(type, args, kwds));
    if (!self.get())
        return NULL;

    self->initialized = false;
    self->version = 0;

    return reinterpret_cast<PyObject *>(self.release());
}

static int pystdcxx_multiset_init(pystdcxx_multiset *self, PyObject *args, PyObject *kwds)
{
    py_ptr<PyObject> tuple, less;;
    static const char *kwlist[] = { "tuple", "less", NULL };
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|O$O", const_cast<char **>(kwlist), &tuple, &less))
        return -1;

    if (less.get()) {
        if (Py_IsNone(less.get())) {
            less.reset();
        } else if (!PyFunction_Check(less.get()) && !PyMethod_Check(less.get())) {
            PyErr_SetString(PyExc_ValueError, "less argument should be function/method type");
            return -1;
        }
    }

    new (std::addressof(self->set)) stdcxx_multiset(PyObjectLess(std::move(less)));
    self->initialized = true;

    if (tuple.get()) {
        try {
            if (PyList_Check(tuple.get())) {
                py_list_for_each(tuple.get(), [self] (PyObject *item) {
                    self->set.insert(py_ptr<PyObject>(item, true));
                });
            } else if (PyTuple_Check(tuple.get())) {
                py_tuple_for_each(tuple.get(), [self] (PyObject *item) {
                    self->set.insert(py_ptr<PyObject>(item, true));
                });
            } else {
                PyErr_SetString(PyExc_ValueError, "Require list/tuple type");
                return -1;
            }
        } catch (std::exception &e) {
            PyErr_SetString(PyExc_RuntimeError, e.what());
            return -1;
        }
    }

    return 0;
}

static void pystdcxx_multiset_dealloc(pystdcxx_multiset *self)
{
    if (self->initialized)
        self->set.~stdcxx_multiset();

    Py_TYPE(self)->tp_free(self);
}

PyTypeObject pystdcxx_multiset_type = {
    .ob_base = PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "stdcxx.multiset",
    .tp_basicsize = sizeof(pystdcxx_multiset),
    .tp_itemsize = 0,
    .tp_dealloc = (destructor)pystdcxx_multiset_dealloc,
    .tp_repr = (reprfunc)pystdcxx_multiset_repr,
    .tp_as_sequence = &pystdcxx_multiset_sequence_methods,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_doc = PyDoc_STR("Python wrapper for std::multiset"),
    .tp_iter = (getiterfunc)pystdcxx_multiset_iter,
    .tp_methods = pystdcxx_multiset_methods,
    .tp_init = (initproc)pystdcxx_multiset_init,
    .tp_new = (newfunc)pystdcxx_multiset_new,
};
