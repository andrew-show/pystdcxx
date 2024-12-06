#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <pyerrors.h>
#include "set.hpp"

static PyModuleDef pystdcxx_def = {
    .m_base = PyModuleDef_HEAD_INIT,
    .m_name = "stdcxx",
    .m_doc = "Python wrapper for libstdc++",
    .m_size = -1,
    //.m_methods = pystdcxx_methods,
};

PyMODINIT_FUNC PyInit_stdcxx(void)
{
    PyTypeObject *pystdcxx_set_type = py_type<pystdcxx_set>::get();

    if (PyType_Ready(pystdcxx_set_type) < 0)
        return NULL;

    //if (PyType_Ready(&pystdcxx_multiset_type) < 0)
    //  return NULL;

    py_ptr<PyObject> pystdcxx(PyModule_Create(&pystdcxx_def));
    if (!pystdcxx.get())
        return NULL;

    py_ptr<PyTypeObject> pystdcxx_set_type_ptr(pystdcxx_set_type, true);
    if (PyModule_AddObject(pystdcxx.get(), "set", (PyObject *)pystdcxx_set_type) < 0)
        return NULL;

    //py_ptr<PyTypeObject> pystdcxx_multiset_type_ptr(&pystdcxx_multiset_type, true);
    //if (PyModule_AddObject(pystdcxx.get(), "multiset", (PyObject *)&pystdcxx_multiset_type) < 0)
    //    return NULL;

    pystdcxx_set_type_ptr.release();
    //pystdcxx_multiset_type_ptr.release();
    return pystdcxx.release();
}
