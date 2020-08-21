from setuptools import setup, find_packages

setup_requires = [
    ]

install_requires = [
    ]

dependency_links = [
    ]

setup(
    name='Web_Back',
    version='0.1',
    description='Backend for Web pages which shows results of trace-replay',
    author='jsy0404',
    author_email='pnmsegwyd@gmail.com',
    packages=find_packages(),
    install_requires=install_requires,
    entry_points={
        'console_scripts': [
            'Web_Back = Web_Back.main:main',
            ],
        },
    )
