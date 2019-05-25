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

   Source File Name = dmsTmpBlkUnit.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of OSS component. This file contains declares for IO operations.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef DMSTMPBLKUNIT_HPP_
#define DMSTMPBLKUNIT_HPP_

#include "ossIO.hpp"
#include <string>
#include <list>

#define DMS_TMP_BLK_FILE_BEGIN "TMP_"

namespace engine
{
   class _dmsTmpBlkUnit ;

   class _dmsTmpBlk
   {
   public:
      _dmsTmpBlk() ;
      ~_dmsTmpBlk() ;

   public:
      void reset()
      {
         _read = 0 ;
      }

      std::string toString()const ;

      const UINT64 &size() const
      {
         return _size ;
      }

   private:
      UINT64 _begin ;
      UINT64 _size ;
      UINT64 _read ;

      friend class _dmsTmpBlkUnit ;
   } ;
   typedef class _dmsTmpBlk dmsTmpBlk ;

   typedef std::list<_dmsTmpBlk> RTN_SORT_BLKS ;
   typedef SINT64 DMS_TMP_FILE_ID ;

   class _dmsTmpBlkUnit
   {
   public:
      _dmsTmpBlkUnit() ;
      virtual ~_dmsTmpBlkUnit() ;

   public:
      OSS_INLINE UINT64 totalSize() const
      {
         return _totalSize ;
      }

      OSS_INLINE BOOLEAN isOpened() const
      {
         return _opened ;
      }

      INT32 openFile( const CHAR *path,
                      const DMS_TMP_FILE_ID &id ) ;

      INT32 write( const void *buf,
                   UINT64 size,
                   BOOLEAN extendSize ) ;

      INT32 seek( const UINT64 &offset ) ;

      INT32 read( dmsTmpBlk &blk, UINT64 size,
                  void *buf, UINT64 &got ) ;

      INT32 buildBlk( const UINT64 &begin,
                      const UINT64 &size,
                      dmsTmpBlk &blk ) ;
   private:
      INT32 _removeFile() ;

   private:
      std::string _fullPath ;
      _OSS_FILE _file ;
      UINT64 _totalSize ;
      BOOLEAN _opened ;
   } ;
}

#endif

