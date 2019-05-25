/*******************************************************************************

   Copyright (C) 2011-2014 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the term of the GNU Affero General Public License, version 3,
   as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warrenty of
   MARCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program. If not, see <http://www.gnu.org/license/>.

*******************************************************************************/

#ifndef CREATEBSON_HPP_
#define CREATEBSON_HPP_

#include "core.hpp"



INT32 getObjData(const CHAR *key, CHAR *&str);

void getObjKey(CHAR *str);

void getObjIndexKey(CHAR *str);

void getObjRule(CHAR *str);

void getObjDelKey(CHAR *str);

void getObjIndexDelKey(CHAR *str);

#endif

