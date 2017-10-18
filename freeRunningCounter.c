#include <stdio.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <Python.h>
#include <string.h>


/*
Copyright (c) <2017> <Daniel Perron>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/


/*
   Programmer : Daniel Perron
   Date       : 21 Septembre 2017
   software   : freeRunningCounter
   License     : MIT license
   version 1.1
               -  Add python3 compatibility.
               -  Read /proc/iomem  to select the correct IOMAP for the counter
               -  Add 32bits overflow to detect the rare case  the HIGH 32bits change.
                  betweens the low and high 32bits read.
*/



#define TIMER_OFFSET (4)


typedef union
{
  unsigned long word32[2];
  unsigned long long word64;
}union_word64_32;



static PyObject *freeRunningCounterError;

unsigned long * counter; // 32 bits pointer to get count
void *st_base;
int fd;

static PyObject * freeRunningCounter(PyObject *self, PyObject * args);


//  python function table

static PyMethodDef freeRunningCounterMethods[] = {
  { "get", freeRunningCounter,METH_NOARGS, "Get 64 bits value from the free running 1Mhz counter"},
  { NULL,NULL,0,NULL}
};

#if  PY_MAJOR_VERSION == 3


// python module definition
static struct PyModuleDef freeRunningCounterDef =
{
   PyModuleDef_HEAD_INIT,
   "freeRunningCounter",
   "usage: freeRunningCounter.get()\n",-1,
   freeRunningCounterMethods
};

#endif


// find which  raspberry Pi for the correct IOMAP

unsigned int ReturnIOMapAddress(void)
{
  // let's read /proc/iomem and check for soc/gpio
  // if found extract the first address of the line

  unsigned int Start,End;
  char line[1024];
  FILE * src;

  src = fopen("/proc/iomem","rt");
  if(src == NULL)
    {
      return 0 ;
    }
  while(!feof(src))
  {
    if(fgets(line,1024,src)==NULL) break;
     if(strstr(line,"/soc/gpio")!=NULL)
     {
        // found /soc/gpio
        // extract address
        if(sscanf(line,"%x-%x", &Start,&End) == 2)
        {
          fclose(src);
          return Start;
        }
     }

  }

fclose(src);

return 0; // not found


}



// define correct function for python2 or 3

#if  PY_MAJOR_VERSION == 3
PyMODINIT_FUNC
PyInit_freeRunningCounter(void)
#else
PyMODINIT_FUNC
initfreeRunningCounter(void)
#endif
{
   PyObject *m;
   unsigned int MapIO_base;

#if  PY_MAJOR_VERSION == 3
   m = PyModule_Create(&freeRunningCounterDef);
   #define RETURN(A) return(A)

#else
   m = Py_InitModule("freeRunningCounter", freeRunningCounterMethods);
   #define RETURN(A) return;

#endif

   if ( m == NULL)
      RETURN(NULL);

   freeRunningCounterError = PyErr_NewException("freeRunningCounter.error",NULL,NULL);
   Py_INCREF(freeRunningCounterError);
   PyModule_AddObject(m, "error", freeRunningCounterError);



   MapIO_base =  ReturnIOMapAddress();

   if(MapIO_base ==0) {
      PyErr_SetString(freeRunningCounterError,"Unable to locate GPIO IO Address. Failed");
      RETURN(NULL);
   }

   // OK MapIO_base point to GPIO but we need to point to
   // the free running timer let's adjust the Address
   MapIO_base = MapIO_base  - 0x200000 + 0x3000;


       // get access to system core memory
    if (-1 == (fd = open("/dev/mem", O_RDONLY))) {
      PyErr_SetString(freeRunningCounterError,"Unable to open /mem/dev. sudo ???");
        RETURN(NULL);
    }

    // map a specific page into process's address space
    if (MAP_FAILED == (st_base = mmap(NULL, 4096,
                        PROT_READ, MAP_SHARED, fd, MapIO_base))) {
      PyErr_SetString(freeRunningCounterError,"Unable to map IO memory! Failed");
        RETURN(NULL);
    }
    close(fd);

    // set up pointer, based on mapped page
    counter = (unsigned long *)((char *)st_base + TIMER_OFFSET);

   RETURN(m); 

}





static PyObject * freeRunningCounter(PyObject *self, PyObject * args)
{
  // create a local variable to check if the lower 32 bits overflow
  // Read High 32bits, reads low 32bits
  // read High 32bits. if both high values agree reports
  // if they  are different re-read lower 32bits



  // counter is a 32bits pointer then counter[0] will read the low 32bits
  //                                  counter[1] will read the high 32bits
    union_word64_32 t_counter;
    unsigned int CounterH1;

    CounterH1= counter[1];
    t_counter.word32[0]  = counter[0];
    t_counter.word32[1]  = counter[1];

    // do we have to same count for both High 32bits values
    if(CounterH1 != t_counter.word32[1])
       t_counter.word32[0] = counter[0];

   return PyLong_FromUnsignedLongLong((unsigned long long )t_counter.word64);
}
