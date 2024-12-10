#include "map.hpp"

PyMethodDef *pystdcxx_map::tp_methods()
{
    static PyMethodDef methods[] = {
        { "clear",        (PyCFunction)pystdcxx_map::clear,    METH_NOARGS,  "Clear all items" },
        { "reverse",      (PyCFunction)pystdcxx_map::reverse,  METH_NOARGS,  "Find an item and return an iterator" },
        { "find",         (PyCFunction)pystdcxx_map::find,     METH_O,       "Find an item and return an iterator" },
        { "popitem",      (PyCFunction)pystdcxx_map::popitem,  METH_VARARGS | METH_KEYWORDS,       "Pop and remove the first/last item" },
        { nullptr },
    };

    return methods;
}

int pystdcxx_map::tp_init(pystdcxx_map *self, PyObject *args, PyObject *kwds)
{
    PyObject *tuple = nullptr, *less = nullptr;
    static const char *kwlist[] = { "tuple", "less", nullptr };
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
            if (py_tuple_check(tuple)) {
                py_tuple_for_each(tuple, [self] (PyObject *item) {
                    PyObject *key = py_tuple_get_item(item, 0);
                    PyObject *value = py_tuple_get_item(item, 1);
                    if (!key || !value)
                        throw std::runtime_error("Invalie key/value pair");
                    self->map.emplace(py_ptr<PyObject>(key, true), py_ptr<PyObject>(value, true));
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

PyObject *pystdcxx_map::tp_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    try {
        return reinterpret_cast<PyObject *>(new(type) pystdcxx_map());
    } catch ( ... ) {
        if (!PyErr_Occurred())
            PyErr_SetString(PyExc_RuntimeError, "Create map object failure");
        return nullptr;
    }
}

int pystdcxx_map::tp_traverse(pystdcxx_map *self, visitproc visit, void *arg)
{
    if (self->less.get())
        Py_VISIT(self->less.get());

    for (stdcxx_map::iterator iter = self->map.begin(); iter != self->map.end(); ++iter) {
        Py_VISIT(iter->first.get());
        Py_VISIT(iter->second.get());
    }

    return 0;
}

int pystdcxx_map::tp_clear(pystdcxx_map *self)
{
    stdcxx_map map(std::move(self->map));
    py_ptr<PyObject> less(self->less);
    return 0;
}

PyObject *pystdcxx_map::tp_repr(pystdcxx_map *self)
{
    try {
        std::string repr("{");
        const char *comma = "";

        for (stdcxx_map::iterator iter = self->map.begin(); iter != self->map.end(); ++iter) {
            repr += comma;
            repr += "(";
            repr += py_repr(iter->first.get());
            repr += ", ";
            repr += py_repr(iter->second.get());
            repr += ")";
            comma = ", ";
        }

        repr += "}";
        return PyUnicode_DecodeUTF8(repr.c_str(), repr.size(), "ignore");
    } catch (std::exception &e) {
        if (!PyErr_Occurred())
            PyErr_SetString(PyExc_RuntimeError, e.what());
        return nullptr;
    }
}

PyObject *pystdcxx_map::tp_iter(pystdcxx_map *self)
{
    return reinterpret_cast<PyObject *>(new iterator(self, self->map.begin(), self->map.end()));
}

Py_ssize_t pystdcxx_map::sq_length(pystdcxx_map *self)
{
    return self->map.size();
}

int pystdcxx_map::sq_contains(pystdcxx_map *self, PyObject *value)
{
    return self->map.find(py_ptr<PyObject>(value, true)) != self->map.end();
}

PyObject *pystdcxx_map::sq_inplace_concat(pystdcxx_map *self, PyObject *tuple)
{
    size_t size = self->map.size();
    
    try {
        if (py_tuple_check(tuple)) {
            py_tuple_for_each(tuple, [self] (PyObject *item) {
                PyObject *key = py_tuple_get_item(item, 0);
                PyObject *value = py_tuple_get_item(item, 1);
                if (!key || !value)
                    throw std::runtime_error("Invalie key/value pair");
                self->map.emplace(py_ptr<PyObject>(key, true), py_ptr<PyObject>(value, true));
            });
        } else {
            PyErr_SetString(PyExc_ValueError, "Require list/tuple type");
            return nullptr;
        }
    } catch (std::exception &e) {
        if (size != self->map.size())
            ++self->version;
        if (!PyErr_Occurred())
            PyErr_SetString(PyExc_RuntimeError, e.what());
        return nullptr;
    }

    if (size != self->map.size())
        ++self->version;

    Py_INCREF(self);
    return reinterpret_cast<PyObject *>(self);
}

Py_ssize_t pystdcxx_map::mp_length(pystdcxx_map *self)
{
    return self->map.size();
}

PyObject *pystdcxx_map::mp_subscript(pystdcxx_map *self, PyObject *key)
{
    try {
        stdcxx_map::iterator iter = self->map.find(py_ptr<PyObject>(key, true));
        if (iter == self->map.end()) {
            PyErr_SetString(PyExc_KeyError, "Key error");
            return nullptr;
        }

        PyObject *value = iter->second.get();
        Py_INCREF(value);
        return value;
    } catch (std::exception &e) {
        if (!PyErr_Occurred())
            PyErr_SetString(PyExc_RuntimeError, e.what());
        return nullptr;
    } catch ( ... ) {
        if (!PyErr_Occurred())
            PyErr_SetString(PyExc_RuntimeError, "Unknown error");
        return nullptr;
    }
}

int pystdcxx_map::mp_ass_subscript(pystdcxx_map *self, PyObject *key, PyObject *value)
{
    try {
        if (!value) {
            if (self->map.erase(py_ptr<PyObject>(key, true)) <= 0) {
                PyErr_SetString(PyExc_KeyError, "Key error");
                return -1;
            }
        } else {
            self->map[py_ptr<PyObject>(key, true)] = py_ptr<PyObject>(value, true);
        }

        return 0;
    } catch (std::exception &e) {
        if (!PyErr_Occurred())
            PyErr_SetString(PyExc_RuntimeError, e.what());
        return -1;
    } catch ( ... ) {
        if (!PyErr_Occurred())
            PyErr_SetString(PyExc_RuntimeError, "Unknown error");
        return -1;
    }
}

PyObject *pystdcxx_map::clear(pystdcxx_map *self, PyObject *Py_UNUSED(args))
{
    self->map.clear();
    Py_RETURN_NONE;
}

PyObject *pystdcxx_map::reverse(pystdcxx_map *self, PyObject *Py_UNUSED(args))
{
    return reinterpret_cast<PyObject *>(new reverse_iterator(self, self->map.rbegin(), self->map.rend()));
}

PyObject *pystdcxx_map::find(pystdcxx_map *self, PyObject *key)
{
    try {
        return reinterpret_cast<PyObject*>(new iterator(self,
                                                        self->map.find(py_ptr<PyObject>(key, true)),
                                                        self->map.end()));
    } catch (std::exception &e) {
        if (!PyErr_Occurred())
            PyErr_SetString(PyExc_RuntimeError, e.what());
        return nullptr;
    } catch ( ... ) {
        if (!PyErr_Occurred())
            PyErr_SetString(PyExc_RuntimeError, "Unknown error");
        return nullptr;
    }
}

static PyObject *make_tuple(PyObject *first, PyObject *second) {
    PyObject *tuple = PyTuple_New(2);
    if (!tuple)
        return nullptr;

    Py_INCREF(first);
    PyTuple_SET_ITEM(tuple, 0, first);
    Py_INCREF(second);
    PyTuple_SET_ITEM(tuple, 1, second);
    return tuple;
}

PyObject *pystdcxx_map::popitem(pystdcxx_map *self, PyObject *args, PyObject *kwds)
{
    PyObject *is_last = nullptr;
    static const char *kwlist[] = { "last", nullptr };
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|O", const_cast<char **>(kwlist), &is_last))
        return NULL;

    if (self->map.empty()) {
        PyErr_SetString(PyExc_ValueError, "Empty map");
        return NULL;
    }

    stdcxx_map::iterator iter;
    if (is_last && PyObject_IsTrue(is_last)) {
        iter = self->map.end();
        --iter;
    } else {
        iter = self->map.begin();
    }

    PyObject *tuple = make_tuple(iter->first.get(), iter->second.get());
    self->map.erase(iter);

    return tuple;
}

PyObject *pystdcxx_map::iterator::tp_iter(pystdcxx_map::iterator *self)
{
    Py_INCREF(self);
    return reinterpret_cast<PyObject *>(self);
}

PyObject *pystdcxx_map::iterator::tp_iternext(pystdcxx_map::iterator *self)
{
    if (self->version != self->owner->version) {
        PyErr_SetString(PyExc_RuntimeError, "Can't change map while iterating");
        return nullptr;
    }

    if (self->first == self->last)
        return nullptr;

    PyObject *tuple = make_tuple(self->first->first.get(), self->first->second.get());
    ++self->first;

    return tuple;
}

PyObject *pystdcxx_map::reverse_iterator::tp_iter(pystdcxx_map::reverse_iterator *self)
{
    Py_INCREF(self);
    return reinterpret_cast<PyObject *>(self);
}

PyObject *pystdcxx_map::reverse_iterator::tp_iternext(pystdcxx_map::reverse_iterator *self)
{
    if (self->version != self->owner->version) {
        PyErr_SetString(PyExc_RuntimeError, "Can't change map while iterating");
        return nullptr;
    }

    if (self->first == self->last)
        return nullptr;

    PyObject *tuple = make_tuple(self->first->first.get(), self->first->second.get());
    ++self->first;

    return tuple;
}
