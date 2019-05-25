/*******************************************************************************


   Copyright (C) 2011-2017 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the term of the GNU Affero General Public License, version 3,
   as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warrenty of
   MARCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program. If not, see <http://www.gnu.org/license/>.

   Source File Name = seAdptIdxMetaMgr.hpp

   Descriptive Name = Search Engine Adapter Index Metadata Manager.

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains main function for sdbcm,
   which is used to do cluster managing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          10/09/2017  YSD  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef SEADPT_IDXMETAMGR_HPP__
#define SEADPT_IDXMETAMGR_HPP__

#include "core.hpp"
#include "oss.hpp"
#include "../bson/bson.hpp"
#include "ossRWMutex.hpp"

#include <string>

using bson::BSONObj ;
using engine::ossRWMutex ;

namespace seadapter
{
   class _seIndexMeta : public SDBObject
   {
      public:
         _seIndexMeta() ;
         _seIndexMeta( const CHAR *clFullName, const CHAR *idxName ) ;
         ~_seIndexMeta() ;

         void setVersion( INT64 version ) ;
         void setCLName( const CHAR *clFullName ) ;
         void setIdxName( const CHAR *idxName ) ;
         void setCSLogicalID( UINT32 logicalID ) ;
         void setCLLogicalID( UINT32 logicalID ) ;
         void setIdxLogicalID( UINT32 logicalID ) ;
         void setCappedCLName( const CHAR *cappedCLFullName ) ;
         INT32 setIdxDef( BSONObj &idxDef ) ;
         void setESIdxName( const CHAR *idxName ) ;
         void setESIdxType( const CHAR *type ) ;

         BOOLEAN valid() const
         {
            return ( !_origCLName.empty() && !_cappedCLName.empty() &&
                     !_origIdxName.empty() && !_esIdxName.empty() &&
                     !_esTypeName.empty() && !_indexDef.isEmpty() &&
                     _indexDef.valid() ) ;
         }

         BOOLEAN operator==( const _seIndexMeta &r ) const
         {
            return ( _origCLName == r._origCLName &&
                     _cappedCLName == r._cappedCLName &&
                     _origIdxName == r._origIdxName &&
                     _esIdxName == r._esIdxName &&
                     _esTypeName == r._esTypeName &&
                     _indexDef == r._indexDef &&
                     _csLogicalID == r._csLogicalID &&
                     _clLogicalID == r._clLogicalID &&
                     _idxLogicalID == r._idxLogicalID ) ;
         }

         INT64 getVersion() const
         {
            return _version ;
         }

         const string& getOrigCLName() const
         {
            return _origCLName ;
         }

         const string& getCappedCLName() const
         {
            return _cappedCLName ;
         }

         const string& getOrigIdxName() const
         {
            return _origIdxName ;
         }

         const string& getEsIdxName() const
         {
            return _esIdxName ;
         }

         const string& getEsTypeName() const
         {
            return _esTypeName ;
         }

         const BSONObj& getIdxDef() const
         {
            return _indexDef ;
         }

         std::string toString() const
         {
            try
            {
               std::stringstream ss ;
               ss << "original cl[" << _origCLName << "], "
                  << "capped cl[" << _cappedCLName << "], "
                  << "CS logical id[" << _csLogicalID << "], "
                  << "CL logical id[" << _clLogicalID << "], "
                  << "es index[" << _esIdxName << "], "
                  << "es type[" << _esTypeName << "], "
                  << "original index[" << _origIdxName << ", "
                  << _indexDef.toString() << "], "
                  << "original index logicalID[" << _idxLogicalID << "]" ;
               return ss.str() ;
            }
            catch ( std::exception &e )
            {
               PD_LOG( PDERROR, "Unexpected exception happened: %s",
                       e.what() ) ;
               return "" ;
            }
         }

      private:
         INT64       _version ;
         string      _origCLName ;
         string      _origIdxName ;
         UINT32      _csLogicalID ;
         UINT32      _clLogicalID ;
         UINT32      _idxLogicalID ;
         string      _cappedCLName ;
         string      _esIdxName ;
         string      _esTypeName ;
         BSONObj     _indexDef ;  // Used for fetching data from original collection.
   } ;
   typedef _seIndexMeta seIndexMeta ;

   typedef vector<seIndexMeta>            IDX_META_VEC ;
   typedef IDX_META_VEC::iterator         IDX_META_VEC_ITR ;
   typedef IDX_META_VEC::const_iterator   IDX_META_VEC_CITR ;
   typedef map<string, IDX_META_VEC>      IDX_META_MAP ;
   typedef IDX_META_MAP::iterator         IDX_META_MAP_ITR ;
   typedef IDX_META_MAP::const_iterator   IDX_META_MAP_CITR ;

   class _seIdxMetaMgr : public SDBObject
   {
      public:
         _seIdxMetaMgr() {}
         ~_seIdxMetaMgr() {}

         INT32 lock( OSS_LATCH_MODE mode = SHARED ) ;
         INT32 unlock( OSS_LATCH_MODE mode = SHARED ) ;

         INT32 addIdxMeta( const CHAR *clFullName,
                           const seIndexMeta &idxMeta ) ;
         INT32 rmIdxMeta( const CHAR *clFullName, const CHAR *idxName ) ;

         seIndexMeta* getIdxMeta( const seIndexMeta& idxMeta ) ;
         INT32 getIdxMetas( const CHAR *clFullName, IDX_META_VEC &idxMetaVec ) ;

         IDX_META_MAP* getIdxMetaMap() { return &_idxMetaMap ; }

         void cleanByVer( INT64 excludeVer ) ;
         void clear() ;

      private:
         IDX_META_MAP   _idxMetaMap ;
         ossRWMutex     _lock ;
   } ;
   typedef _seIdxMetaMgr seIdxMetaMgr ;
}

#endif /* SEADPT_IDXMETAMGR_HPP__ */

