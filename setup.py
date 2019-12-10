from setuptools import setup, find_packages

setup(
    name="frame-serial",
    url="https://github.com/SimpleTestTools/FrameSerial",
    package_dir={"": "py"},
    packages=find_packages(where="py"),
    use_scm_version=True,
    setup_requires=["setuptools_scm"],
    install_requires=["cobs", "pyserial"],
    extras_require={"dev": ["black"],},
)
