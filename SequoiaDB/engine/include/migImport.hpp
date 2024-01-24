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

   Source File Name = migImport.hpp

   Descriptive Name = Migration Import Header

   When/how to use: this program may be used on binary and text-formatted
   versions of OSS component. This file contains declare for import operation

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          05/14/2014  JWH Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef MIGIMPORT_HPP__
#define MIGIMPORT_HPP__

#include "core.hpp"
#include "oss.hpp"
#include "../client/bson/bson.h"
#include "utilParseData.hpp"
#include "../client/client.h"
#include "../util/csv2rawbson.hpp"

//#define MIG_MAX_READ_BUFFER (256*1024*1024)
//#define MIG_INC_READ_BUFFER (4194304)

enum IMPRTTYPE
{
   MIGIMPRT_CSV = 0,
   MIGIMPRT_JSON
} ;

struct migImprtArg : public SDBObject
{
   CHAR      delChar ;
   CHAR      delField ;
   CHAR      delRecord ;
   IMPRTTYPE type ;
   INT32     insertNum ;
   BOOLEAN   isHeaderline ;
   BOOLEAN   linePriority ;
   BOOLEAN   autoAddField ;
   BOOLEAN   autoCompletion ;
   BOOLEAN   errorStop ;
   BOOLEAN   force ;
   CHAR     *pHostname ;
   CHAR     *pSvcname ;
   CHAR     *pUser ;
   CHAR     *pPassword ;
   CHAR     *pCSName ;
   CHAR     *pCLName ;
   CHAR     *pFile ;
   CHAR     *pFields ;
#ifdef SDB_SSL
   BOOLEAN   useSSL ;
#endif

   migImprtArg() : delChar(0),
                   delField(0),
                   delRecord(0),
                   type(MIGIMPRT_CSV),
                   insertNum(0),
                   isHeaderline(TRUE),
                   linePriority(TRUE),
                   autoAddField(TRUE),
                   autoCompletion(TRUE),
                   errorStop(TRUE),
                   force(FALSE),
                   pHostname(NULL),
                   pSvcname(NULL),
                   pUser(NULL),
                   pPassword(NULL),
                   pCSName(NULL),
                   pCLName(NULL),
                   pFile(NULL),
                   pFields(NULL)
   {
#ifdef SDB_SSL
      useSSL = FALSE ;
#endif
   }
} ;

class migImport : public SDBObject
{
private:
   migImprtArg        *_pMigArg ;
   _utilDataParser    *_pParser ;
   bson              **_ppBsonArray ;
   sdbConnectionHandle _gConnection ;
   sdbCSHandle         _gCollectionSpace ;
   sdbCollectionHandle _gCollection ;
   csvParser           _csvParser ;
private:
   INT32 _connectDB() ;
   INT32 _getCS( CHAR *pCSName ) ;
   INT32 _getCL( CHAR *pCLName ) ;
private:
   INT32 _importRecord ( bson **bsonObj ) ;
   INT32 _importRecord ( bson **bsonObj, UINT32 bsonNum ) ;
   INT32 _getRecord ( bson &record ) ;
   INT32 _run ( INT32 &total, INT32 &succeed ) ;
public:
   migImport () ;
   ~migImport () ;
   INT32 init ( migImprtArg *pMigArg ) ;
   INT32 run ( INT32 &total, INT32 &succeed ) ;
} ;

#endif
