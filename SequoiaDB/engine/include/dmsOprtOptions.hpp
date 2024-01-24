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

   Source File Name = dmsOprtOptions.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef SDB_DMS_ENGINE_OPTIONS_HPP_
#define SDB_DMS_ENGINE_OPTIONS_HPP_

#include "dms.hpp"
#include "dmsEngineDef.hpp"
#include "msgDef.h"
#include "utilCompression.hpp"
#include "utilRecycleItem.hpp"
#include "utilRenameLogger.hpp"
#include "utilResult.hpp"
#include "../bson/bson.hpp"

namespace engine
{

   /*
      _dmsOpenEngineOptions define
    */
   class _dmsOpenEngineOptions : public SDBObject
   {
   public:
      _dmsOpenEngineOptions() = default ;
      virtual ~_dmsOpenEngineOptions() = default ;
      _dmsOpenEngineOptions( const _dmsOpenEngineOptions &o ) = default ;
      _dmsOpenEngineOptions &operator =( const _dmsOpenEngineOptions & ) = default ;
   } ;

   typedef class _dmsOpenEngineOptions dmsOpenEngineOptions ;

   /*
      _dmsCloseEngineOptions define
    */
   class _dmsCloseEngineOptions : public SDBObject
   {
   public:
      _dmsCloseEngineOptions() = default ;
      virtual ~_dmsCloseEngineOptions() = default ;
      _dmsCloseEngineOptions( const _dmsCloseEngineOptions &o ) = default ;
      _dmsCloseEngineOptions &operator =( const _dmsCloseEngineOptions & ) = default ;
   } ;

   typedef class _dmsCloseEngineOptions dmsCloseEngineOptions ;

   /*
      _dmsCreateCSOptions define
    */
   class _dmsCreateCSOptions : public SDBObject
   {
   public:
      _dmsCreateCSOptions() = default ;
      virtual ~_dmsCreateCSOptions() = default ;
      _dmsCreateCSOptions( const _dmsCreateCSOptions &o ) = default ;
      _dmsCreateCSOptions &operator =( const _dmsCreateCSOptions & ) = default ;
   } ;

   typedef class _dmsCreateCSOptions dmsCreateCSOptions ;

   /*
      _dmsCappedCLOptions
    */
   class _dmsCappedCLOptions
   {
   public:
      INT64 _maxSize ;
      INT64 _maxRecNum ;
      BOOLEAN _overwrite ;

      _dmsCappedCLOptions()
      {
         _maxSize = DMS_DFT_CAPPEDCL_SIZE ;
         _maxRecNum = DMS_DFT_CAPPEDCL_RECNUM ;
         _overwrite = FALSE ;
      }
   } ;
   typedef class _dmsCappedCLOptions dmsCappedCLOptions ;

   /*
      _dmsCreateCLOptions define
    */
   class _dmsCreateCLOptions
   {
   public:
      _dmsCreateCLOptions() = default ;
      virtual ~_dmsCreateCLOptions() = default ;
      _dmsCreateCLOptions( const _dmsCreateCLOptions &o ) = default ;
      _dmsCreateCLOptions &operator =( const _dmsCreateCLOptions & ) = default ;

   public:
      UINT8 _compressorType = UTIL_COMPRESSOR_TYPE::UTIL_COMPRESSOR_SNAPPY ;

      // capped collection options
      dmsCappedCLOptions _cappedOptions ;
   } ;

   typedef class _dmsCreateCLOptions dmsCreateCLOptions ;

   /*
      _dmsRecycleOptions define
    */
   typedef struct _dmsRecycleOptions
   {
      _dmsRecycleOptions()
      : _blockOpID( 0 ),
        _localTaskID( 0 ),
        _needSaveItem( TRUE )
      {
      }

      _dmsRecycleOptions( const utilRecycleItem &recycleItem,
                          BOOLEAN needSaveItem )
      : _recycleItem( recycleItem ),
        _blockOpID( 0 ),
        _localTaskID( 0 ),
        _needSaveItem( needSaveItem )
      {
      }

      BOOLEAN isTakenOver() const
      {
         return _recycleItem.isValid() ;
      }

      utilRecycleItem _recycleItem ;
      UINT64          _blockOpID ;
      UINT64          _localTaskID ;
      BOOLEAN         _needSaveItem ;
   } dmsRecycleOptions ;

   /*
      _dmsTruncCLOptions define
    */
   typedef struct _dmsTruncCLOptions : public _dmsRecycleOptions
   {
      _dmsTruncCLOptions()
      : _dmsRecycleOptions(),
        _isPrepared( FALSE )
      {
      }

      _dmsTruncCLOptions( const utilRecycleItem &recycleItem,
                          BOOLEAN needSaveItem )
      : _dmsRecycleOptions( recycleItem, needSaveItem ),
        _isPrepared( FALSE )
      {
      }

      INT32 parseOptions( const bson::BSONObj &boOptions )
      {
         INT32 rc = SDB_OK ;

         try
         {
            if ( boOptions.hasField( FIELD_NAME_RECYCLE_ITEM ) )
            {
               rc = _recycleItem.fromBSON( boOptions,
                                           FIELD_NAME_RECYCLE_ITEM ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to parse recycle item, "
                            "rc: %d", rc ) ;
            }

            _boOptions = boOptions ;
            _isPrepared = TRUE ;
         }
         catch ( exception &e )
         {
            PD_LOG( PDERROR, "Failed to parse options, occur exception %s",
                    e.what() ) ;
            rc = ossException2RC( &e ) ;
            goto error ;
         }

      done:
         return rc ;

      error:
         goto done ;
      }

      INT32 prepareOptions()
      {
         INT32 rc = SDB_OK ;

         try
         {
            if ( !_isPrepared )
            {
               bson::BSONObjBuilder builder ;
               if ( _recycleItem.isValid() )
               {
                  rc = _recycleItem.toBSON( builder, FIELD_NAME_RECYCLE_ITEM ) ;
                  PD_RC_CHECK( rc, PDERROR, "Failed to build recycle item, "
                               "rc: %d", rc ) ;
                  _boOptions = builder.obj() ;
               }
               _isPrepared = TRUE ;
            }
         }
         catch ( exception &e )
         {
            PD_LOG( PDERROR, "Failed to build options, occur exception %s",
                    e.what() ) ;
            rc = ossException2RC( &e ) ;
            goto error ;
         }

      done:
         return rc ;

      error:
         goto done ;
      }

      bson::BSONObj _boOptions ;
      BOOLEAN       _isPrepared ;
   } dmsTruncCLOptions ;

   /*
      _dmsDropCLOptions define
    */
   typedef struct _dmsDropCLOptions : public _dmsTruncCLOptions
   {
      _dmsDropCLOptions()
      : _dmsTruncCLOptions()
      {
      }

      _dmsDropCLOptions( const utilRecycleItem &recycleItem,
                         BOOLEAN needSaveItem )
      : _dmsTruncCLOptions( recycleItem, needSaveItem )
      {
      }
   } dmsDropCLOptions ;

   /*
      _dmsDropCSOptions define
    */
   typedef struct _dmsDropCSOptions : public _dmsRecycleOptions
   {
      _dmsDropCSOptions()
      : _dmsRecycleOptions(),
        _isPrepared( FALSE ),
        _logger()
      {
      }

      _dmsDropCSOptions( const utilRecycleItem &recycleItem,
                         BOOLEAN needSaveItem )
      : _dmsRecycleOptions( recycleItem, needSaveItem ),
        _isPrepared( FALSE ),
        _logger()
      {
      }

      INT32 parseOptions( const bson::BSONObj &boOptions )
      {
         INT32 rc = SDB_OK ;

         try
         {
            if ( boOptions.hasField( FIELD_NAME_RECYCLE_ITEM ) )
            {
               rc = _recycleItem.fromBSON( boOptions,
                                           FIELD_NAME_RECYCLE_ITEM ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to parse recycle item, "
                            "rc: %d", rc ) ;
            }

            _boOptions = boOptions ;
            _isPrepared = TRUE ;
         }
         catch ( exception &e )
         {
            PD_LOG( PDERROR, "Failed to parse options, occur exception %s",
                    e.what() ) ;
            rc = ossException2RC( &e ) ;
            goto error ;
         }

      done:
         return rc ;

      error:
         goto done ;
      }

      INT32 prepareOptions()
      {
         INT32 rc = SDB_OK ;

         try
         {
            if ( !_isPrepared )
            {
               bson::BSONObjBuilder builder ;
               if ( _recycleItem.isValid() )
               {
                  rc = _recycleItem.toBSON( builder, FIELD_NAME_RECYCLE_ITEM ) ;
                  PD_RC_CHECK( rc, PDERROR, "Failed to build recycle item, "
                               "rc: %d", rc ) ;
                  _boOptions = builder.obj() ;
               }
               _isPrepared = TRUE ;
            }
         }
         catch ( exception &e )
         {
            PD_LOG( PDERROR, "Failed to build options, occur exception %s",
                    e.what() ) ;
            rc = ossException2RC( &e ) ;
            goto error ;
         }

      done:
         return rc ;

      error:
         goto done ;
      }

      bson::BSONObj _boOptions ;
      BOOLEAN       _isPrepared ;
      utilRenameLogger _logger ;
   } dmsDropCSOptions ;

   /*
      _dmsReturnOptions define
    */
   typedef struct _dmsReturnOptions : public _dmsRecycleOptions
   {
      _dmsReturnOptions()
      : _dmsRecycleOptions(),
        _isPrepared( FALSE )
      {
         _needSaveItem = FALSE ;
      }

      _dmsReturnOptions( const utilRecycleItem &recycleItem )
      : _dmsRecycleOptions( recycleItem, FALSE ),
        _isPrepared( FALSE )
      {
      }

      INT32 parseOptions( const bson::BSONObj &boOptions )
      {
         INT32 rc = SDB_OK ;

         try
         {
            if ( boOptions.hasField( FIELD_NAME_RECYCLE_ITEM ) )
            {
               rc = _recycleItem.fromBSON( boOptions,
                                           FIELD_NAME_RECYCLE_ITEM ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to parse recycle item, "
                            "rc: %d", rc ) ;
            }

            _boOptions = boOptions ;
            _isPrepared = TRUE ;
         }
         catch ( exception &e )
         {
            PD_LOG( PDERROR, "Failed to parse options, occur exception %s",
                    e.what() ) ;
            rc = ossException2RC( &e ) ;
            goto error ;
         }

      done:
         return rc ;

      error:
         goto done ;
      }

      INT32 prepareOptions()
      {
         INT32 rc = SDB_OK ;

         try
         {
            if ( !_isPrepared )
            {
               bson::BSONObjBuilder builder ;
               if ( _recycleItem.isValid() )
               {
                  rc = _recycleItem.toBSON( builder, FIELD_NAME_RECYCLE_ITEM ) ;
                  PD_RC_CHECK( rc, PDERROR, "Failed to build recycle item, "
                               "rc: %d", rc ) ;
                  _boOptions = builder.obj() ;
               }
               _isPrepared = TRUE ;
            }
         }
         catch ( exception &e )
         {
            PD_LOG( PDERROR, "Failed to build options, occur exception %s",
                    e.what() ) ;
            rc = ossException2RC( &e ) ;
            goto error ;
         }

      done:
         return rc ;

      error:
         goto done ;
      }

      bson::BSONObj _boOptions ;
      BOOLEAN       _isPrepared ;
   } dmsReturnOptions ;

   /*
      _dmsCreateIdxOptions define
    */
   class _dmsCreateIdxOptions : public SDBObject
   {
   public:
      _dmsCreateIdxOptions() = default ;
      virtual ~_dmsCreateIdxOptions() = default ;
      _dmsCreateIdxOptions( const _dmsCreateIdxOptions &o ) = default ;
      _dmsCreateIdxOptions &operator =( const _dmsCreateIdxOptions & ) = default ;
   } ;

   typedef class _dmsCreateIdxOptions dmsCreateIdxOptions ;

   /*
      _dmsDropIdxOptions define
    */
   class _dmsDropIdxOptions : public SDBObject
   {
   public:
      _dmsDropIdxOptions() = default ;
      virtual ~_dmsDropIdxOptions() = default ;
      _dmsDropIdxOptions( const _dmsDropIdxOptions &o ) = default ;
      _dmsDropIdxOptions &operator =( const _dmsDropIdxOptions & ) = default ;
   } ;

   typedef class _dmsDropIdxOptions dmsDropIdxOptions ;

   /*
      _dmsTruncateIdxOptions define
    */
   class _dmsTruncateIdxOptions : public SDBObject
   {
   public:
      _dmsTruncateIdxOptions() = default ;
      virtual ~_dmsTruncateIdxOptions() = default ;
      _dmsTruncateIdxOptions( const _dmsTruncateIdxOptions &o ) = default ;
      _dmsTruncateIdxOptions &operator =( const _dmsTruncateIdxOptions & ) = default ;
   } ;

   typedef class _dmsTruncateIdxOptions dmsTruncateIdxOptions ;

   /*
      _dmsCompactOptions define
    */
   class _dmsCompactOptions : public SDBObject
   {
   public:
      _dmsCompactOptions() = default ;
      virtual ~_dmsCompactOptions() = default ;
      _dmsCompactOptions( const _dmsCompactOptions &o ) = default ;
      _dmsCompactOptions &operator =( const _dmsCompactOptions & ) = default ;
   } ;

   typedef class _dmsCompactOptions dmsCompactOptions ;
   typedef class _dmsCompactOptions dmsCompactCLOptions ;
   typedef class _dmsCompactOptions dmsCompactIdxOptions ;
   typedef class _dmsCompactOptions dmsCompactLobOptions ;

} // namespace engine


#endif // SDB_DMS_ENGINE_OPTIONS_HPP_
