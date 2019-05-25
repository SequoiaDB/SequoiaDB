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

#ifndef PHP_RESOURCE7_H__
#define PHP_RESOURCE7_H__

#include "php_driver.h"

#define PHP_REGISTER_RESOURCE( destroy, longDestroy, resourceName, resourceId )\
{\
   resourceId = zend_register_list_destructors_ex( destroy,\
                                                   longDestroy,\
                                                   resourceName,\
                                                   module_number ) ;\
}

#define MAKE_STD_ZVAL(z) zval _stack_zval_##z; z = &(_stack_zval_##z);

#define ALLOC_STD_ZVAL(z)\
{\
   (z) = (zval *) emalloc(sizeof(zval)) ;\
   ZVAL_NULL(z);\
}

#define PHP_SAVE_RESOURCE( thisObj, name, resource, resourceId )\
{\
   zval *pZvalResource = NULL ;\
   void *pResource = (void *)resource ;\
   MAKE_STD_ZVAL( pZvalResource ) ;\
   if( pResource == NULL )\
   {\
      ZVAL_NULL( pZvalResource ) ;\
   }\
   else\
   {\
      zend_resource *pTmpResource = zend_register_resource( pResource,\
                                                            resourceId ) ;\
      ZVAL_RES( pZvalResource, pTmpResource ) ;\
   }\
   zend_update_property( Z_OBJCE_P( thisObj ),\
                         thisObj,\
                         ZEND_STRL( name ),\
                         pZvalResource TSRMLS_CC ) ;\
}

#define PHP_SAVE_HANDLE( thisObj, handle, resourceId )\
{\
   PHP_SAVE_RESOURCE( thisObj, "_handle", handle, resourceId ) ;\
}

#define PHP_READ_RESOURCE( thisObj, name, resource, resourceType, resourceName, resourceId )\
{\
   zval *pZvalResource = NULL ;\
   PHP_READ_VAR( thisObj, name, pZvalResource ) ;\
   if( Z_TYPE_P( pZvalResource ) == IS_NULL )\
   {\
      resource = 0 ;\
   }\
   else\
   {\
      ZEND_FETCH_RESOURCE_NO_RETURN( resource,\
                                     resourceType,\
                                     &pZvalResource,\
                                     -1,\
                                     resourceName,\
                                     resourceId ) ;\
   }\
}

#define PHP_DEL_RESOURCE( thisObj, name )\
{\
   zval *pZvalResource = NULL ;\
   PHP_READ_VAR( thisObj, name, pZvalResource ) ;\
   zend_list_close( Z_RES_P( pZvalResource ) ) ;\
}

#define PHP_READ_HANDLE( thisObj, handle, handleType, resourceName, resourceId )\
{\
   PHP_READ_RESOURCE( thisObj, "_handle", handle, handleType, resourceName, resourceId ) ;\
}

#define PHP_DEL_HANDLE( thisObj )\
{\
   PHP_DEL_RESOURCE( thisObj, "_handle" ) ;\
}

#endif