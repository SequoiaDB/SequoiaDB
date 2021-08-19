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

   Source File Name = utilInsertResult.hpp

   Dependencies: N/A

   Restrictions: N/AdmsStorageDataCommon

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          02/13/2019   LYB Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef UTIL_INSERT_RESULT_HPP_
#define UTIL_INSERT_RESULT_HPP_

#include "oss.hpp"
#include "utilResult.hpp"

using namespace bson ;

namespace engine
{

   /*
      utilInsertResult define
   */
   class utilInsertResult : public utilWriteResult
   {
   public:
      utilInsertResult() ;
      virtual ~utilInsertResult() ;

   protected:
      virtual void      _resetStat() ;
      virtual void      _resetInfo() ;
      virtual void      _toBSON( BSONObjBuilder &builder ) const ;
      virtual BOOLEAN   _filterResultElement( const BSONElement &e ) const ;

   public:
      void     enableIndexErrInfo() ;
      void     disableIndexErrInfo() ;
      BOOLEAN  isEnaleIndexErrInfo() const ;

      void     enableReturnIDInfo() ;
      void     disableReturnIDInfo() ;
      BOOLEAN  isEnableReturnIDInfo() const ;

      UINT64               insertedNum() const { return _insertedNum ; }
      UINT64               duplicatedNum() const { return _duplicatedNum ; }

      void                 incInsertedNum( UINT64 step = 1 )
      {
         _insertedNum += step ;
      }

      void                 incDuplicatedNum( UINT64 step = 1 )
      {
         _duplicatedNum += step ;
      }

      void                 setReturnIDByObj( const BSONObj &obj ) ;
      BSONObj              getReturnIDObj() const ;

   private:
      UINT64               _insertedNum ;
      UINT64               _duplicatedNum ;
      BOOLEAN              _enableReturnID ;
      BSONObj              _returnIDObj ;
   } ;

   /*
      utilUpdateResult define
   */
   class utilUpdateResult : public utilInsertResult
   {
   public:
      utilUpdateResult() ;
      virtual ~utilUpdateResult() ;

   protected:
      virtual void      _resetStat() ;
      virtual void      _resetInfo() ;
      virtual void      _toBSON( BSONObjBuilder &builder ) const ;
      virtual BOOLEAN   _filterResultElement( const BSONElement &e ) const ;

   public:
      UINT64               updateNum() const { return _updatedNum ; }
      UINT64               modifiedNum() const { return _modifiedNum ; }

      void                 incUpdatedNum( UINT64 step = 1 )
      {
         _updatedNum += step ;
      }
      void                 incModifiedNum( UINT64 step = 1 )
      {
         _modifiedNum += step ;
      }

      void                 setCurrentField( BSONElement &errEle ) ;

   private:
      UINT64               _updatedNum ;
      UINT64               _modifiedNum ;
      BSONObj              _currentFieldObj ;

   } ;

   /*
      utilDeleteResult define
   */
   class utilDeleteResult : public utilWriteResult
   {
   public:
      utilDeleteResult() ;
      virtual ~utilDeleteResult() ;

      UINT64            deletedNum() const { return _deletedNum ; }

      void              incDeletedNum( UINT64 step = 1 )
      {
         _deletedNum += step ;
      }

   protected:
      virtual void      _resetStat() ;
      virtual void      _resetInfo() ;
      virtual void      _toBSON( BSONObjBuilder &builder ) const ;
      virtual BOOLEAN   _filterResultElement( const BSONElement &e ) const ;

   private:
      UINT64               _deletedNum ;

   } ;

}

#endif /* UTIL_INSERT_RESULT_HPP_ */

