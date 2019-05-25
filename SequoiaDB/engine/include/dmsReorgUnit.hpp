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

   Source File Name = dmsReorgUnit.hpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/28/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef DMSREORGUNIT_HPP__
#define DMSREORGUNIT_HPP__

#include "core.hpp"
#include "oss.hpp"
#include "ossMmap.hpp"
#include "dms.hpp"
#include "dmsExtent.hpp"
#include "dmsCompress.hpp"
#include "ossUtil.hpp"
#include "ossMem.hpp"
#include "utilCompressor.hpp"
#include "../bson/bson.h"
#include "../bson/oid.h"
using namespace bson ;

namespace engine
{

   #define DMS_REORG_UNIT_EYECATCHER         "DMSREORG"
   #define DMS_REORG_UNIT_EYECATCHER_LEN     8

   class _pmdEDUCB ;

   /*
      _dmsReorgUnit define
   */
   class _dmsReorgUnit : public SDBObject
   {
   private :
      class _reorgUnitHead : public SDBObject
      {
      public :
         CHAR     _eyeCatcher [ DMS_REORG_UNIT_EYECATCHER_LEN ] ;
         INT32    _headerSize ;
         CHAR     _fileName [ OSS_MAX_PATHSIZE ] ;
         SINT32   _pageSize ;
      } ;

      SINT32      _pageSize ;
      INT32       _headSize ;
      BOOLEAN     _readOnly ;
      CHAR        _fileName [ OSS_MAX_PATHSIZE + 1 ] ;
      OSSFILE     _file ;
      CHAR        *_pCurrentExtent ;
      INT32       _currentExtentSize ;
      INT32       _buffSize ;

   protected:
      INT32       _init ( BOOLEAN createNew ) ;
      INT32       _allocateExtent ( INT32 requestSize ) ;
      INT32       _flushExtent () ;
      void        _initExtentHeader ( dmsExtent *extAddr, UINT16 numPages ) ;

   public :
      _dmsReorgUnit () ;
      ~_dmsReorgUnit () ;

      BOOLEAN isOpened() const ;

      INT32 open ( const CHAR *pFileName,
                   SINT32 pageSize,
                   BOOLEAN createNew ) ;

      void  close () ;
      INT32 cleanup () ;
      INT32 flush () ;

      const CHAR* getFileName() const { return _fileName ; }

      INT32 insertRecord ( BSONObj &obj,
                           _pmdEDUCB *cb,
                           dmsCompressorEntry *compEntry ) ;

      void  beginExport() ;
      INT32 getNextExtentSize ( SINT32 &size ) ;
      INT32 exportExtent( CHAR *pBuffer ) ;

   } ;
   typedef class _dmsReorgUnit dmsReorgUnit ;
}

#endif // DMSREORGUNIT_HPP__

