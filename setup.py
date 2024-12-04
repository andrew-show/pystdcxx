import subprocess
from setuptools import setup, Extension


setup(name="pystdcxx",
      version='0.6.0',
      ext_modules=[
          Extension("stdcxx",
                    [ "pystdcxx.cpp", "set.cpp" ],
                    language='c++')]
      )

