/*******************************************************************************

   Copyright (C) 2011-2016 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the term of the GNU Affero General Public License, version 3,
   as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warrenty of
   MARCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program. If not, see <http://www.gnu.org/license/>.

   Source File Name = expExport.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who          Description
   ====== =========== ============ =============================================
          29/07/2016  Lin Yuebang  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef EXP_EXPORT_HPP_
#define EXP_EXPORT_HPP_

#include "oss.hpp"
#include "../client/client.h"
#include "../client/bson/bson.h"
#include "expOptions.hpp"
#include "expCL.hpp"
#include "expOutput.hpp"
#include <vector>

namespace exprt
{
   using namespace std ;

   class expCLExporter : public SDBObject
   {
   public :
      expCLExporter( expOptions &options, 
                     sdbConnectionHandle hConn,
                     const expCL &cl, 
                     expCLOutput &out, 
                     expConvertor &convertor  ) :
                     
                     _options(options), 
                     _hConn(hConn), 
                     _cl(cl), 
                     _out(out),
                     _convertor(convertor)
      {
      }
      INT32 run( UINT64 &exportedCount, UINT64 &failCount ) ;
   private :
      INT32 _query( sdbCollectionHandle &hCL, sdbCursorHandle &hCusor ) ;
      INT32 _exportRecords( sdbCursorHandle hCusor, 
                            UINT64 &exportedCount, UINT64 &failCount ) ;
   private :
      expOptions           &_options ;
      sdbConnectionHandle  _hConn ;
      const expCL          &_cl ;
      expCLOutput          &_out ;
      expConvertor         &_convertor ;
   } ;

   class expRoutine : public SDBObject
   {
   public :
      explicit expRoutine( expOptions &options ) : 
                                                   _options(options),
                                                   _clSet(options),
                                                   _exportedCLCount(0),
                                                   _failCLCount(0),
                                                   _exportedRecordCount(0),
                                                   _failRecordCount(0)
      {
      }
      INT32 run() ;
      void  printStatistics() ;
   private :
      INT32 _exportOne( sdbConnectionHandle hConn, const expCL &cl ) ;
      INT32 _export( sdbConnectionHandle hConn ) ;
      INT32 _connectDB( sdbConnectionHandle &hConn ) ;
      void  _getCLFileName( const expCL &cl, string &fileName ) ;
   private :
      expOptions &_options ;
      expCLSet    _clSet ;
      UINT32      _exportedCLCount ;
      UINT32      _failCLCount ;
      UINT64      _exportedRecordCount ;
      UINT64      _failRecordCount ;
   } ;
}

#endif