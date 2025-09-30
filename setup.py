from setuptools import setup, Extension
from pybind11.setup_helpers import Pybind11Extension, build_ext

ext_modules = [
    Pybind11Extension(
        "simple_image_recon",
        ["python/pybind11_bindings.cpp", "src/simple_image_reconstructor.cpp"],
        include_dirs=["include"],
        cxx_std=14,
    ),
]

setup(
    ext_modules=ext_modules,
    cmdclass={"build_ext": build_ext},
    zip_safe=False,
)