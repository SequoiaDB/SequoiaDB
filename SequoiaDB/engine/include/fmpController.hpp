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

   Source File Name = fmpController.hpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/19/2013  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef FMPCONTROLLER_HPP_
#define FMPCONTROLLER_HPP_

#include "core.hpp"
#include "ossIO.hpp"
#include "../bson/bson.h"

using namespace bson ;

class _fmpVM ;

class _fmpController
{
public:
   _fmpController() ;
   virtual ~_fmpController() ;

public:
   INT32 run() ;

private:
   INT32 _runLoop() ;

   INT32 _handleOneLoop( const BSONObj &obj,
                         INT32 step ) ;

   INT32 _readMsg( BSONObj &msg ) ;

   INT32 _writeMsg( const BSONObj &msg ) ;

   INT32 _createVM( SINT32 type ) ;

   void _clear() ;

private:
   OSSFILE _in ;
   OSSFILE _out ;
   _fmpVM *_vm ;
   CHAR *_inBuf ;
   UINT32 _inBufSize ;
   INT32  _step ;
} ;

typedef class _fmpController fmpController ;

#endif

