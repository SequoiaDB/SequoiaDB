/*******************************************************************************


   Copyright (C) 2023-present SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

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
#include "ossMemPool.hpp"
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
   OSSFILE     _in ;
   OSSFILE     _out ;
   _fmpVM      *_vm ;
   CHAR        *_inBuf ;
   UINT32      _inBufSize ;
   INT32       _step ;

   /// global db info
   ossPoolString _svcname ;
   ossPoolString _userName ;
   ossPoolString _password ;

} ;

typedef class _fmpController fmpController ;

#endif

