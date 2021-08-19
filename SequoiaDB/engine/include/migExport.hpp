/*******************************************************************************


   Copyright (C) 2011-2018 SequoiaDB Ltd.

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

   Source File Name = migExport.hpp

   Descriptive Name = Migration Export Header

   When/how to use: this program may be used on binary and text-formatted
   versions of OSS component. This file contains declare for export operation

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft
          05/22/2014  HJW Initial Draft
   Last Changed =

*******************************************************************************/
#ifndef MIGEXPORT_HPP__
#define MIGEXPORT_HPP__

#include "core.hpp"
#include "oss.hpp"
#include "../client/bson/bson.h"
#include "../client/client.h"
#include "../util/utilDecodeRawbson.hpp"
#include "ossIO.hpp"

enum EXPRTTYPE
{
   MIGEXPRT_CSV = 0,
   MIGEXPRT_JSON
} ;

struct migExprtArg : public SDBObject
{
   CHAR      delChar ;
   CHAR      delField ;
   CHAR      delRecord ;
   EXPRTTYPE type ;
   BOOLEAN   include ;
   BOOLEAN   errorStop ;
   BOOLEAN   includeBinary ;
   BOOLEAN   includeRegex ;
   CHAR     *pHostname ;
   CHAR     *pSvcname ;
   CHAR     *pUser ;
   CHAR     *pPassword ;
   CHAR     *pCSName ;
   CHAR     *pCLName ;
   CHAR     *pFile ;
   CHAR     *pFields ;
   CHAR     *pFiter ;
   CHAR     *pSort ;
   CHAR     *pPrefInst ;
#ifdef SDB_SSL
   BOOLEAN   useSSL ;
#endif

   migExprtArg() : delChar(0),
                   delField(0),
                   delRecord(0),
                   type(MIGEXPRT_CSV),
                   include(TRUE),
                   errorStop(TRUE),
                   includeBinary(FALSE),
                   includeRegex(FALSE),
                   pHostname(NULL),
                   pSvcname(NULL),
                   pUser(NULL),
                   pPassword(NULL),
                   pCSName(NULL),
                   pCLName(NULL),
                   pFile(NULL),
                   pFields(NULL),
                   pFiter(NULL),
                   pSort(NULL),
                   pPrefInst(NULL)
   {
#ifdef SDB_SSL
      useSSL = FALSE ;
#endif
   }
} ;

#define MIG_COLLECTION_SPACE_SIZE 257

class migExport : public SDBObject
{
private:
   sdbConnectionHandle _gConnection ;
   sdbCSHandle         _gCollectionSpace ;
   sdbCollectionHandle _gCollection ;
   sdbCursorHandle     _gCSList ;
   sdbCursorHandle     _gCLList ;
   sdbCursorHandle     _gCursor ;
   INT32               _bufferSize ;
   BOOLEAN             _isOpen ;
   migExprtArg        *_pMigArg ;
   CHAR               *_pBuffer ;
   OSSFILE             _file ;
   CHAR                _fullName[ MIG_COLLECTION_SPACE_SIZE ] ;
   utilDecodeBson      _decodeBson ;
private:
   INT32 _writeData( CHAR *pBuffer, INT32 size ) ;
   INT32 _writeSubField( fieldResolve *pFieldRe, BOOLEAN isFirst ) ;
   INT32 _writeInclude() ;
   INT32 _connectDB() ;
   INT32 _getCSList() ;
   INT32 _getCLList() ;
   INT32 _getCS( const CHAR *pCSName ) ;
   INT32 _getCL( const CHAR *pCLName ) ;
   INT32 _query() ;
private:
   INT32 _reallocBuffer( CHAR **ppBuffer, INT32 size, INT32 newSize ) ;
   INT32 _writeRecord( bson *pbson ) ;
   INT32 _exportCL( const CHAR *pCSName, const CHAR *pCLName, INT32 &total ) ;
   INT32 _run( const CHAR *pCSName, const CHAR *pCLName, INT32 &total ) ;
public:
   migExport() ;
   ~migExport() ;
   INT32 init( migExprtArg *pMigArg ) ;
   INT32 run( INT32 &total ) ;
} ;

#endif
