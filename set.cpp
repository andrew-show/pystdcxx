#include "set.hpp"

PyMethodDef *pystdcxx_set::tp_methods()
{
    static PyMethodDef methods[] = {
        { "add",          (PyCFunction)pystdcxx_set::add,      METH_O,       "Add item" },
        { "remove",       (PyCFunction)pystdcxx_set::remove,   METH_O,       "Remove item" },
        { "clear",        (PyCFunction)pystdcxx_set::clear,    METH_NOARGS,  "Clear all items" },
        { "reverse",      (PyCFunction)pystdcxx_set::reverse,  METH_NOARGS,  "Find an item and return an iterator" },
        { "find",         (PyCFunction)pystdcxx_set::find,     METH_O,       "Find an item and return an iterator" },
        { "popitem",      (PyCFunction)pystdcxx_set::popitem,  METH_O,       "Pop and remove the first/last item" },
        { NULL },
    };

    return methods;
}

int pystdcxx_set::tp_init(pystdcxx_set *self, PyObject *args, PyObject *kwds)
{
    PyObject *tuple = nullptr, *less = nullptr;
    static const char *kwlist[] = { "tuple", "less", NULL };
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|O$O", const_cast<char **>(kwlist), &tuple, &less))
        return -1;

    if (less) {
        if (PyCallable_Check(less)) {
            self->less = py_ptr<PyObject>(less, true);
        } else if (!Py_IsNone(less)) {
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

PyObject *pystdcxx_set::tp_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    try {
        return reinterpret_cast<PyObject *>(new(type) pystdcxx_set());
    } catch ( ... ) {
        if (!PyErr_Occurred())
            PyErr_SetString(PyExc_RuntimeError, "Create set object failure");
        return NULL;
    }
}

int pystdcxx_set::tp_traverse(pystdcxx_set *self, visitproc visit, void *arg)
{
    if (self->less.get())
        Py_VISIT(self->less.get());

    for (stdcxx_set::iterator iter = self->set.begin(); iter != self->set.end(); ++iter)
        Py_VISIT(iter->get());

    return 0;
}

int pystdcxx_set::tp_clear(pystdcxx_set *self)
{
    stdcxx_set set(std::move(self->set));
    py_ptr<PyObject> less(self->less);
    return 0;
}

PyObject *pystdcxx_set::tp_repr(pystdcxx_set *self)
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

PyObject *pystdcxx_set::tp_iter(pystdcxx_set *self)
{
    return reinterpret_cast<PyObject *>(new iterator(self, self->set.begin(), self->set.end()));
}

Py_ssize_t pystdcxx_set::sq_length(pystdcxx_set *self)
{
    return self->set.size();
}

int pystdcxx_set::sq_contains(pystdcxx_set *self, PyObject *value)
{
    return self->set.find(py_ptr<PyObject>(value, true)) != self->set.end();
}

PyObject *pystdcxx_set::sq_inplace_concat(pystdcxx_set *self, PyObject *value)
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
        } else if (!PyObject_IsInstance(value, reinterpret_cast<PyObject *>(py_type<pystdcxx_set>::get()))) {
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

PyObject *pystdcxx_set::add(pystdcxx_set *self, PyObject *value)
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

PyObject *pystdcxx_set::remove(pystdcxx_set *self, PyObject *value)
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

PyObject *pystdcxx_set::clear(pystdcxx_set *self, PyObject *Py_UNUSED(args))
{
    self->set.clear();
    Py_RETURN_NONE;
}

PyObject *pystdcxx_set::reverse(pystdcxx_set *self, PyObject *Py_UNUSED(args))
{
    return reinterpret_cast<PyObject *>(new reverse_iterator(self, self->set.rbegin(), self->set.rend()));
}

PyObject *pystdcxx_set::find(pystdcxx_set *self, PyObject *value)
{
    try {
        return reinterpret_cast<PyObject*>(new iterator(self,
                                                        self->set.find(py_ptr<PyObject>(value, true)),
                                                        self->set.end()));
    } catch ( ... ) {
        if (!PyErr_Occurred())
            PyErr_SetString(PyExc_RuntimeError, "Unknown error");
        return NULL;
    }
}

PyObject *pystdcxx_set::iterator::tp_iter(pystdcxx_set::iterator *self)
{
    Py_INCREF(self);
    return reinterpret_cast<PyObject *>(self);
}

PyObject *pystdcxx_set::iterator::tp_iternext(pystdcxx_set::iterator *self)
{
    if (self->version != self->owner->version) {
        PyErr_SetString(PyExc_RuntimeError, "Can't change set while iterating");
        return NULL;
    }

    if (self->first == self->last)
        return NULL;

    PyObject *item = self->first->get();
    ++self->first;

    Py_INCREF(item);
    return item;
}

PyObject *pystdcxx_set::reverse_iterator::tp_iter(pystdcxx_set::reverse_iterator *self)
{
    Py_INCREF(self);
    return reinterpret_cast<PyObject *>(self);
}

PyObject *pystdcxx_set::reverse_iterator::tp_iternext(pystdcxx_set::reverse_iterator *self)
{
    if (self->version != self->owner->version) {
        PyErr_SetString(PyExc_RuntimeError, "Can't change set while iterating");
        return NULL;
    }

    if (self->first == self->last)
        return NULL;

    PyObject *item = self->first->get();
    ++self->first;

    Py_INCREF(item);
    return item;
}
