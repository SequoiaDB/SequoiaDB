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

   Source File Name = myclass2.hpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          12/1/2014  ly  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef MYCLASS2_HPP_
#define MYCLASS2_HPP_

#include "sptApi.hpp"

using namespace engine;

class myclass2 : public SDBObject
{
JS_DECLARE_CLASS(myclass2)

public:
   myclass2() ;
   virtual ~myclass2() ;

public:
   INT32 construct( const _sptParamContainer &arg,
                    _sptReturnVal &rval,
                    bson::BSONObj &detail) ;

   INT32 func( const _sptParamContainer &arg,
               _sptReturnVal &rval,
               bson::BSONObj &detail ) ;

   INT32 destruct() ; 
} ;

#endif

