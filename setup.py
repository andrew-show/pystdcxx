import subprocess
from setuptools import setup, Extension


setup(name="pystdcxx",
      description="Python wrapper for C++ containers",
      version='0.6.0',
      author="Andrew Chow",
      author_email="andrew_show@hotmail.com",
      url="https://github.com/andrew-show/pystdcxx",
      ext_modules=[
          Extension("stdcxx",
                    [ "pystdcxx.cpp", "set.cpp", "map.cpp" ],
                    language='c++')]
      )

