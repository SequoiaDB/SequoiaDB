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

   Source File Name = dpsLogRecordDef.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of DPS component. This file contains declare for dpsLogWrapper.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/27/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef DPSLOGRECORDDEF_HPP_
#define DPSLOGRECORDDEF_HPP_

#include "dpsDef.hpp"

namespace engine
{
   enum DPS_LOG_PUBLIC
   {
      DPS_LOG_PUBLIC_INVALID = 0,
      DPS_LOG_PUBLIC_BEGIN = 200,
      DPS_LOG_PUBLIC_FULLNAME = 201,
      DPS_LOG_PUBLIC_TRANSID = 202,
      DPS_LOG_PUBLIC_PRETRANS = 203,
      DPS_LOG_PUBLIC_RELATED_TRANS = 204,    // only for rollback trans,
      DPS_LOG_PUBLIC_FIRSTTRANS = 205
   } ;


   enum DPS_LOG_INSERT
   {
      DPS_LOG_INSERT_OBJ = 1,
   } ;

   enum DPS_LOG_UPDATE
   {
      DPS_LOG_UPDATE_OLDMATCH =1,
      DPS_LOG_UPDATE_OLDOBJ = 2,
      DPS_LOG_UPDATE_NEWMATCH = 3,
      DPS_LOG_UPDATE_NEWOBJ = 4,
      DPS_LOG_UPDATE_OLDSHARDINGKEY = 5,
      DPS_LOG_UPDATE_NEWSHARDINGKEY = 6
   } ;

   enum DPS_LOG_DELETE
   {
      DPS_LOG_DELETE_OLDOBJ = 1,
   } ;

   enum DPS_LOG_POP
   {
      DPS_LOG_POP_LID = 1,
      DPS_LOG_POP_DIRECTION = 2
   } ;

   enum DPS_LOG_CSCRT
   {
      DPS_LOG_CSCRT_CSNAME = 1,
      DPS_LOG_CSCRT_PAGESIZE = 2,
      DPS_LOG_CSCRT_LOBPAGESZ = 3,
      DPS_LOG_CSCRT_CSTYPE = 4
   } ;

   enum DPS_LOG_CSDEL
   {
      DPS_LOG_CSDEL_CSNAME = 1,
   } ;

   enum DPS_LOG_CSRENAME
   {
      DPS_LOG_CSRENAME_CSNAME       = 1,
      DPS_LOG_CSRENAME_NEWNAME      = 2
   } ;

   enum DPS_LOG_CLCRT
   {
      DPS_LOG_CLCRT_ATTRIBUTE = 1,
      DPS_LOG_CLCRT_COMPRESS_TYPE = 2,
      DPS_LOG_CLCRT_EXT_OPTIONS = 3
   } ;

   enum DPS_LOG_CLDEL
   {
   } ;

   enum DPS_LOG_IXCRT
   {
      DPS_LOG_IXCRT_IX = 1,
      DPS_LOG_IXCRT_IX_MODE = 2,
   } ;

   enum DPS_LOG_IXDEL
   {
      DPS_LOG_IXDEL_IX = 1,
   } ;

   enum DPS_LOG_CLRENAME
   {
      DPS_LOG_CLRENAME_CSNAME =1,
      DPS_LOG_CLRENAME_CLOLDNAME =2,
      DPS_LOG_CLRENAME_CLNEWNAME =3,
   } ;

   enum DPS_LOG_CLTRUNC
   {
   } ;

   enum DPS_LOG_TS_COMMIT
   {
   } ;

   enum DPS_LOG_TS_ROLLBACK
   {
   } ;

   enum DPS_LOG_INVALIDCATA
   {
      DPS_LOG_INVALIDCATA_TYPE = 1,
      DPS_LOG_INVALIDCATA_IXNAME
   } ;

   enum DPS_LOG_ROW
   {
      DPS_LOG_ROW_ROWDATA = 1,
   };

   enum DPS_LOG_LOB
   {
      DPS_LOG_LOB_OID = 1,
      DPS_LOG_LOB_SEQUENCE,
      DPS_LOG_LOB_OFFSET,
      DPS_LOG_LOB_HASH,
      DPS_LOG_LOB_LEN,
      DPS_LOG_LOB_DATA,
      DPS_LOG_LOB_PAGE,
      DPS_LOG_LOB_OLD_LEN,
      DPS_LOG_LOB_OLD_DATA,
      DPS_LOG_LOB_PAGE_SIZE
   } ;

   enum DPS_LOG_ANALYZE
   {
      DPS_LOG_ANALYZE_CSNAME = 1,
      DPS_LOG_ANALYZE_CLNAME,
      DPS_LOG_ANALYZE_IXNAME,
      DPS_LOG_ANALYZE_MODE
   } ;
}


#endif

