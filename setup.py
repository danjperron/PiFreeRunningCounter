from distutils.core import setup, Extension

module1 = Extension('freeRunningCounter',
                     sources = ['freeRunningCounter.c'])

setup (name = 'freeRunningCounter',
       version = '1.1',
       description= 'Read the 64Bits free running 1mHZ counter',
       ext_modules= [module1])

