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

   Source File Name = utilESBulkBuilder.hpp

   Descriptive Name = Elasticsearch bulk operation builder.

   When/how to use: this program may be used on binary and text-formatted
   versions of data management component. This file contains structure for
   DMS storage unit and its methods.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          14/04/2017  YSD Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef UTIL_ESBULKBUILDER_HPP__
#define UTIL_ESBULKBUILDER_HPP__

/*
 * The _bulk API in Elasticsearch is a cheaper and faster API. By using it it's
 * possible to perform many index/delete operations in a single API call.
 * The structure of data required by this API is as follows:
 *
 *          action_and_meta_data\n
 *          optional_source\n
 *          action_and_meta_data\n
 *          optional_source\n
 *          ...
 *          action_and_meta_data\n
 *          optional_source\n
 *
 * Note that the '\n' character at the end of each line is required, including
 * the last line.
 *
 * Here we provide a builder to generate the structual data shown above. We
 * building is done, we can get the data from it and pass to the _bulk API.
 */

#include "core.hpp"
#include "oss.hpp"
#include <string>

#define UTIL_ESBULK_MAX_SIZE        (16 * 1024 * 1024)
#define UTIL_ESBULK_DFT_SIZE        (10 * 1024 * 1024)

#define UTIL_ESBULK_MIN_META_SIZE   64

namespace seadapter
{
   enum _utilESBulkActionType
   {
      UTIL_ES_ACTION_INVALID = 0,
      UTIL_ES_ACTION_CREATE,     // If document does not exists, create it.
      UTIL_ES_ACTION_INDEX,      // Create a new document or replace an existsing one.
      UTIL_ES_ACTION_UPDATE,     // Partly updating a document.
      UTIL_ES_ACTION_DELETE      // Delete a document.
   } ;
   typedef _utilESBulkActionType utilESBulkActionType ;

   class _utilESBulkActionBase : public SDBObject
   {
      public:
         _utilESBulkActionBase( const CHAR *index, const CHAR *type ) ;
         virtual ~_utilESBulkActionBase() ;

         INT32 setID( const CHAR *id ) ;
         INT32 setSourceData( const CHAR *sourceData, INT32 length,
                              BOOLEAN copy = FALSE ) ;

         INT32 outSizeEstimate() const  ;
         INT32 output( CHAR *buffer, INT32 size, INT32 &length,
                       BOOLEAN withIndex = TRUE, BOOLEAN withType = TRUE,
                       BOOLEAN withID = TRUE ) const ;

         virtual utilESBulkActionType getType() const = 0 ;

      protected:
         INT32 _outputActionAndMeta( CHAR *buffer, INT32 size, INT32 &length,
                                     BOOLEAN withIndex = TRUE,
                                     BOOLEAN withType = TRUE,
                                     BOOLEAN withID = TRUE ) const ;

         virtual INT32 _outputSrcData( CHAR *buffer, INT32 size,
                                       INT32 &length ) const = 0 ;

         virtual const CHAR* _getActionName() const = 0 ;
         virtual BOOLEAN _hasSourceData() const = 0 ;

      protected:
         CHAR       *_sourceData ;
         INT32       _srcDataLen ;
         BOOLEAN     _ownData ;
      private:
         std::string _index ;
         std::string _type ;
         std::string _id ;
   } ;
   typedef _utilESBulkActionBase utilESBulkActionBase ;

   class _utilESActionCreate : public _utilESBulkActionBase
   {
      public:
         _utilESActionCreate( const CHAR *index, const CHAR *type ) ;
         virtual ~_utilESActionCreate() ;

         utilESBulkActionType getType() const ;

      private:
         virtual INT32 _outputSrcData( CHAR *buffer, INT32 size,
                                       INT32 &length ) const ;
         virtual const CHAR* _getActionName() const ;
         virtual BOOLEAN _hasSourceData() const ;
   } ;
   typedef _utilESActionCreate utilESActionCreate ;

   class _utilESActionIndex : public _utilESBulkActionBase
   {
      public:
         _utilESActionIndex( const CHAR *index, const CHAR *type ) ;
         virtual ~_utilESActionIndex() ;

         utilESBulkActionType getType() const ;

      private:
         virtual INT32 _outputSrcData( CHAR *buffer, INT32 size,
                                       INT32 &length ) const ;
         virtual const CHAR* _getActionName() const ;
         virtual BOOLEAN _hasSourceData() const ;
   } ;
   typedef _utilESActionIndex utilESActionIndex ;

   class _utilESActionUpdate : public _utilESBulkActionBase
   {
      public:
         _utilESActionUpdate( const CHAR *index, const CHAR *type ) ;
         virtual ~_utilESActionUpdate() ;

         utilESBulkActionType getType() const ;

      private:
         virtual INT32 _outputSrcData( CHAR *buffer, INT32 size,
                                       INT32 &length ) const ;
         virtual const CHAR* _getActionName() const ;
         virtual BOOLEAN _hasSourceData() const ;
   } ;
   typedef _utilESActionUpdate utilESActionUpdate ;

   class _utilESActionDelete : public _utilESBulkActionBase
   {
      public:
         _utilESActionDelete( const CHAR *index, const CHAR *type ) ;
         virtual ~_utilESActionDelete() ;

         utilESBulkActionType getType() const ;

      private:
         virtual INT32 _outputSrcData( CHAR *buffer, INT32 size,
                                       INT32 &length ) const ;
         virtual const CHAR* _getActionName() const ;
         virtual BOOLEAN _hasSourceData() const ;
   } ;
   typedef _utilESActionDelete utilESActionDelete ;

   class _utilESBulkBuilder : public SDBObject
   {
      public:
         _utilESBulkBuilder() ;
         ~_utilESBulkBuilder() ;

         INT32 init( UINT32 bufferSize = UTIL_ESBULK_DFT_SIZE ) ;
         BOOLEAN isInit() const { return ( NULL != _buffer ) ; }
         void reset() ;
         INT32 getFreeSize() const ;
         INT32 appendItem( const utilESBulkActionBase &item,
                           BOOLEAN withIndex = TRUE, BOOLEAN withType = TRUE,
                           BOOLEAN withID = TRUE ) ;
         const CHAR* getData() const { return _buffer ; }
         INT32 getDataLen() const { return _dataLen ; }

      private:
         CHAR  *_buffer ;
         INT32 _capacity ;
         INT32 _dataLen ;
   } ;
   typedef _utilESBulkBuilder utilESBulkBuilder ;
}

#endif /* UTIL_ESBULKBUILDER_HPP__ */

