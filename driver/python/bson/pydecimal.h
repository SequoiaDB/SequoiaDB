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

#ifndef _SDB_BSON_DECIMAL_H_
#define _SDB_BSON_DECIMAL_H_

#include "util.hpp"

#define __METHOD_IMPLEMENT(name) \
   __METHOD_DECLARE(name)

__METHOD_DECLARE(create);
__METHOD_DECLARE(destroy);
__METHOD_DECLARE(init);
__METHOD_DECLARE(init2);
__METHOD_DECLARE(setZero);
__METHOD_DECLARE(isZero);
__METHOD_DECLARE(setMin);
__METHOD_DECLARE(isMin);
__METHOD_DECLARE(setMax);
__METHOD_DECLARE(isMax);
__METHOD_DECLARE(fromInt);
__METHOD_DECLARE(toInt);
__METHOD_DECLARE(fromFloat);
__METHOD_DECLARE(toFloat);
__METHOD_DECLARE(fromString);
__METHOD_DECLARE(toString);
__METHOD_DECLARE(toJsonString);
__METHOD_DECLARE(fromBsonValue);
__METHOD_DECLARE(compareInt);
__METHOD_DECLARE(compare);
__METHOD_DECLARE(toBsonElememt);
#endif
