#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <pyerrors.h>

static PyModuleDef pystdcxx_def = {
    .m_base = PyModuleDef_HEAD_INIT,
    .m_name = "stdcxx",
    .m_doc = "Python wrapper for libstdc++",
    .m_size = -1,
    //.m_methods = pystdcxx_methods,
};

extern PyTypeObject pystdcxx_set_type;
extern PyTypeObject pystdcxx_multiset_type;

PyMODINIT_FUNC PyInit_stdcxx(void)
{
    if (PyType_Ready(&pystdcxx_set_type) < 0)
        return NULL;

    if (PyType_Ready(&pystdcxx_multiset_type) < 0)
        return NULL;

    PyObject *pystdcxx = PyModule_Create(&pystdcxx_def);
    if (!pystdcxx)
        return NULL;

    Py_INCREF(&pystdcxx_set_type);
    Py_INCREF(&pystdcxx_multiset_type);

    if ((PyModule_AddObject(pystdcxx, "set", (PyObject *)&pystdcxx_set_type) < 0) ||
        (PyModule_AddObject(pystdcxx, "multiset", (PyObject *)&pystdcxx_multiset_type) < 0)) {
        Py_DECREF(&pystdcxx_set_type);
        Py_DECREF(pystdcxx);
        return NULL;
    }

    return pystdcxx;
}
