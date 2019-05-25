/*******************************************************************************

   Copyright (C) 2012-2014 SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

*******************************************************************************/
#ifndef _SDB_PYTHON_DRIVER_UTIL_HPP_
#define _SDB_PYTHON_DRIVER_UTIL_HPP_

#include "ossFeat.hpp"
#undef _XOPEN_SOURCE
#undef _POSIX_C_SOURCE
#include <Python.h>

#define SDB_OK          0
#define SDB_OOM         -2 
#define SDB_INVALIDARGS -6


/* some useful macros
 **/
#define PYOBJECT PyObject

#define __METHOD_DECLARE(name) \
   static PYOBJECT* name( PYOBJECT *self, PYOBJECT *args )

#define __METHOD_IMP(name) \
   __METHOD_DECLARE(name)

#define PARSE_PYTHON_ARGS PyArg_ParseTuple

#define NEW_CPPOBJECT( pObject, CLASSNAME ) \
   pObject = new (std::nothrow) CLASSNAME()

#define NEW_CPPOBJECT_INIT( pObject, CLASSNAME, pValue ) \
   pObject = new (std::nothrow) CLASSNAME ( pValue )

#define DELETE_CPPOBJECT( pObject ) \
   if ( NULL != pObject )           \
   {                                \
      delete pObject ;              \
      pObject = NULL ;              \
   } 

#define PY_NULL \
   Py_IncRef( Py_None ), Py_None ;
/*
 *@brief     macro to cast C++ object to a python object 
 **/

#if PY_MAJOR_VERSION >= 3 && PY_MINOR_VERSION >= 1

#define __OBJNAME "cppobj"

#define MAKE_PYOBJECT( cpp_object ) \
    ( PyObject * )PyCapsule_New( cpp_object, __OBJNAME, NULL )

#define PyCObject_FromVoidPtr( cpp_object, destructor ) \
    PyCapsule_New( cpp_object, __OBJNAME, destructor )

#define PyCObject_AsVoidPtr( pyobj ) \
    PyCapsule_GetPointer( pyobj, __OBJNAME)

#else

#define MAKE_PYOBJECT( cpp_object )  \
   ( PyObject * )PyCObject_FromVoidPtr( cpp_object, NULL )

#endif

#if PY_MAJOR_VERSION >= 3

#define PyString_Check PyUnicode_Check

#if PY_MINOR_VERSION >= 3
#define PyString_AsString PyUnicode_AsUTF8
#else
#error "python3 should be >= 3.3"
#endif

#endif

#define MAKE_RETURN_INT( ret_value ) \
   ( PyObject * )Py_BuildValue( "i", ret_value )

#define MAKE_RETURN_INT_INT( ret_value, int_value ) \
   ( PyObject * )Py_BuildValue( "(i,i)", ret_value, int_value )

#define MAKE_RETURN_INT_LONG( ret_value, long_value ) \
   ( PyObject * )Py_BuildValue( "(i,l)", ret_value, long_value )

#define MAKE_RETURN_INT_ULLONG( ret_value, ull_value ) \
   ( PyObject * )Py_BuildValue( "(i,K)", ret_value, ull_value )

#define MAKE_RETURN_INT_OBJECT( ret_value, py_object ) \
   ( PyObject * )Py_BuildValue( "(i,O)", ret_value, py_object )

#define MAKE_RETURN_INT_PYSTRING( ret_value, c_string ) \
   ( PyObject * )Py_BuildValue( "(i,s)", ret_value, c_string )

#define MAKE_RETURN_INT_PYSTRING_UINT( ret_value, c_string, c_stringsize ) \
   ( PyObject * )Py_BuildValue( "(i,s,k)", ret_value, c_string, c_stringsize )

#define MAKE_RETURN_INT_PYSTRING_SIZE( ret_value, c_string, c_stringsize ) \
   ( PyObject * )Py_BuildValue( "(i,s#)", ret_value, c_string, c_stringsize )

#define MAKE_RETURN_INT_INT_PYSTRING_SIZE( ret_value, type_value, c_string, c_stringsize ) \
   ( PyObject * )Py_BuildValue( "(i,i,s#)", ret_value, type_value, c_string, c_stringsize )

#define MAKE_RETURN_INT_INT_INT_INT_STRING( verion, sub_verion, fixed, release, build)\
   ( PyObject * )Py_BuildValue( "(i,i,i,i,s)", version, sub_version, fixed, release, build )

#if PY_MAJOR_VERSION >= 3
#define MAKE_RETURN_INT_PYBYTES_SIZE( ret_value, c_string, c_stringsize ) \
   ( PyObject * )Py_BuildValue( "(i,y#)", ret_value, c_string, c_stringsize )

#define MAKE_RETURN_INT_INT_PYBYTES_SIZE( ret_value, type_value, c_string, c_stringsize ) \
   ( PyObject * )Py_BuildValue( "(i,i,y#)", ret_value, type_value, c_string, c_stringsize )
#else
#define MAKE_RETURN_INT_PYBYTES_SIZE( ret_value, c_string, c_stringsize ) \
   ( PyObject * )Py_BuildValue( "(i,s#)", ret_value, c_string, c_stringsize )

#define MAKE_RETURN_INT_INT_PYBYTES_SIZE( ret_value, type_value, c_string, c_stringsize ) \
   ( PyObject * )Py_BuildValue( "(i,i,s#)", ret_value, type_value, c_string, c_stringsize )
#endif

/*
 *@brief    macro to cast python object to specified class object
 *@py_object object need to cast
 *@classname the class of the instance
 *@instance  [out] the pointer pointing to real object
 **/
#define CAST_PYOBJECT_TO_COBJECT( py_object, classname, instance )   \
   do                                                                \
   {                                                                 \
      void *tmp = PyCObject_AsVoidPtr( py_object ) ;                 \
      if ( NULL == tmp )                                             \
      {                                                              \
         rc = SDB_INVALIDARGS ;                                      \
         goto done ;                                                 \
      }                                                              \
                                                                     \
      instance = static_cast< classname * >( tmp ) ;                 \
      if ( NULL == instance )                                        \
      {                                                              \
         rc = SDB_INVALIDARGS ;                                      \
         goto done ;                                                 \
      }                                                              \
   }while( 0 )

/*
 *@brief    macro to new a c++ bson object, with initialize python bson object.
 *          it will new a  c++ object, and must be remember to delete.
 *@py_object object need to cast
 *@instance  [out] the pointer pointing to real object
 **/
#define CAST_PYBSON_TO_CPPBSON( py_object, bson_object )       \
   if ( Py_None == py_object )                                 \
   {                                                           \
      NEW_CPPOBJECT( bson_object, bson::BSONObj ) ;            \
   }                                                           \
   else                                                        \
   {                                                           \
      const char *tmp = PyBytes_AsString( py_object ) ;        \
      if ( NULL == tmp )                                       \
      {                                                        \
         rc = SDB_INVALIDARGS ;                                \
         goto done ;                                           \
      }                                                        \
                                                               \
      NEW_CPPOBJECT_INIT( bson_object, bson::BSONObj, tmp ) ;  \
      if ( NULL == bson_object )                               \
      {                                                        \
         rc = SDB_OOM ;                                        \
         goto done ;                                           \
      }                                                        \
   }

#define MAKE_PYLIST_TO_VECTOR( py_list, vec_bson )                      \
   do                                                                   \
   {                                                                    \
      if( !PyList_Check( py_list) )                                     \
      {                                                                 \
         rc = SDB_INVALIDARGS ;                                         \
         goto done ;                                                    \
      }                                                                 \
                                                                        \
      Py_ssize_t list_size = PyList_Size( py_list ) ;                   \
      for ( int idx = 0 ; idx < list_size ; ++idx )                     \
      {                                                                 \
         const bson::BSONObj *obj = NULL ;                              \
         CAST_PYBSON_TO_CPPBSON( PyList_GetItem( py_list, idx), obj ) ; \
         vec_bson.push_back( *obj ) ;                                   \
         DELETE_CPPOBJECT( obj ) ;                                      \
      }                                                                 \
   }while( FALSE ) 

#define MAKE_PYLIST_TO_BUFFER( py_list, buffer )                        \
   do                                                                   \
   {                                                                    \
      if( !PyList_Check( py_list) )                                     \
      {                                                                 \
         rc = SDB_INVALIDARGS ;                                         \
         goto done ;                                                    \
      }                                                                 \
                                                                        \
      Py_ssize_t list_size = PyList_Size( py_list ) ;                   \
      for ( int idx = 0 ; idx < list_size ; ++idx )                     \
      {                                                                 \
         SINT64 id = PyLong_AsLongLong(PyList_GetItem( py_list, idx)) ; \
         buffer[idx] = id;                                              \
      }                                                                 \
   }while( FALSE ) 

struct module_state {
    PyObject *error;
};

#define DEFINE_MODULE(modulename, methods)   \
static struct PyModuleDef moduledef = {      \
   PyModuleDef_HEAD_INIT,                    \
   #modulename,                              \
   NULL,                                     \
   sizeof(struct module_state),              \
   methods,                                  \
   NULL,                                     \
   NULL,                                     \
   NULL,                                     \
   NULL                                      \
};                                        

#if PY_MAJOR_VERSION >= 3                        
   #define INITERROR return NULL
   #define DECLARE_MODULE_FUN(modulename, methods)    \
      DEFINE_MODULE(modulename, methods)              \
      PyMODINIT_FUNC PyInit_##modulename(void)
#else                                       
   #define INITERROR return                        
   #define DECLARE_MODULE_FUN(modulename, methods) \
      PyMODINIT_FUNC init##modulename(void)
#endif                                       

#if PY_MAJOR_VERSION >= 3                       
   #define MODULE_CREATE(modulename, methods)   \
      m = PyModule_Create(&moduledef)               
#else                                      
   #define MODULE_CREATE(modulename, methods)   \
      m = Py_InitModule(modulename, methods)         
#endif                                  


#if PY_MAJOR_VERSION >= 3                       
   #define RETURN   return m
#else
   #define RETURN   return
#endif                                       
                                    
#define CREATE_MODULE( modulename, methods ) \
DECLARE_MODULE_FUN( modulename, methods )    \
{                                            \
   PyObject *m;                              \
   MODULE_CREATE(#modulename, methods) ;     \
   if (m == NULL)                            \
   {                                         \
      INITERROR;                             \
   }                                         \
                                             \
   RETURN ;                                  \
} 

#endif
