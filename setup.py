# Setup for obj2las.cpp python bindings
import os
from setuptools import setup, Extension
from setuptools.command.build_ext import build_ext
import sys

ext_modules = [
    Extension(
        'obj2las',
        ['obj2las.cpp'],
        include_dirs=['/usr/include/eigen3'],
        language='c++'
    ),
]