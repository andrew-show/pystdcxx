#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <pyerrors.h>
#include "set.hpp"
#include "map.hpp"

static PyModuleDef pystdcxx_def = {
    .m_base = PyModuleDef_HEAD_INIT,
    .m_name = "stdcxx",
    .m_doc = "Python wrapper for libstdc++",
    .m_size = -1,
    //.m_methods = pystdcxx_methods,
};

PyMODINIT_FUNC PyInit_stdcxx(void)
{
    if (PyType_Ready(py_type<pystdcxx_set>::get()) < 0)
        return NULL;

    
    if (PyType_Ready(py_type<pystdcxx_map>::get()) < 0)
        return NULL;

    py_ptr<PyObject> pystdcxx(PyModule_Create(&pystdcxx_def));
    if (!pystdcxx.get())
        return NULL;

    py_ptr<PyTypeObject> pystdcxx_set_type(py_type<pystdcxx_set>::get(), true);
    if (PyModule_AddObject(pystdcxx.get(), "set", (PyObject *)pystdcxx_set_type.get()) < 0)
        return NULL;

    py_ptr<PyTypeObject> pystdcxx_map_type(py_type<pystdcxx_map>::get(), true);
    if (PyModule_AddObject(pystdcxx.get(), "map", (PyObject *)pystdcxx_map_type.get()) < 0)
        return NULL;

    pystdcxx_set_type.release();
    pystdcxx_map_type.release();
    return pystdcxx.release();
}
