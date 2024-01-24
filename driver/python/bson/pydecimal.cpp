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

#include "pydecimal.h"
#include "bsonDecimal.h"
#include "bsontypes.h"
#include "bsonobj.h"
#include "bsonobjbuilder.h"
#include "bsonobjiterator.h"
#include "bson-inl.h"

__METHOD_IMPLEMENT(create)
{
   INT32 rc = SDB_OK ;
   bson::bsonDecimal* decimal = NULL ;
   PyObject *decObject = NULL ;
   if (!PyArg_ParseTuple(args, ""))
   {
      // do nothing
   }

   decimal = new (std::nothrow) bson::bsonDecimal() ;
   if ( NULL == decimal )
   {
      // LOG ERROR
      rc = SDB_OOM ;
      goto error ;
   }

   decObject = (PyObject *)PyCObject_FromVoidPtr(decimal, NULL) ;

done:
   return (PyObject*)Py_BuildValue("(i,O)", rc, decObject) ;
error:
   goto done ;
}

__METHOD_IMPLEMENT(destroy)
{
   INT32 rc = SDB_OK ;
   bson::bsonDecimal* decimal = NULL ;
   PyObject* decObject = NULL ;
   if (!PyArg_ParseTuple(args, "O", &decObject))
   {
      rc = SDB_INVALIDARGS ;
      goto error ;
   }

   decimal = (bson::bsonDecimal *)PyCObject_AsVoidPtr(decObject) ;
   if (NULL == decimal)
   {
      rc = SDB_INVALIDARGS ;
      goto error ;
   }

   delete decimal ;
   decimal = NULL ;

done:
   return (PyObject *)Py_BuildValue("i", rc) ;
error:
   goto done ;
}

__METHOD_IMPLEMENT(init)
{
   INT32 rc = SDB_OK ;
   bson::bsonDecimal *decimal = NULL ;
   PyObject* decObject = NULL ;
   if (!PyArg_ParseTuple(args, "O", &decObject))
   {
      rc = SDB_INVALIDARGS ;
      goto error ;
   }

   decimal = (bson::bsonDecimal *)PyCObject_AsVoidPtr(decObject) ;
   if (NULL == decimal)
   {
      rc = SDB_INVALIDARGS ;
      goto error ;
   }

   decimal->init() ;
done:
   return (PyObject *)Py_BuildValue("i", rc) ;
error:
   goto done ;
}

__METHOD_IMPLEMENT(init2)
{
   INT32 rc = SDB_OK ;
   bson::bsonDecimal *decimal = NULL ;
   PyObject* decObject = NULL ;
   INT32 precision = 0 ;
   INT32 scale     = 0 ;
   if (!PyArg_ParseTuple(args, "Oii", &decObject, &precision, &scale))
   {
      rc = SDB_INVALIDARGS ;
      goto error ;
   }

   decimal = (bson::bsonDecimal *)PyCObject_AsVoidPtr(decObject) ;
   if (NULL == decimal)
   {
      rc = SDB_INVALIDARGS ;
      goto error ;
   }

   rc = decimal->init(precision, scale) ;
done:
   return (PyObject *)Py_BuildValue("i", rc) ;
error:
   goto done ;
}

__METHOD_IMPLEMENT(setZero)
{
   INT32 rc = SDB_OK ;
   bson::bsonDecimal *decimal = NULL ;
   PyObject* decObject = NULL ;
   if (!PyArg_ParseTuple(args, "O", &decObject))
   {
      rc = SDB_INVALIDARGS ;
      goto error ;
   }

   decimal = (bson::bsonDecimal *)PyCObject_AsVoidPtr(decObject) ;
   if (NULL == decimal)
   {
      rc = SDB_INVALIDARGS ;
      goto error ;
   }

   decimal->setZero() ;
done:
   return (PyObject *)Py_BuildValue("i", rc) ;
error:
   goto done ;
}

__METHOD_IMPLEMENT(isZero)
{
   INT32 rc = SDB_OK ;
   BOOLEAN iszero = FALSE ;
   bson::bsonDecimal *decimal = NULL ;
   PyObject* decObject = NULL ;
   if (!PyArg_ParseTuple(args, "O", &decObject))
   {
      rc = SDB_INVALIDARGS ;
      goto error ;
   }

   decimal = (bson::bsonDecimal *)PyCObject_AsVoidPtr(decObject) ;
   if (NULL == decimal)
   {
      rc = SDB_INVALIDARGS ;
      goto error ;
   }

   iszero = decimal->isZero() ;

done:
   return (PyObject *)Py_BuildValue("(i,i)", rc, iszero) ;
error:
   goto done ;
}

__METHOD_IMPLEMENT(setMin)
{
   INT32 rc = SDB_OK ;
   bson::bsonDecimal *decimal = NULL ;
   PyObject* decObject = NULL ;
   if (!PyArg_ParseTuple(args, "O", &decObject))
   {
      rc = SDB_INVALIDARGS ;
      goto error ;
   }

   decimal = (bson::bsonDecimal *)PyCObject_AsVoidPtr(decObject) ;
   if (NULL == decimal)
   {
      rc = SDB_INVALIDARGS ;
      goto error ;
   }

   decimal->setMin() ;
done:
   return (PyObject *)Py_BuildValue("i", rc) ;
error:
   goto done ;
}

__METHOD_IMPLEMENT(isMin)
{
   INT32 rc = SDB_OK ;
   BOOLEAN isMin = FALSE ;
   bson::bsonDecimal *decimal = NULL ;
   PyObject* decObject = NULL ;
   if (!PyArg_ParseTuple(args, "O", &decObject))
   {
      rc = SDB_INVALIDARGS ;
      goto error ;
   }

   decimal = (bson::bsonDecimal *)PyCObject_AsVoidPtr(decObject) ;
   if (NULL == decimal)
   {
      rc = SDB_INVALIDARGS ;
      goto error ;
   }

   isMin = decimal->isMin() ;

done:
   return (PyObject *)Py_BuildValue("(i,i)", rc, isMin) ;
error:
   goto done ;
}

__METHOD_IMPLEMENT(setMax)
{
   INT32 rc = SDB_OK ;
   bson::bsonDecimal *decimal = NULL ;
   PyObject* decObject = NULL ;
   if (!PyArg_ParseTuple(args, "O", &decObject))
   {
      rc = SDB_INVALIDARGS ;
      goto error ;
   }

   decimal = (bson::bsonDecimal *)PyCObject_AsVoidPtr(decObject) ;
   if (NULL == decimal)
   {
      rc = SDB_INVALIDARGS ;
      goto error ;
   }

   decimal->setMax() ;
done:
   return (PyObject *)Py_BuildValue("i", rc) ;
error:
   goto done ;
}

__METHOD_IMPLEMENT(isMax)
{
   INT32 rc = SDB_OK ;
   BOOLEAN isMax = FALSE ;
   bson::bsonDecimal *decimal = NULL ;
   PyObject* decObject = NULL ;
   if (!PyArg_ParseTuple(args, "O", &decObject))
   {
      rc = SDB_INVALIDARGS ;
      goto error ;
   }

   decimal = (bson::bsonDecimal *)PyCObject_AsVoidPtr(decObject) ;
   if (NULL == decimal)
   {
      rc = SDB_INVALIDARGS ;
      goto error ;
   }

   isMax = decimal->isMax() ;

done:
   return (PyObject *)Py_BuildValue("(i,i)", rc, isMax) ;
error:
   goto done ;
}

__METHOD_IMPLEMENT(fromInt)
{
   INT32 rc = SDB_OK ;
   INT64 value = 0 ;
   bson::bsonDecimal *decimal = NULL ;
   PyObject* decObject = NULL ;
   if (!PyArg_ParseTuple(args, "OL", &decObject, &value))
   {
      rc = SDB_INVALIDARGS ;
      goto error ;
   }

   decimal = (bson::bsonDecimal *)PyCObject_AsVoidPtr(decObject) ;
   if (NULL == decimal)
   {
      rc = SDB_INVALIDARGS ;
      goto error ;
   }

   rc = decimal->fromLong(value) ;

done:
   return (PyObject *)Py_BuildValue("i", rc) ;
error:
   goto done ;
}

__METHOD_IMPLEMENT(toInt)
{
   INT32 rc = SDB_OK ;
   INT64 value = 0 ;
   bson::bsonDecimal *decimal = NULL ;
   PyObject* decObject = NULL ;
   if (!PyArg_ParseTuple(args, "O", &decObject))
   {
      rc = SDB_INVALIDARGS ;
      goto error ;
   }

   decimal = (bson::bsonDecimal *)PyCObject_AsVoidPtr(decObject) ;
   if (NULL == decimal)
   {
      rc = SDB_INVALIDARGS ;
      goto error ;
   }

   rc = decimal->toLong( &value) ;

done:
   return (PyObject *)Py_BuildValue("(i,L)", rc, value) ;
error:
   goto done ;
}

__METHOD_IMPLEMENT(fromFloat)
{
   INT32 rc = SDB_OK ;
   FLOAT64 value = 0 ;
   bson::bsonDecimal *decimal = NULL ;
   PyObject* decObject = NULL ;
   if (!PyArg_ParseTuple(args, "Od", &decObject, &value))
   {
      rc = SDB_INVALIDARGS ;
      goto error ;
   }

   decimal = (bson::bsonDecimal *)PyCObject_AsVoidPtr(decObject) ;
   if (NULL == decimal)
   {
      rc = SDB_INVALIDARGS ;
      goto error ;
   }

   rc = decimal->fromDouble(value) ;

done:
   return (PyObject *)Py_BuildValue("i", rc) ;
error:
   goto done ;
}

__METHOD_IMPLEMENT(toFloat)
{
   INT32 rc = SDB_OK ;
   FLOAT64 value = 0 ;
   bson::bsonDecimal *decimal = NULL ;
   PyObject* decObject = NULL ;
   if (!PyArg_ParseTuple(args, "O", &decObject))
   {
      rc = SDB_INVALIDARGS ;
      goto error ;
   }

   decimal = (bson::bsonDecimal *)PyCObject_AsVoidPtr(decObject) ;
   if (NULL == decimal)
   {
      rc = SDB_INVALIDARGS ;
      goto error ;
   }

   rc = decimal->toDouble( &value) ;

done:
   return (PyObject *)Py_BuildValue("(i,d)", rc, value) ;
error:
   goto done ;
}

__METHOD_IMPLEMENT(fromString)
{
   INT32 rc = SDB_OK ;
   const CHAR* value = NULL ;
   bson::bsonDecimal *decimal = NULL ;
   PyObject* decObject = NULL ;
   if (!PyArg_ParseTuple(args, "Os", &decObject, &value))
   {
      rc = SDB_INVALIDARGS ;
      goto error ;
   }

   decimal = (bson::bsonDecimal *)PyCObject_AsVoidPtr(decObject) ;
   if (NULL == decimal)
   {
      rc = SDB_INVALIDARGS ;
      goto error ;
   }

   rc = decimal->fromString(value) ;

done:
   return (PyObject *)Py_BuildValue("i", rc) ;
error:
   goto done ;
}

__METHOD_IMPLEMENT(toString)
{
   INT32 rc = SDB_OK ;
   bson::bsonDecimal *decimal = NULL ;
   std::string str ;
   PyObject* decObject = NULL ;
   if (!PyArg_ParseTuple(args, "O", &decObject))
   {
      rc = SDB_INVALIDARGS ;
      goto error ;
   }

   decimal = (bson::bsonDecimal *)PyCObject_AsVoidPtr(decObject) ;
   if (NULL == decimal)
   {
      rc = SDB_INVALIDARGS ;
      goto error ;
   }

   str = decimal->toString() ;

done:
   return (PyObject *)Py_BuildValue("(i,s)", rc, str.c_str()) ;
error:
   goto done ;
}

__METHOD_IMPLEMENT(toJsonString)
{
   INT32 rc = SDB_OK ;
   bson::bsonDecimal *decimal = NULL ;
   std::string str ;
   PyObject* decObject = NULL ;
   if (!PyArg_ParseTuple(args, "O", &decObject))
   {
      rc = SDB_INVALIDARGS ;
      goto error ;
   }

   decimal = (bson::bsonDecimal *)PyCObject_AsVoidPtr(decObject) ;
   if (NULL == decimal)
   {
      rc = SDB_INVALIDARGS ;
      goto error ;
   }

   str = decimal->toJsonString() ;

done:
   return (PyObject *)Py_BuildValue("(i,s)", rc, str.c_str()) ;
error:
   goto done ;
}

__METHOD_IMPLEMENT(fromBsonValue)
{
   INT32 rc = SDB_OK ;
   INT32 vlen = 0 ;
   const CHAR* bsonValue = NULL ;
   bson::bsonDecimal *decimal = NULL ;
   PyObject* decObject = NULL ;
   if (!PyArg_ParseTuple(args, "Os#", &decObject, &bsonValue, &vlen))
   {
      rc = SDB_INVALIDARGS ;
      goto error ;
   }

   decimal = (bson::bsonDecimal *)PyCObject_AsVoidPtr(decObject) ;
   if (NULL == decimal)
   {
      rc = SDB_INVALIDARGS ;
      goto error ;
   }

   rc = decimal->fromBsonValue(bsonValue) ;
   if ( SDB_OK != rc )
   {
      goto error;
   }

   vlen = decimal->getSize() ;

done:
   return (PyObject *)Py_BuildValue("ii", rc, vlen) ;
error:
   goto done ;
}

__METHOD_IMPLEMENT(compareInt)
{
   INT32 rc = SDB_OK ;
   INT32 value = 0 ;
   bson::bsonDecimal *decimal = NULL ;
   PyObject* decObject = NULL ;
   if (!PyArg_ParseTuple(args, "Oi", &decObject, &value))
   {
      rc = SDB_INVALIDARGS ;
      goto error ;
   }

   decimal = (bson::bsonDecimal *)PyCObject_AsVoidPtr(decObject) ;
   if (NULL == decimal)
   {
      rc = SDB_INVALIDARGS ;
      goto error ;
   }

   rc = decimal->compare(value) ;

done:
   return (PyObject *)Py_BuildValue("i", rc) ;
error:
   goto done ;
}

__METHOD_IMPLEMENT(compare)
{
   INT32 rc = SDB_OK ;
   INT32 value = 0 ;
   bson::bsonDecimal *decimal = NULL ;
   bson::bsonDecimal *rhs = NULL ;
   PyObject* decObject = NULL ;
   PyObject* rhsObject = NULL ;
   if (!PyArg_ParseTuple(args, "OO", &decObject, &rhsObject))
   {
      rc = SDB_INVALIDARGS ;
      goto error ;
   }

   decimal = (bson::bsonDecimal *)PyCObject_AsVoidPtr(decObject) ;
   if (NULL == decimal)
   {
      rc = SDB_INVALIDARGS ;
      goto error ;
   }

   rhs = (bson::bsonDecimal *)PyCObject_AsVoidPtr(rhsObject) ;
   if (NULL == rhs)
   {
      rc = SDB_INVALIDARGS ;
      goto error ;
   }

   rc = decimal->compare(*rhs) ;

done:
   return (PyObject *)Py_BuildValue("i", rc) ;
error:
   goto done ;
}

__METHOD_IMPLEMENT(toBsonElement)
{
   INT32 rc = SDB_OK ;
   bson::bsonDecimal *decimal = NULL ;
   PyObject* decObject        = NULL ;
   bson::BSONObjBuilder bob ;
   bson::BSONObj obj ;
   bson::BSONElement ele ;

   if (!PyArg_ParseTuple(args, "O", &decObject))
   {
      rc = SDB_INVALIDARGS ;
      goto error ;
   }

   decimal = (bson::bsonDecimal *)PyCObject_AsVoidPtr(decObject) ;
   if (NULL == decimal)
   {
      rc = SDB_INVALIDARGS ;
      goto error ;
   }

   try
   {
      bob.append("key", *decimal) ;
      obj = bob.obj() ;
      ele = obj.getField("key") ;
   }
   catch(std::exception &e)
   {
      rc = SDB_INVALIDARGS ;
      goto error ;
   }

done:
#if PY_MAJOR_VERSION >= 3
   return (PyObject *)Py_BuildValue("(i,y#)", rc, ele.value(), ele.valuesize()) ;
#else
   return (PyObject *)Py_BuildValue("(i,s#)", rc, ele.value(), ele.valuesize()) ;
#endif
error:
   goto done ;
}

static PyMethodDef decimal_methods[] = {
   { "create",        create,        METH_VARARGS },
   { "destroy",       destroy,       METH_VARARGS },
   { "init",          init,          METH_VARARGS },
   { "init2",         init2,         METH_VARARGS },
   { "setZero",       setZero,       METH_VARARGS },
   { "isZero",        isZero,        METH_VARARGS },
   { "setMin",        setMin,        METH_VARARGS },
   { "isMin",         isMin,         METH_VARARGS },
   { "setMax",        setMax,        METH_VARARGS },
   { "isMax",         isMax,         METH_VARARGS },
   { "fromInt",       fromInt,       METH_VARARGS },
   { "toInt",         toInt,         METH_VARARGS },
   { "fromFloat",     fromFloat,     METH_VARARGS },
   { "toFloat",       toFloat,       METH_VARARGS },
   { "fromString",    fromString,    METH_VARARGS },
   { "toString",      toString,      METH_VARARGS },
   { "toJsonString",  toJsonString,  METH_VARARGS },
   { "fromBsonValue", fromBsonValue, METH_VARARGS },
   { "compareInt",    compareInt,    METH_VARARGS },
   { "compare",       compare,       METH_VARARGS },
   { "toBsonElement", toBsonElement, METH_VARARGS },
   { NULL, NULL }
} ;

CREATE_MODULE( bsondecimal, decimal_methods )
