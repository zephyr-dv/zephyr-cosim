
import os, stat
from setuptools import setup
from setuptools.command.install import install


setup(
  name = "zephyr-cosim",
  version = version,
  packages=['zephyr_cosim'],
  package_dir = {'' : 'src'},
  package_data = {'zephr_cosim': ['share/*']},
  author = "Matthew Ballance",
  author_email = "matt.ballance@gmail.com",
  description = ("Zephyr-cosim provides scripts and tools for including the Zephyr OS in simulation."),
  license = "Apache 2.0",
  keywords = ["SystemVerilog", "Verilog", "RTL", "Coverage"],
  url = "https://github.com/zephyr-cosim/zephyr-cosim",
  entry_points={
    'console_scripts': [
      'zephyr-cosim = zephyr_cosim.__main__:main'
    ]
  },
  setup_requires=[
    'setuptools_scm',
  ],
  install_requires=[
      'vte',
      'pyelftools',
      'pyyaml'
  ],
#  cmdclass={
#    'install': InstallCmd
#  },
)