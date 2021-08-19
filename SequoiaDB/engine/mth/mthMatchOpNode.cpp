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

   Source File Name = mthMatchOpNode.cpp

   Descriptive Name = Method Match Operation Node

   When/how to use: this program may be used on binary and text-formatted
   versions of Method component. This file contains functions for matcher, which
   indicates whether a record matches a given matching rule.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/15/2016  LinYouBin  Initial Draft

   Last Changed =

*******************************************************************************/
#include "mthMatchOpNode.hpp"
#include "utilMemListPool.hpp"
#include "mthMatchTree.hpp"
#include "pd.hpp"
#include "pdTrace.hpp"
#include "mthTrace.hpp"
#include "mthDef.hpp"
#include "mthCommon.hpp"
#include "msgDef.hpp"

using namespace bson ;

namespace engine
{
   //************************_mthMatchFunc********************************
   _mthMatchFunc::_mthMatchFunc( _mthNodeAllocator *allocator )
   {
      _funcEle   = BSONObj().firstElement() ;
      _allocator = allocator ;
   }

   _mthMatchFunc::~_mthMatchFunc()
   {
      clear() ;
   }

   INT32 _mthMatchFunc::init( const CHAR *fieldName, const BSONElement &ele )
   {
      INT32 rc = SDB_OK ;
      rc = _fieldName.setFieldName( fieldName ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "set fieldName failed:fieldName=%s,rc=%d",
                 fieldName, rc ) ;
         goto error ;
      }

      _funcEle = ele ;

      rc = _init( fieldName, ele ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "_init failed:rc=%d", rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      clear() ;
      goto done ;
   }

   void _mthMatchFunc::clear()
   {
      _fieldName.clear() ;
      _funcEle = BSONObj().firstElement() ;
   }

   void* _mthMatchFunc::operator new ( size_t size,
                                       _mthNodeAllocator *allocator )
   {
      void *p = NULL ;
      if ( size > 0 )
      {
         // In order to know if the memory is allocated by malloc() when
         // deleting the object, reserve space for a flag at the head of the
         // allocated space.
         size_t reserveSize = size + MTH_MEM_TYPE_SIZE ;
         if ( allocator )
         {
            p = allocator->allocate( reserveSize ) ;
         }

         if ( NULL == p )
         {
            p = SDB_THREAD_ALLOC( reserveSize ) ;
            if ( NULL == p )
            {
               goto error ;
            }
            *(INT32 *)p = MTH_MEM_BY_DFT_ALLOCATOR ;
         }
         else
         {
            *(INT32 *)p = MTH_MEM_BY_USER_ALLOCATOR ;
         }
         // Seek address which can actually be used by the user.
         p = (CHAR *)p + MTH_MEM_TYPE_SIZE ;
      }

   done:
      return p ;
   error:
      goto done ;
   }

   void _mthMatchFunc::operator delete( void *p )
   {
      if ( p )
      {
         void *beginAddr = (void *)( (CHAR *)p - MTH_MEM_TYPE_SIZE ) ;
         // Only release memory allocted by SDB_THREAD_ALLOC().
         // Objects allocated by instances of _utilAllocator(allocator is not
         // NULL in new) will not be released seperately, as they are allocated
         // in a stack. They space is released when the allocator is destroyed.
         if ( MTH_MEM_BY_DFT_ALLOCATOR == *(INT32 *)beginAddr )
         {
            SDB_THREAD_FREE( beginAddr ) ;
         }
      }
   }

   void _mthMatchFunc::operator delete( void *p, _mthNodeAllocator *allocator )
   {
      _mthMatchFunc::operator delete( p ) ;
   }

   string _mthMatchFunc::toString()
   {
      BSONObj obj = toBson() ;
      return obj.toString() ;
   }

   BSONObj _mthMatchFunc::toBson()
   {
      BSONObjBuilder builder ;

      BSONObjBuilder b( builder.subobjStart( _fieldName.getFieldName() ) ) ;
      b.append( _funcEle ) ;
      b.doneFast() ;

      return builder.obj() ;
   }

   INT32 _mthMatchFunc::adjustIndexForReturnMatch( _utilArray< INT32 > &in,
                                                   _utilArray< INT32 > &out )
   {
      out.clear() ;
      return in.copyTo( out ) ;
   }

   //************************_mthMatchFuncABS********************************
   _mthMatchFuncABS::_mthMatchFuncABS( _mthNodeAllocator *allocator )
                    :_mthMatchFunc( allocator )
   {
   }

   _mthMatchFuncABS::~_mthMatchFuncABS()
   {
      clear() ;
   }

   INT32 _mthMatchFuncABS::call( const BSONElement &in, BSONObj &out )
   {
      INT32 rc = SDB_OK ;
      INT32 flag = 0 ;
      BSONObjBuilder builder ;

      rc = mthAbs( _fieldName.getFieldName(), in, builder, flag ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "mthAbs failed:rc=%d", rc ) ;
      }

      out = builder.obj() ;

      return rc ;
   }

   void _mthMatchFuncABS::clear()
   {
      _mthMatchFunc::clear() ;
   }

   INT32 _mthMatchFuncABS::getType()
   {
      return EN_MATCH_FUNC_ABS ;
   }

   const CHAR* _mthMatchFuncABS::getName()
   {
      return MTH_FUNCTION_STR_ABS ;
   }

   INT32 _mthMatchFuncABS::_init( const CHAR *fieldName,
                                  const BSONElement &ele )
   {
      if ( !ele.isNumber() || ele.numberInt() != 1 )
      {
         return SDB_INVALIDARG ;
      }

      return SDB_OK ;
   }

   //************************_mthMatchFuncCEILING********************************
   _mthMatchFuncCEILING::_mthMatchFuncCEILING( _mthNodeAllocator *allocator )
                        :_mthMatchFuncABS( allocator )
   {
   }

   _mthMatchFuncCEILING::~_mthMatchFuncCEILING()
   {
      clear() ;
   }

   INT32 _mthMatchFuncCEILING::call( const BSONElement &in, BSONObj &out )
   {
      INT32 rc = SDB_OK ;
      BSONObjBuilder builder ;

      rc = mthCeiling( _fieldName.getFieldName(), in, builder ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "mthCeiling failed:rc=%d", rc ) ;
      }

      out = builder.obj() ;

      return rc ;
   }

   INT32 _mthMatchFuncCEILING::getType()
   {
      return EN_MATCH_FUNC_CEILING ;
   }

   const CHAR* _mthMatchFuncCEILING::getName()
   {
      return MTH_FUNCTION_STR_CEILING ;
   }

   //************************_mthMatchFuncFLOOR********************************
   _mthMatchFuncFLOOR::_mthMatchFuncFLOOR( _mthNodeAllocator *allocator )
                      :_mthMatchFuncABS( allocator )
   {
   }

   _mthMatchFuncFLOOR::~_mthMatchFuncFLOOR()
   {
      clear() ;
   }

   INT32 _mthMatchFuncFLOOR::call( const BSONElement &in, BSONObj &out )
   {
      INT32 rc = SDB_OK ;
      BSONObjBuilder builder ;

      rc = mthFloor( _fieldName.getFieldName(), in, builder ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "mthCeiling failed:rc=%d", rc ) ;
      }

      out = builder.obj() ;

      return rc ;
   }

   INT32 _mthMatchFuncFLOOR::getType()
   {
      return EN_MATCH_FUNC_FLOOR ;
   }

   const CHAR* _mthMatchFuncFLOOR::getName()
   {
      return MTH_FUNCTION_STR_FLOOR ;
   }

   //************************_mthMatchFuncLOWER********************************
   _mthMatchFuncLOWER::_mthMatchFuncLOWER( _mthNodeAllocator *allocator )
                      :_mthMatchFuncABS( allocator )
   {
   }

   _mthMatchFuncLOWER::~_mthMatchFuncLOWER()
   {
      clear() ;
   }

   INT32 _mthMatchFuncLOWER::call( const BSONElement &in, BSONObj &out )
   {
      INT32 rc = SDB_OK ;
      BSONObjBuilder builder ;

      rc = mthLower( _fieldName.getFieldName(), in, builder ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "mthCeiling failed:rc=%d", rc ) ;
      }

      out = builder.obj() ;

      return rc ;
   }

   INT32 _mthMatchFuncLOWER::getType()
   {
      return EN_MATCH_FUNC_LOWER ;
   }

   const CHAR* _mthMatchFuncLOWER::getName()
   {
      return MTH_FUNCTION_STR_LOWER ;
   }

   //************************_mthMatchFuncUPPER********************************
   _mthMatchFuncUPPER::_mthMatchFuncUPPER( _mthNodeAllocator *allocator )
                      :_mthMatchFuncABS( allocator )
   {
   }

   _mthMatchFuncUPPER::~_mthMatchFuncUPPER()
   {
      clear() ;
   }

   INT32 _mthMatchFuncUPPER::call( const BSONElement &in, BSONObj &out )
   {
      INT32 rc = SDB_OK ;
      BSONObjBuilder builder ;

      rc = mthUpper( _fieldName.getFieldName(), in, builder ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "mthUpper failed:rc=%d", rc ) ;
      }

      out = builder.obj() ;

      return rc ;
   }

   INT32 _mthMatchFuncUPPER::getType()
   {
      return EN_MATCH_FUNC_UPPER ;
   }

   const CHAR* _mthMatchFuncUPPER::getName()
   {
      return MTH_FUNCTION_STR_UPPER ;
   }

   //************************_mthMatchFuncLTRIM********************************
   _mthMatchFuncLTRIM::_mthMatchFuncLTRIM( _mthNodeAllocator *allocator )
                      :_mthMatchFuncABS( allocator )
   {
   }

   _mthMatchFuncLTRIM::~_mthMatchFuncLTRIM()
   {
      clear() ;
   }

   INT32 _mthMatchFuncLTRIM::call( const BSONElement &in, BSONObj &out )
   {
      INT32 rc = SDB_OK ;
      BSONObjBuilder builder ;

      rc = mthTrim( _fieldName.getFieldName(), in, -1, builder ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "mthTrim failed:rc=%d", rc ) ;
      }

      out = builder.obj() ;

      return rc ;
   }

   INT32 _mthMatchFuncLTRIM::getType()
   {
      return EN_MATCH_FUNC_LTRIM ;
   }

   const CHAR* _mthMatchFuncLTRIM::getName()
   {
      return MTH_FUNCTION_STR_LTRIM ;
   }

   //************************_mthMatchFuncRTRIM********************************
   _mthMatchFuncRTRIM::_mthMatchFuncRTRIM( _mthNodeAllocator *allocator )
                      :_mthMatchFuncABS( allocator )
   {
   }

   _mthMatchFuncRTRIM::~_mthMatchFuncRTRIM()
   {
      clear() ;
   }

   INT32 _mthMatchFuncRTRIM::call( const BSONElement &in, BSONObj &out )
   {
      INT32 rc = SDB_OK ;
      BSONObjBuilder builder ;

      rc = mthTrim( _fieldName.getFieldName(), in, 1, builder ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "mthTrim failed:rc=%d", rc ) ;
      }

      out = builder.obj() ;

      return rc ;
   }

   INT32 _mthMatchFuncRTRIM::getType()
   {
      return EN_MATCH_FUNC_RTRIM ;
   }

   const CHAR* _mthMatchFuncRTRIM::getName()
   {
      return MTH_FUNCTION_STR_RTRIM ;
   }

   //************************_mthMatchFuncTRIM********************************
   _mthMatchFuncTRIM::_mthMatchFuncTRIM( _mthNodeAllocator *allocator )
                     :_mthMatchFuncABS( allocator )
   {
   }

   _mthMatchFuncTRIM::~_mthMatchFuncTRIM()
   {
      clear() ;
   }

   INT32 _mthMatchFuncTRIM::call( const BSONElement &in, BSONObj &out )
   {
      INT32 rc = SDB_OK ;
      BSONObjBuilder builder ;

      rc = mthTrim( _fieldName.getFieldName(), in, 0, builder ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "mthTrim failed:rc=%d", rc ) ;
      }

      out = builder.obj() ;

      return rc ;
   }

   INT32 _mthMatchFuncTRIM::getType()
   {
      return EN_MATCH_FUNC_TRIM ;
   }

   const CHAR* _mthMatchFuncTRIM::getName()
   {
      return MTH_FUNCTION_STR_TRIM ;
   }

   //************************_mthMatchFuncSTRLEN********************************
   _mthMatchFuncSTRLEN::_mthMatchFuncSTRLEN( _mthNodeAllocator *allocator )
                       :_mthMatchFuncABS( allocator )
   {
   }

   _mthMatchFuncSTRLEN::~_mthMatchFuncSTRLEN()
   {
      clear() ;
   }

   INT32 _mthMatchFuncSTRLEN::call( const BSONElement &in, BSONObj &out )
   {
      INT32 rc = SDB_OK ;
      BSONObjBuilder builder ;

      rc = mthStrLen( _fieldName.getFieldName(), in, builder ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "mthStrLen failed:rc=%d", rc ) ;
      }

      out = builder.obj() ;

      return rc ;
   }

   INT32 _mthMatchFuncSTRLEN::getType()
   {
      return EN_MATCH_FUNC_STRLEN ;
   }

   const CHAR* _mthMatchFuncSTRLEN::getName()
   {
      return MTH_FUNCTION_STR_STRLEN ;
   }

   //************************_mthMatchFuncSUBSTR********************************
   _mthMatchFuncSUBSTR::_mthMatchFuncSUBSTR( _mthNodeAllocator *allocator )
                       :_mthMatchFunc( allocator )
   {
      _begin = 0 ;
      _limit = -1 ;
   }

   _mthMatchFuncSUBSTR::~_mthMatchFuncSUBSTR()
   {
      clear() ;
   }

   INT32 _mthMatchFuncSUBSTR::call( const BSONElement &in, BSONObj &out )
   {
      INT32 rc = SDB_OK ;
      BSONObjBuilder builder ;

      rc = mthSubStr( _fieldName.getFieldName(), in, _begin, _limit, builder ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "mthSubStr failed:rc=%d", rc ) ;
      }

      out = builder.obj() ;

      return rc ;
   }

   INT32 _mthMatchFuncSUBSTR::getType()
   {
      return EN_MATCH_FUNC_STRLEN ;
   }

   const CHAR* _mthMatchFuncSUBSTR::getName()
   {
      return MTH_FUNCTION_STR_SUBSTR ;
   }

   void _mthMatchFuncSUBSTR::clear()
   {
      _begin = 0 ;
      _limit = -1 ;

      _mthMatchFunc::clear() ;
   }

   INT32 _mthMatchFuncSUBSTR::_init( const CHAR *fieldName,
                                     const BSONElement &ele )
   {
      INT32 rc = SDB_OK ;
      if ( ele.isNumber() )
      {
         INT32 temp = ele.numberInt() ;
         if ( temp >= 0 )
         {
            _limit = temp ;
         }
         else
         {
            _begin = temp ;
         }
      }
      else if ( Array == ele.type() )
      {
         BSONObjIterator i( ele.embeddedObject() ) ;
         BSONElement subELe ;
         if ( !i.more() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "substr must have two element2 in array:ele=%s",
                    ele.toString().c_str() ) ;
            goto error ;
         }

         subELe = i.next() ;
         if ( !subELe.isNumber() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "substr element1 must be number:ele=%s",
                    ele.toString().c_str() ) ;
            goto error ;
         }

         _begin = subELe.numberInt() ;

         if ( !i.more() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "substr must have two element in array:ele=%s",
                    ele.toString().c_str() ) ;
            goto error ;
         }

         subELe = i.next() ;
         if ( !subELe.isNumber() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "substr element2 must be number:ele=%s",
                    ele.toString().c_str() ) ;
            goto error ;
         }

         _limit = subELe.numberInt() ;
         if ( !mthIsValidLen( _limit ) )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "limit is invalid:len=%d", _limit ) ;
            goto error ;
         }
      }
      else
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "substr's obj is invaid:ele=%s",
                 ele.toString().c_str() ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   //************************_mthMatchFuncMOD********************************
   _mthMatchFuncMOD::_mthMatchFuncMOD( _mthNodeAllocator *allocator )
                    :_mthMatchFunc( allocator )
   {
   }

   _mthMatchFuncMOD::~_mthMatchFuncMOD()
   {
      clear() ;
   }

   INT32 _mthMatchFuncMOD::call( const BSONElement &in, BSONObj &out )
   {
      INT32 rc = SDB_OK ;
      BSONObjBuilder builder ;

      rc = mthMod( _fieldName.getFieldName(), in, _funcEle, builder ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "mthMod failed:rc=%d", rc ) ;
      }

      out = builder.obj() ;

      return rc ;
   }

   INT32 _mthMatchFuncMOD::getType()
   {
      return EN_MATCH_FUNC_MOD ;
   }

   const CHAR* _mthMatchFuncMOD::getName()
   {
      return MTH_FUNCTION_STR_MOD ;
   }

   void _mthMatchFuncMOD::clear()
   {
      _mthMatchFunc::clear() ;
   }

   INT32 _mthMatchFuncMOD::_init( const CHAR *fieldName,
                                  const BSONElement &ele )
   {
      INT32 rc = SDB_OK ;
      if ( !ele.isNumber() || 0 == ele.Number() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "modm must be number, and can't be 0:ele=%s",
                 ele.toString().c_str() ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   //************************_mthMatchFuncADD********************************
   _mthMatchFuncADD::_mthMatchFuncADD( _mthNodeAllocator *allocator )
                    :_mthMatchFunc( allocator )
   {
   }

   _mthMatchFuncADD::~_mthMatchFuncADD()
   {
      clear() ;
   }

   INT32 _mthMatchFuncADD::call( const BSONElement &in, BSONObj &out )
   {
      INT32 rc = SDB_OK ;
      INT32 flag = 0 ;
      BSONObjBuilder builder ;

      rc = mthAdd( _fieldName.getFieldName(), in, _funcEle, builder, flag ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "mthAdd failed:rc=%d", rc ) ;
      }

      out = builder.obj() ;

      return rc ;
   }

   INT32 _mthMatchFuncADD::getType()
   {
      return EN_MATCH_FUNC_ADD ;
   }

   const CHAR* _mthMatchFuncADD::getName()
   {
      return MTH_FUNCTION_STR_ADD ;
   }

   void _mthMatchFuncADD::clear()
   {
      _mthMatchFunc::clear() ;
   }

   INT32 _mthMatchFuncADD::_init( const CHAR *fieldName,
                                  const BSONElement &ele )
   {
      INT32 rc = SDB_OK ;
      if ( !ele.isNumber() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "addend must be number:ele=%s",
                 ele.toString().c_str() ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   //************************_mthMatchFuncSUBTRACT********************************
   _mthMatchFuncSUBTRACT::_mthMatchFuncSUBTRACT( _mthNodeAllocator *allocator )
                         :_mthMatchFunc( allocator )
   {
   }

   _mthMatchFuncSUBTRACT::~_mthMatchFuncSUBTRACT()
   {
      clear() ;
   }

   INT32 _mthMatchFuncSUBTRACT::call( const BSONElement &in, BSONObj &out )
   {
      INT32 rc = SDB_OK ;
      INT32 flag = 0 ;
      BSONObjBuilder builder ;

      rc = mthSub( _fieldName.getFieldName(), in, _funcEle, builder, flag ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "mthSub failed:rc=%d", rc ) ;
      }

      out = builder.obj() ;

      return rc ;
   }

   INT32 _mthMatchFuncSUBTRACT::getType()
   {
      return EN_MATCH_FUNC_SUBTRACT ;
   }

   const CHAR* _mthMatchFuncSUBTRACT::getName()
   {
      return MTH_FUNCTION_STR_SUBTRACT ;
   }

   void _mthMatchFuncSUBTRACT::clear()
   {
      _mthMatchFunc::clear() ;
   }

   INT32 _mthMatchFuncSUBTRACT::_init( const CHAR *fieldName,
                                       const BSONElement &ele )
   {
      INT32 rc = SDB_OK ;
      if ( !ele.isNumber() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "subtrahead must be number:ele=%s",
                 ele.toString().c_str() ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   //************************_mthMatchFuncMULTIPLY********************************
   _mthMatchFuncMULTIPLY::_mthMatchFuncMULTIPLY( _mthNodeAllocator *allocator )
                         :_mthMatchFunc( allocator )
   {
   }

   _mthMatchFuncMULTIPLY::~_mthMatchFuncMULTIPLY()
   {
      clear() ;
   }

   INT32 _mthMatchFuncMULTIPLY::call( const BSONElement &in, BSONObj &out )
   {
      INT32 rc = SDB_OK ;
      INT32 flag = 0 ;
      BSONObjBuilder builder ;

      rc = mthMultiply( _fieldName.getFieldName(), in, _funcEle, builder, flag ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "mthMultiply failed:rc=%d", rc ) ;
      }

      out = builder.obj() ;

      return rc ;
   }

   INT32 _mthMatchFuncMULTIPLY::getType()
   {
      return EN_MATCH_FUNC_MULTIPLY ;
   }

   const CHAR* _mthMatchFuncMULTIPLY::getName()
   {
      return MTH_FUNCTION_STR_MULTIPLY ;
   }

   void _mthMatchFuncMULTIPLY::clear()
   {
      _mthMatchFunc::clear() ;
   }

   INT32 _mthMatchFuncMULTIPLY::_init( const CHAR *fieldName,
                                       const BSONElement &ele )
   {
      INT32 rc = SDB_OK ;
      if ( !ele.isNumber() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "multiplier must be number:ele=%s",
                 ele.toString().c_str() ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   //************************_mthMatchFuncDIVIDE********************************
   _mthMatchFuncDIVIDE::_mthMatchFuncDIVIDE( _mthNodeAllocator *allocator )
                       :_mthMatchFunc( allocator )
   {
   }

   _mthMatchFuncDIVIDE::~_mthMatchFuncDIVIDE()
   {
      clear() ;
   }

   INT32 _mthMatchFuncDIVIDE::call( const BSONElement &in, BSONObj &out )
   {
      INT32 rc = SDB_OK ;
      INT32 flag = 0 ;
      BSONObjBuilder builder ;

      rc = mthDivide( _fieldName.getFieldName(), in, _funcEle, builder, flag ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "mthDivide failed:rc=%d", rc ) ;
      }

      out = builder.obj() ;

      return rc ;
   }

   INT32 _mthMatchFuncDIVIDE::getType()
   {
      return EN_MATCH_FUNC_DIVIDE ;
   }

   const CHAR* _mthMatchFuncDIVIDE::getName()
   {
      return MTH_FUNCTION_STR_DIVIDE ;
   }

   void _mthMatchFuncDIVIDE::clear()
   {
      _mthMatchFunc::clear() ;
   }

   INT32 _mthMatchFuncDIVIDE::_init( const CHAR *fieldName,
                                     const BSONElement &ele )
   {
      INT32 rc = SDB_OK ;
      if ( !ele.isNumber() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "divisor must be number:ele=%s",
                 ele.toString().c_str() ) ;
         goto error ;
      }

      if ( mthIsZero( ele ) )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "divisor must not be zero:ele=%s",
                 ele.toString().c_str() ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   //************************_mthMatchFuncCAST********************************
   _mthMatchFuncCAST::_mthMatchFuncCAST( _mthNodeAllocator *allocator )
                     :_mthMatchFunc( allocator ), _castType( EOO )
   {
   }

   _mthMatchFuncCAST::~_mthMatchFuncCAST()
   {
      clear() ;
   }

   INT32 _mthMatchFuncCAST::call( const BSONElement &in, BSONObj &out )
   {
      INT32 rc = SDB_OK ;
      BSONObjBuilder builder ;

      rc = mthCast( _fieldName.getFieldName(), in, _castType, builder ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "mthDivide failed:rc=%d", rc ) ;
      }

      out = builder.obj() ;

      return rc ;
   }

   INT32 _mthMatchFuncCAST::getType()
   {
      return EN_MATCH_FUNC_CAST ;
   }

   const CHAR* _mthMatchFuncCAST::getName()
   {
      return MTH_FUNCTION_STR_CAST ;
   }

   void _mthMatchFuncCAST::clear()
   {
      _castType = EOO ;
      _mthMatchFunc::clear() ;
   }

   INT32 _mthMatchFuncCAST::_init( const CHAR *fieldName,
                                   const BSONElement &ele )
   {
      INT32 rc = SDB_OK ;

      if ( String == ele.type() )
      {
         rc = mthGetCastTranslator()->getCastType( ele.valuestr(), _castType ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "invalid cast type:%s", ele.valuestr() ) ;
            goto error ;
         }
      }
      else if ( !ele.isNumber() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "invalid cast type:%s", ele.toString().c_str() ) ;
         goto error ;
      }
      else
      {
         _castType = ( BSONType )( ele.numberInt() ) ;
      }

      switch( _castType )
      {
      case MinKey :
         break ;
      case EOO :
         rc = SDB_INVALIDARG ;
         break ;
      case NumberDouble :
      case String :
      case Object :
         break ;
      case Array :
      case BinData :
      case Undefined :
         rc = SDB_INVALIDARG ;
         break ;
      case jstOID :
      case Bool :
      case Date :
      case jstNULL :
         break ;
      case RegEx :
      case DBRef :
      case Code :
      case Symbol :
      case CodeWScope :
         rc = SDB_INVALIDARG ;
         break ;
      case NumberInt :
      case Timestamp :
      case NumberLong :
      case NumberDecimal :
      case MaxKey :
         break ;
      default:
         rc = SDB_INVALIDARG ;
         break ;
      }

      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "invalid cast type:type=%d", _castType ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   //************************_mthMatchFuncSLICE********************************
   _mthMatchFuncSLICE::_mthMatchFuncSLICE( _mthNodeAllocator *allocator )
                      :_mthMatchFunc( allocator )
   {
      _begin = 0 ;
      _limit = -1 ;
   }

   _mthMatchFuncSLICE::~_mthMatchFuncSLICE()
   {
      clear() ;
   }

   INT32 _mthMatchFuncSLICE::call( const BSONElement &in, BSONObj &out )
   {
      INT32 rc = SDB_OK ;
      BSONObjBuilder builder ;

      rc = mthSlice( _fieldName.getFieldName(), in, _begin, _limit, builder ) ;
      PD_RC_CHECK( rc, PDERROR, "mthSlice failed:rc=%d", rc ) ;

      out = builder.obj() ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _mthMatchFuncSLICE::getType()
   {
      return EN_MATCH_FUNC_SLICE ;
   }

   const CHAR* _mthMatchFuncSLICE::getName()
   {
      return MTH_FUNCTION_STR_SLICE ;
   }

   void _mthMatchFuncSLICE::clear()
   {
      _begin = 0 ;
      _limit = -1 ;

      _mthMatchFunc::clear() ;
   }

   INT32 _mthMatchFuncSLICE::_init( const CHAR *fieldName,
                                    const BSONElement &ele )
   {
      INT32 rc = SDB_OK ;
      if ( ele.isNumber() )
      {
         INT32 temp = ele.numberInt() ;
         if ( temp >= 0 )
         {
            _limit = temp ;
         }
         else
         {
            _begin = temp ;
         }
      }
      else if ( Array == ele.type() )
      {
         BSONObjIterator i( ele.embeddedObject() ) ;
         BSONElement subELe ;
         if ( !i.more() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "slice must have two element2 in array:ele=%s",
                    ele.toString().c_str() ) ;
            goto error ;
         }

         subELe = i.next() ;
         if ( !subELe.isNumber() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "slice element1 must be number:ele=%s",
                    ele.toString().c_str() ) ;
            goto error ;
         }

         _begin = subELe.numberInt() ;

         if ( !i.more() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "slice must have two element in array:ele=%s",
                    ele.toString().c_str() ) ;
            goto error ;
         }

         subELe = i.next() ;
         if ( !subELe.isNumber() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "slice element2 must be number:ele=%s",
                    ele.toString().c_str() ) ;
            goto error ;
         }

         _limit = subELe.numberInt() ;
         if ( !mthIsValidLen( _limit ) )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "limit is invalid:len=%d", _limit ) ;
            goto error ;
         }

      }
      else
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "slice's obj is invaid:ele=%s",
                 ele.toString().c_str() ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _mthMatchFuncSLICE::adjustIndexForReturnMatch( _utilArray< INT32 > &in,
                                                      _utilArray< INT32 > &out )
   {
      INT32 rc    = SDB_OK ;
      INT32 i     = 0 ;
      INT32 len   =  _limit ;
      INT32 start = _begin < 0 ? _begin + in.size() : _begin ;
      if ( start < 0 )
      {
         start = 0 ;
      }

      if ( len < 0 )
      {
         len = in.size() - start ;
      }

      if ( start >= ( INT32 )in.size() )
      {
         len = 0 ;
      }

      if ( start + len > ( INT32 )in.size() )
      {
         len = in.size() - start ;
      }

      for ( i = 0 ; i < len ; i++ )
      {
         rc = out.append( in[ start + i ] ) ;
         PD_RC_CHECK( rc, PDERROR, "append failed:rc=%d", rc ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   //**********************_mthMatchFuncSIZE******************************
   _mthMatchFuncSIZE::_mthMatchFuncSIZE( _mthNodeAllocator *allocator )
                     :_mthMatchFunc( allocator )
   {
   }

   _mthMatchFuncSIZE::~_mthMatchFuncSIZE()
   {
      clear() ;
   }

   INT32 _mthMatchFuncSIZE::call( const BSONElement &in, BSONObj &out )
   {
      INT32 rc = SDB_OK ;
      BSONObjBuilder builder ;

      rc = mthSize( _fieldName.getFieldName(), in, builder ) ;
      PD_RC_CHECK( rc, PDERROR, "mthSize failed:rc=%d" ) ;
      out = builder.obj() ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _mthMatchFuncSIZE::getType()
   {
      return EN_MATCH_FUNC_SIZE ;
   }

   const CHAR* _mthMatchFuncSIZE::getName()
   {
      return MTH_FUNCTION_STR_SIZE ;
   }

   void _mthMatchFuncSIZE::clear()
   {
      _mthMatchFunc::clear() ;
   }

   INT32 _mthMatchFuncSIZE::_init( const CHAR *fieldName,
                                   const BSONElement &ele )
   {
      if ( !ele.isNumber() || ele.numberInt() != 1 )
      {
         return SDB_INVALIDARG ;
      }

      return SDB_OK ;
   }


   //**********************_mthMatchFuncTYPE******************************
      _mthMatchFuncTYPE::_mthMatchFuncTYPE( _mthNodeAllocator *allocator )
                        :_mthMatchFunc( allocator ), _resultType( -1 )
      {
      }

      _mthMatchFuncTYPE::~_mthMatchFuncTYPE()
      {
         clear() ;
      }

      INT32 _mthMatchFuncTYPE::call( const BSONElement &in, BSONObj &out )
      {
         INT32 rc = SDB_OK ;
         BSONObjBuilder builder ;

         rc = mthType( _fieldName.getFieldName(), _resultType, in, builder ) ;
         PD_RC_CHECK( rc, PDERROR, "mthType failed:rc=%d" ) ;
         out = builder.obj() ;

      done:
         return rc ;
      error:
         goto done ;
      }

      INT32 _mthMatchFuncTYPE::getType()
      {
         return EN_MATCH_FUNC_TYPE ;
      }

      const CHAR* _mthMatchFuncTYPE::getName()
      {
         return MTH_FUNCTION_STR_TYPE ;
      }

      void _mthMatchFuncTYPE::clear()
      {
         _resultType = -1 ;
         _mthMatchFunc::clear() ;
      }

      INT32 _mthMatchFuncTYPE::_init( const CHAR *fieldName,
                                      const BSONElement &ele )
      {
         if ( !ele.isNumber() )
         {
            return SDB_INVALIDARG ;
         }

         _resultType = ele.numberInt() ;
         if ( 1 != _resultType && 2 != _resultType )
         {
            return SDB_INVALIDARG ;
         }

         return SDB_OK ;
      }

   //************************_mthMatchFuncRETURNMATCH********************************
   _mthMatchFuncRETURNMATCH::_mthMatchFuncRETURNMATCH( _mthNodeAllocator *allocator )
                            :_mthMatchFunc( allocator )
   {
      _offset = 0 ;
      _len = -1 ;
   }

   _mthMatchFuncRETURNMATCH::~_mthMatchFuncRETURNMATCH()
   {
      clear() ;
   }

   INT32 _mthMatchFuncRETURNMATCH::getOffset()
   {
      return _offset ;
   }

   INT32 _mthMatchFuncRETURNMATCH::getLen()
   {
      return _len ;
   }

   INT32 _mthMatchFuncRETURNMATCH::call( const BSONElement &in, BSONObj &out )
   {
      SDB_ASSERT( FALSE, "impossible" ) ;
      return SDB_INVALIDARG ;
   }

   INT32 _mthMatchFuncRETURNMATCH::getType()
   {
      return EN_MATCH_ATTR_RETURNMATCH ;
   }

   const CHAR* _mthMatchFuncRETURNMATCH::getName()
   {
      return MTH_ATTR_STR_RETURNMATCH ;
   }

   void _mthMatchFuncRETURNMATCH::clear()
   {
      _mthMatchFunc::clear() ;
   }

   INT32 _mthMatchFuncRETURNMATCH::_init( const CHAR *fieldName,
                                          const BSONElement &ele )
   {
      INT32 rc = SDB_OK ;
      if ( ele.type() == NumberInt )
      {
         _offset = ele.numberInt() ;
         _len    = -1 ;
      }
      else if ( ele.type() == Array )
      {
         BSONObj obj = ele.embeddedObject() ;
         if ( obj.nFields() != 2 )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "attr must have two elements:attr=%s,rc=%d",
                    ele.toString().c_str(), rc ) ;
            goto error ;
         }

         {
            BSONObjIterator iter( obj ) ;
            BSONElement tmpEle ;

            tmpEle = iter.next() ;
            if ( !tmpEle.isNumber() )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG( PDERROR, "arg1 must be a number:arg1=%s,rc=%d",
                       tmpEle.toString().c_str(), rc ) ;
               goto error ;
            }

            _offset = tmpEle.numberInt() ;
            tmpEle = iter.next() ;
            if ( !tmpEle.isNumber() )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG( PDERROR, "arg2 must be a number:arg2=%s,rc=%d",
                       tmpEle.toString().c_str(), rc ) ;
               goto error ;
            }

            _len = tmpEle.numberInt() ;
         }
      }
      else
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "set attr failed:attr=%s,rc=%d",
                 ele.toString().c_str(), rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   //************************_mthMatchFuncEXPAND********************************
   _mthMatchFuncEXPAND::_mthMatchFuncEXPAND( _mthNodeAllocator *allocator )
                       :_mthMatchFunc( allocator )
   {
   }

   _mthMatchFuncEXPAND::~_mthMatchFuncEXPAND()
   {
      clear() ;
   }

   INT32 _mthMatchFuncEXPAND::call( const BSONElement &in, BSONObj &out )
   {
      SDB_ASSERT( FALSE, "impossible" ) ;
      return SDB_INVALIDARG ;
   }

   INT32 _mthMatchFuncEXPAND::getType()
   {
      return EN_MATCH_ATTR_EXPAND ;
   }

   const CHAR* _mthMatchFuncEXPAND::getName()
   {
      return MTH_ATTR_STR_EXPAND ;
   }

   const CHAR* _mthMatchFuncEXPAND::getFieldName()
   {
      return _fieldName.getFieldName() ;
   }

   void _mthMatchFuncEXPAND::getElement( BSONElement &ele )
   {
      ele = _funcEle ;
   }

   void _mthMatchFuncEXPAND::clear()
   {
      _mthMatchFunc::clear() ;
   }

   INT32 _mthMatchFuncEXPAND::_init( const CHAR *fieldName,
                                     const BSONElement &ele )
   {
      INT32 rc = SDB_OK ;
      if ( ele.type() != NumberInt || ele.numberInt() != 1 )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "set attr failed:attr=%s,rc=%d",
                 ele.toString().c_str(), rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   //************************_mthMatchOpNode********************************
   _mthMatchOpNode::_mthMatchOpNode( _mthNodeAllocator *allocator,
                                     const mthNodeConfig *config )
                   :_mthMatchNode( allocator, config )
   {
      _isCompareField     = FALSE ;
      _hasDollarFieldName = FALSE ;
      _cmpFieldName       = NULL ;
      _hasReturnMatch     = FALSE ;
      _hasExpand          = FALSE ;
      _offset             = 0 ;
      _len                = 0 ;
      _paramIndex         = -1 ;
      _addedToPred        = FALSE ;
      _doneByPred         = FALSE ;
   }

   _mthMatchOpNode::~_mthMatchOpNode()
   {
      clear() ;
   }

   INT32 _mthMatchOpNode::init( const CHAR *fieldName,
                                const BSONElement &element )
   {
      INT32 rc = SDB_OK ;
      const CHAR *name = NULL ;
      rc = _mthMatchNode::init( fieldName, element ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "set fieldName failed:fieldName=%s,rc=%d",
                 fieldName, rc ) ;
         goto error ;
      }

      if ( NULL != ossStrstr( fieldName, ".$" ) )
      {
         _hasDollarFieldName = TRUE ;
      }

      _toMatch = element ;
      name     = element.fieldName() ;
      if ( ossStrcmp( name, MTH_OPERATOR_STR_FIELD ) == 0 )
      {
         if ( element.type() != String )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "$field must be String type:ele=%s,type=%d,rc=%d",
                    element.toString().c_str(), element.type(), rc ) ;
            goto error ;
         }

         _isCompareField = TRUE ;
         _cmpFieldName   = element.valuestr() ;
      }

      rc = _init( fieldName, element ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "_init failed:rc=%d", rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      clear() ;
      goto done ;
   }

   INT32 _mthMatchOpNode::_init( const CHAR *fieldName,
                                 const BSONElement &element )
   {
      return SDB_OK ;
   }

   void _mthMatchOpNode::_clear()
   {
      return ;
   }

   UINT32 _mthMatchOpNode::_evalFuncCPUCost () const
   {
      if ( _funcList.empty() )
      {
         return 0 ;
      }

      UINT32 cpuCost = 0 ;
      MTH_FUNC_LIST::const_iterator iter = _funcList.begin() ;
      while ( iter != _funcList.end() )
      {
         _mthMatchFunc *func = *iter ;
         cpuCost += func->getEvalCPUCost() ;
         iter++ ;
      }
      return cpuCost ;
   }

   void _mthMatchOpNode::clear()
   {
      _clear() ;

      MTH_FUNC_LIST::iterator iter = _funcList.begin() ;
      while ( iter != _funcList.end() )
      {
         _mthMatchFunc *func = *iter ;
         mthGetMatchNodeFactory()->releaseFunc( func ) ;
         iter++ ;
      }
      _funcList.clear() ;
      _hasDollarFieldName = FALSE ;
      _isCompareField     = FALSE ;
      _cmpFieldName       = NULL ;
      _hasReturnMatch     = FALSE ;
      _hasExpand          = FALSE ;
      _offset             = 0 ;
      _len                = 0 ;

      _mthMatchNode::clear() ;
   }

   void _mthMatchOpNode::setWeight( UINT32 weight )
   {
      SDB_ASSERT( FALSE, "no need to setWeight in _mthMatchOpNode" ) ;
   }

   void _mthMatchOpNode::evalEstimation ( const optCollectionStat *pCollectionStat,
                                          double &selectivity,
                                          UINT32 &cpuCost )
   {

      if ( isAddedToPred() )
      {
         _evalEstimation( NULL, selectivity, cpuCost ) ;
         // Already in the predicates, no need to be calculated
         selectivity = 1.0 ;
      }
      else
      {
         _evalEstimation( pCollectionStat, selectivity, cpuCost ) ;
      }
   }

   void _mthMatchOpNode::_evalEstimation ( const optCollectionStat *pCollectionStat,
                                           double &selectivity,
                                           UINT32 &cpuCost )
   {
      // Simply estimate
      selectivity = OPT_MTH_OPTR_DEFAULT_SELECTIVITY ;
      cpuCost = _evalCPUCost() ;
   }

   INT32 _mthMatchOpNode::calcPredicate( rtnPredicateSet &predicateSet,
                                         const rtnParamList * paramList )
   {
      INT32 rc = SDB_OK ;
      const UINT32 bufLen        = 31 ;
      CHAR staticBuf[ bufLen+1 ] = { 0 } ;
      CHAR *buf                  = staticBuf ;
      BOOLEAN rebuildName        = FALSE ;
      const CHAR *fieldName      = NULL ;

      if ( _isCompareField || _funcList.size() > 0 )
      {
         // $field or functions do not have predicate
         goto done ;
      }

      fieldName = ( CHAR * ) _fieldName.getFieldName() ;

      if ( NULL != ossStrstr( fieldName, ".$" ) )
      {
         UINT32 pos      = 0 ;
         const CHAR *p   = fieldName ;
         BOOLEAN ignored = FALSE ;
         rebuildName     = TRUE ;

         while ( '\0' != *p )
         {
            if ( !ignored )
            {
               if ( '$' == *p &&
                    0 < ( p - fieldName ) &&
                    '.' == *( p - 1 ) )
               {
                  ignored = TRUE ;
                  --pos ;
               }
               else
               {
                  buf[pos++] = *p ;
               }
            }
            else if ( '.' == *p )
            {
               ignored = FALSE ;
               buf[pos++] = *p ;
            }
            else
            {
               /// do nothing
            }

            ++p ;
            if ( bufLen == pos && buf == staticBuf )
            {
               UINT32 allocLen = ossStrlen( fieldName ) + 1 ;
               buf = ( CHAR * )SDB_THREAD_ALLOC( allocLen ) ;
               if ( NULL == buf )
               {
                  PD_LOG( PDERROR, "failed to allocate mem." ) ;
                  rc = SDB_OOM ;
                  goto error ;
               }
               ossMemcpy( buf, staticBuf, pos ) ;
            }
         }

         buf[pos] = '\0' ;
      }

      PD_LOG( PDDEBUG, "add preicate[%s] to predicates set",
              rebuildName ? buf : fieldName ) ;

      if ( SDB_OK == _addPredicate ( predicateSet,
                                     rebuildName ? buf : fieldName,
                                     paramList ) &&
           isTotalConverted() )
      {
         _addedToPred = TRUE ;
      }

   done:
      if ( buf != staticBuf && NULL != buf )
      {
         SDB_THREAD_FREE( buf ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 _mthMatchOpNode::extraEqualityMatches( BSONObjBuilder &builder,
                                                const rtnParamList *parameters )
   {
      //only $all and $et have EqualityMatches
      return SDB_OK ;
   }

   BOOLEAN _mthMatchOpNode::_isNot()
   {
      if ( _isUnderLogicNot )
      {
         if ( getType() == EN_MATCH_OPERATOR_NE ||
              getType() == EN_MATCH_OPERATOR_NIN )
         {
            return FALSE ;
         }
         else
         {
            return TRUE ;
         }
      }
      else
      {
         if ( getType() == EN_MATCH_OPERATOR_NE ||
              getType() == EN_MATCH_OPERATOR_NIN )
         {
            return TRUE ;
         }
         else
         {
            return FALSE ;
         }
      }
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHMATCHOPNODE_DOLLARMATCHES, "_mthMatchOpNode::_dollarMatches" )
   INT32 _mthMatchOpNode::_dollarMatches( const CHAR *pFieldName,
                                          const BSONElement &element,
                                          _mthMatchTreeContext &context,
                                          BOOLEAN &result,
                                          BOOLEAN &gotUndefined )
   {
      PD_TRACE_ENTRY( SDB__MTHMATCHOPNODE_DOLLARMATCHES ) ;
      INT32 rc = SDB_OK ;
      const CHAR *p = pFieldName ;
      const CHAR *childName = NULL ;
      INT32 dollarValue = 0 ;

      SDB_ASSERT( NULL != pFieldName &&
                  MTH_OPERATOR_EYECATCHER == *p, "impossible" ) ;

      rc = ossStrToInt ( p + 1, &dollarValue ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG ( PDERROR, "Failed to parse number:p=%s,rc=%d", p, rc ) ;
         goto error ;
      }

      if ( Array != element.type() || ossStrlen( pFieldName ) <= 1 )
      {
         result = FALSE ;
         goto done ;
      }

      childName = ossStrchr( p, MTH_FIELDNAME_SEP ) ;
      {
         BSONObjIterator i( element.embeddedObject() ) ;
         while ( i.more() )
         {
            BSONElement e = i.next() ;
            if ( NULL != childName )
            {
               //a.$0.xxx
               if ( MTH_OPERATOR_EYECATCHER == *( childName + 1 ) &&
                    Array == e.type() )
               {
                  // a.$0.$1, now childName is .$1
                  rc = _dollarMatches( childName + 1, e, context, result,
                                       gotUndefined ) ;
                  if ( SDB_OK != rc )
                  {
                     PD_LOG( PDERROR, "failed to child field name:%s, rc:%d",
                             childName, rc ) ;
                     goto error ;
                  }
               }
               else if ( Object == e.type() )
               {
                  // a.$0.b, now childName is .b
                  rc = _execute( childName + 1, e.embeddedObject(), FALSE,
                                 context, result, gotUndefined ) ;
                  if ( SDB_OK != rc )
                  {
                     PD_LOG( PDERROR, "_execute failed:childName=%s,rc:%d",
                             childName, rc ) ;
                     goto error ;
                  }
               }
               else
               {
                  result = FALSE ;
               }
            }
            else
            {
               BSONElement right = _toMatch ;
               if ( _isCompareField )
               {
                  right = context._originalObj.getFieldDotted( _cmpFieldName ) ;
               }
               rc = _doFuncMatch( e, right, context, mthEnabledMixCmp(),
                                  result ) ;
               if ( SDB_OK != rc )
               {
                  PD_LOG( PDERROR, "_doFuncMatch failed:rc=%d", rc ) ;
                  goto error ;
               }
            }

            if ( context.isDollarListEnabled() )
            {
               if ( ( !_isNot() && result ) || ( _isNot() && !result ) )
               {
                  INT64 temp       = 0 ;
                  INT32 dollarNum2 = ossAtoi( e.fieldName() ) ;
                  temp = ( ( (INT64) dollarValue ) << 32 ) |
                         ( ( (INT64) dollarNum2 ) & 0xFFFFFFFF ) ;
                  context._dollarList.push_back( temp ) ;
               }
            }

            if ( result )
            {
               goto done ;
            }
         }
      }

      result = FALSE ;
   done:
      PD_TRACE_EXITRC( SDB__MTHMATCHOPNODE_DOLLARMATCHES, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _mthMatchOpNode::_doFuncMatch( const BSONElement &original,
                                        const BSONElement &matchTarget,
                                        _mthMatchTreeContext &context,
                                        BOOLEAN mixCmp,
                                        BOOLEAN &matchResult )
   {
      INT32 rc = SDB_OK ;
      BSONObj resultObj ;
      BSONElement resultEle = original ;

      if ( _funcList.size() > 0 )
      {
         rc = _calculateFuncs( original, resultObj ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "_calculateFuncs failed:rc=%d", rc ) ;
            goto error ;
         }

         resultEle = resultObj.firstElement() ;

         // Expect the same canonical type of inputs
         mixCmp = FALSE ;
      }

      rc = _valueMatch( resultEle, matchTarget, mixCmp, context, matchResult ) ;
      PD_RC_CHECK( rc, PDERROR, "_valueMatch failed:rc=%d", rc ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _mthMatchOpNode::_calculateFuncs( const BSONElement &in,
                                           BSONObj &out )
   {
      INT32 rc = SDB_OK ;
      MTH_FUNC_LIST::iterator iter ;
      BSONElement tmpIn ;
      BSONObj savedObj ;

      tmpIn = in ;
      iter  = _funcList.begin() ;
      while( iter != _funcList.end() )
      {
         BSONObj tmpObj ;
         _mthMatchFunc *func = *iter ;
         rc = func->call( tmpIn, tmpObj ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "call func failed:func=%s,rc=%d",
                    func->toString().c_str(), rc ) ;
            goto error ;
         }

         savedObj = tmpObj ;
         tmpIn    = savedObj.firstElement() ;
         iter++ ;
      }

      out = savedObj ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _mthMatchOpNode::_saveElement( _mthMatchTreeContext &context,
                                        BOOLEAN isMatch, INT32 index )
   {
      INT32 rc = SDB_OK ;
      if ( !_hasReturnMatch )
      {
         goto done ;
      }

      context.setIsUseElement( TRUE ) ;

      if ( ( isMatch && !isUnderLogicNot() ) ||
           ( !isMatch && isUnderLogicNot() ) )
      {
         rc = context.saveElement( index ) ;
         PD_RC_CHECK( rc, PDERROR, "save element failed:index=%d,rc=%d",
                      index, rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHMATCHOPNODE__EXECUTE, "_mthMatchOpNode::_execute" )
   INT32 _mthMatchOpNode::_execute( const CHAR *pFieldName,
                                    const BSONObj &obj, BOOLEAN isArrayObj,
                                    _mthMatchTreeContext &context,
                                    BOOLEAN &result,
                                    BOOLEAN &gotUndefined )
   {
      PD_TRACE_ENTRY( SDB__MTHMATCHOPNODE__EXECUTE ) ;
      INT32 rc = SDB_OK ;
      CHAR *pTmpFieldName = NULL ;
      _mthMatchFieldName<> mthFieldName ;
      BSONObj funcResultObj ;
      BSONElement recordEle ;
      BSONElement toMatchEle ;
      CHAR *p  = NULL ;

      BOOLEAN mixCmp = mthEnabledMixCmp() ;

      gotUndefined = FALSE ;

      if ( _paramIndex != -1 )
      {
         if ( _doneByPred )
         {
            // Already calculated by predicates against index
            result = TRUE ;
            goto done ;
         }
         else if ( context.paramDoneByPred( _paramIndex ) )
         {
            // Already calculated by predicates against index
            result = TRUE ;
            _doneByPred = TRUE ;
            goto done ;
         }
         // Bind parameter
         toMatchEle = context.getParameter( _paramIndex ) ;
      }
      else
      {
         toMatchEle = _toMatch ;
      }

      rc = mthFieldName.setFieldName( pFieldName ) ;
      PD_RC_CHECK( rc, PDERROR, "set fieldName failed:fieldName=%s,rc=%d",
                   pFieldName, rc ) ;

      pTmpFieldName = ( CHAR * ) mthFieldName.getFieldName() ;
      p = ossStrchr ( pTmpFieldName, MTH_FIELDNAME_SEP ) ;
      if ( p )
      {
         //xxx.xxx.xxx
         *p = '\0' ;
         BSONElement ele = obj.getField( pTmpFieldName ) ;
         *p = '.' ;
         if ( ele.type() == Object || ele.type() == Array )
         {
            //xxx.$1.xxx
            if ( MTH_OPERATOR_EYECATCHER == *(p + 1) )
            {
               rc = _dollarMatches( p + 1, ele, context, result,
                                    gotUndefined ) ;
               PD_RC_CHECK( rc, PDERROR, "_dollarMatches failed:rc=%d", rc ) ;
            }
            else
            {
               // xxx.xxx.xxx
               BSONObj subObj = ele.embeddedObject () ;
               // obj : { "a" : [ { "b" : 1 }, { "c" : 2 } ] }
               // ele.type() == Array:
               //   subObj: { 0 : { "b" : 1 }, 1 : { "c" : 2 } }
               rc = _execute( p + 1, subObj, ( ele.type() == Array ), context,
                              result, gotUndefined ) ;
               PD_RC_CHECK( rc, PDERROR, "failed to match child field:rc=%d",
                            rc ) ;
            }

            goto done ;
         }
      }

      if ( isArrayObj )
      {
         // obj: { 0 : { "b" : 1 }, 1 : { "c" : 2 } }
         BSONObjIterator it ( obj ) ;
         BOOLEAN tmpUndefined = TRUE ;
         result = FALSE ;
         while ( it.more() )
         {
            BOOLEAN subUndefined = TRUE ;
            BSONElement z = it.next() ;
            if ( ossStrcmp( z.fieldName(), pTmpFieldName ) == 0 )
            {
               subUndefined = FALSE ;
               // Inside an array, mix-compare mode should be disabled
               rc = _doFuncMatch( z, toMatchEle, context, FALSE, result ) ;
               PD_RC_CHECK( rc, PDERROR, "_doFuncMatch failed:rc=%d", rc ) ;

               if ( result )
               {
                  goto done ;
               }
            }

            if ( z.type() == Object )
            {
               BSONObj subObj = z.embeddedObject() ;
               //pass the input pFieldName, not pTmpFieldName
               rc = _execute( pFieldName, subObj, FALSE, context, result,
                              subUndefined ) ;
               PD_RC_CHECK( rc, PDERROR, "_execute failed:rc=%d", rc ) ;

               if ( result )
               {
                  goto done ;
               }
            }
            tmpUndefined = tmpUndefined && subUndefined ;
         }
         // Report undefined only when all sub-elements got undefined
         // Note: empty array in this case is undefined
         gotUndefined = tmpUndefined ;

         if ( gotUndefined && _flagAcceptUndefined() )
         {
            rc = _execute( pFieldName, BSONObj(), FALSE, context, result,
                           tmpUndefined ) ;
            PD_RC_CHECK( rc, PDERROR, "_execute failed:rc=%d", rc ) ;
         }

         goto done ;
      }

      if ( p && !_flagAcceptUndefined() )
      {
         gotUndefined = TRUE ;
         result = FALSE ;
         goto done ;
      }

      if ( _isCompareField )
      {
         toMatchEle = context._originalObj.getFieldDotted( _cmpFieldName ) ;
         if ( toMatchEle.eoo() )
         {
            gotUndefined = TRUE ;
            result = FALSE ;
            goto done ;
         }
      }

      recordEle = obj.getField( pTmpFieldName ) ;
      if ( _funcList.size() > 0 )
      {
         rc = _calculateFuncs( recordEle, funcResultObj ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "_calculateFuncs failed:rc=%d", rc ) ;
            goto error ;
         }

         recordEle = funcResultObj.firstElement() ;

         // Expect the same canonical type of inputs
         mixCmp = FALSE ;
      }

      if ( recordEle.eoo() && !_flagAcceptUndefined() )
      {
         gotUndefined = TRUE ;
         result = FALSE ;
         goto done ;
      }

      rc = _valueMatch( recordEle, toMatchEle, mixCmp, context, result ) ;
      PD_RC_CHECK( rc, PDERROR, "_valueMatch failed:rc=%d", rc ) ;

      if ( EN_MATCH_OPERATOR_EXISTS == getType() ||
           EN_MATCH_OPERATOR_ISNULL == getType() ||
           EN_MATCH_OPERATOR_IN == getType() ||
           EN_MATCH_OPERATOR_NIN == getType() ||
           EN_MATCH_OPERATOR_ALL == getType() )
      {
         // no need to check if left is array
         goto done ;
      }

      if ( !result )
      {
         //if no match. try iterator and check the array items
         if ( Array == recordEle.type() )
         {
            BSONObj eEmbObj = recordEle.embeddedObject() ;
            BSONObjIterator iter( eEmbObj ) ;
            INT32 index = 0 ;
            // If parameter is a array, we only match array elements inside
            BOOLEAN innerMixCmp = toMatchEle.type() == Array ? FALSE : mixCmp ;
            while ( iter.more() )
            {
               BOOLEAN tmpResult = FALSE ;
               BSONElement innerEle = iter.next() ;

               rc = _valueMatch( innerEle, toMatchEle, innerMixCmp, context,
                                 tmpResult ) ;
               PD_RC_CHECK( rc, PDERROR, "_valueMatch failed:rc=%d", rc ) ;
               if ( EN_MATCH_OPERATOR_NE == getType() )
               {
                  rc = _saveElement( context, !tmpResult, index ) ;
               }
               else
               {
                  rc = _saveElement( context, tmpResult, index ) ;
               }
               PD_RC_CHECK( rc, PDERROR, "_saveElement failed:rc=%d", rc ) ;

               if ( tmpResult )
               {
                  result = tmpResult ;
                  if ( !_hasReturnMatch )
                  {
                     goto done ;
                  }
               }

               index++ ;
            }

            if ( _hasReturnMatch )
            {
               rc = context.subElements( _offset, _len ) ;
               PD_RC_CHECK( rc, PDERROR, "subElements failed:rc=%d", rc ) ;
            }
         }
      }
   done:
      PD_TRACE_EXITRC( SDB__MTHMATCHOPNODE__EXECUTE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHMATCHOPNODE_EXECUTE, "_mthMatchOpNode::execute" )
   INT32 _mthMatchOpNode::execute( const BSONObj &obj,
                                   _mthMatchTreeContext &context,
                                   BOOLEAN &result )
   {
      PD_TRACE_ENTRY( SDB__MTHMATCHOPNODE_EXECUTE ) ;

      INT32 rc = SDB_OK ;
      BOOLEAN gotUndefined = FALSE ;

      if ( _hasReturnMatch )
      {
         context.setReturnMatchExecuted( TRUE ) ;
      }

      rc = _execute( _fieldName.getFieldName(), obj, FALSE, context, result,
                     gotUndefined ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "_execute failed:rc=%d", rc ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__MTHMATCHOPNODE_EXECUTE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   BOOLEAN _mthMatchOpNode::hasDollarFieldName()
   {
      return _hasDollarFieldName ;
   }

   BOOLEAN _mthMatchOpNode::isTotalConverted()
   {
      if ( _isCompareField || _funcList.size() > 0 )
      {
         return FALSE ;
      }

      return TRUE ;
   }

   INT32 _mthMatchOpNode::addChild( _mthMatchNode *child )
   {
      SDB_ASSERT( FALSE, "_mthMatchOpNode can't have child" ) ;
      return SDB_INVALIDARG ;
   }

   void _mthMatchOpNode::delChild( _mthMatchNode *child )
   {
      SDB_ASSERT( FALSE, "_mthMatchOpNode can't have child" ) ;
   }

   void _mthMatchOpNode::setDoneByPred ( BOOLEAN doneByPred )
   {
      if ( doneByPred && isAddedToPred() )
      {
         _doneByPred = TRUE ;
      }
      else
      {
         _doneByPred = FALSE ;
      }
   }

   INT32 _mthMatchOpNode::addFunc( _mthMatchFunc *func )
   {
      INT32 rc = SDB_OK ;
      SDB_ASSERT( NULL != func, "func can't be null!" ) ;
       try
      {
         _funcList.push_back( func ) ;
      }
      catch( std::exception & )
      {
         rc = SDB_OOM;
         PD_LOG( PDERROR, "add function failed:rc=%d", rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   // functions will be removed from funcList
   INT32 _mthMatchOpNode::addFuncList( MTH_FUNC_LIST &funcList )
   {
      INT32 rc = SDB_OK ;

      while ( TRUE )
      {
         MTH_FUNC_LIST::iterator iter = funcList.begin() ;
         if ( iter == funcList.end() )
         {
            break ;
         }

         _mthMatchFunc *func = *iter ;
         if ( func->getType() == EN_MATCH_ATTR_RETURNMATCH )
         {
            _mthMatchFuncRETURNMATCH *rmFunc = NULL ;
            rmFunc = dynamic_cast< _mthMatchFuncRETURNMATCH * > ( func ) ;
            if ( NULL == rmFunc )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG( PDERROR, "dynamic_cast(func->RETURNMATCH) failed:"
                          "func=%s,rc=%d", func->toString().c_str(), rc ) ;
               goto error ;
            }

            SDB_ASSERT( FALSE == _hasReturnMatch, "only once" ) ;
            _hasReturnMatch = TRUE ;
            _offset         = rmFunc->getOffset() ;
            _len            = rmFunc->getLen() ;
            mthGetMatchNodeFactory()->releaseFunc( rmFunc ) ;
         }
         else if ( func->getType() == EN_MATCH_ATTR_EXPAND )
         {
            _mthMatchFuncEXPAND *expandFunc = NULL ;
            expandFunc = dynamic_cast< _mthMatchFuncEXPAND * > ( func ) ;
            if ( NULL == expandFunc )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG( PDERROR, "dynamic_cast(func->EXPAND) failed:"
                          "func=%s,rc=%d", func->toString().c_str(), rc ) ;
               goto error ;
            }

            SDB_ASSERT( FALSE == _hasExpand, "only once" ) ;
            _hasExpand = TRUE ;
            mthGetMatchNodeFactory()->releaseFunc( expandFunc ) ;
         }
         else
         {
            rc = addFunc( func ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "add function failed:func=%s,rc=%d",
                       func->toString().c_str(), rc ) ;
               goto error ;
            }
         }

         funcList.erase( iter ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   void _mthMatchOpNode::getFuncList( MTH_FUNC_LIST &funcList )
   {
      MTH_FUNC_LIST::iterator iter = _funcList.begin() ;
      while ( iter != _funcList.end() )
      {
         funcList.push_back( *iter ) ;
         iter++ ;
      }
   }

   BOOLEAN _mthMatchOpNode::hasReturnMatch()
   {
      return _hasReturnMatch ;
   }

   BSONObj _mthMatchOpNode::_toBson ( const rtnParamList &parameters )
   {
      BSONObjBuilder builder ;
      MTH_FUNC_LIST::iterator iter ;
      BSONObjBuilder b( builder.subobjStart( _fieldName.getFieldName() ) ) ;

      if ( _hasExpand )
      {
         b.append( MTH_ATTR_STR_EXPAND, 1 ) ;
      }

      if ( _hasReturnMatch )
      {
         BSONObj tmp = BSON( "0" << _offset << "1" << _len ) ;
         b.appendArray( MTH_ATTR_STR_RETURNMATCH, tmp ) ;
      }

      iter = _funcList.begin() ;
      while ( iter != _funcList.end() )
      {
         _mthMatchFunc *func = *iter ;
         BSONObj obj = func->toBson() ;
         BSONElement ele = obj.firstElement() ;
         if ( ele.type() == Object || ele.type() == Array )
         {
            b.appendElements( ele.embeddedObject() ) ;
         }

         iter++ ;
      }

      if ( _isCompareField )
      {
         BSONObj fieldObj = BSON( MTH_OPERATOR_STR_FIELD <<
                                  _toMatch.valuestrsafe() ) ;
         b.append( getOperatorStr(), fieldObj ) ;
      }
      else if ( _paramIndex != -1 )
      {
         _toParamBson( b, parameters ) ;
      }
      else
      {
         b.appendAs( _toMatch, getOperatorStr() ) ;
      }

      b.doneFast() ;

      return builder.obj() ;
   }

   void _mthMatchOpNode::_toParamBson ( BSONObjBuilder &builder,
                                        const rtnParamList &parameters )
   {
      SDB_ASSERT( -1 != _paramIndex, "_paramIndex is invalid" ) ;

      if ( parameters.isEmpty() )
      {
         // Generate $param fields { $param : x, $ctype : y }
         BSONObjBuilder paramBuilder( builder.subobjStart( getOperatorStr() ) ) ;
         if ( -1 == _getFuzzyIndex() )
         {
            paramBuilder.append( FIELD_NAME_PARAM, (INT32)_paramIndex ) ;
         }
         else
         {
            // Generate $param field with fuzzy operator
            // $param : [ paramIndex, fuzzyIndex ]
            BSONArrayBuilder paramArrBuilder(
                  paramBuilder.subarrayStart( FIELD_NAME_PARAM ) ) ;
            paramArrBuilder.append( (INT32)_paramIndex ) ;
            paramArrBuilder.append( (INT32)_getFuzzyIndex() ) ;
            paramArrBuilder.done() ;
         }
         if ( mthEnabledMixCmp() )
         {
            paramBuilder.append( FIELD_NAME_CTYPE,
                                 (INT32)_toMatch.canonicalType() ) ;
         }
         paramBuilder.done() ;
      }
      else
      {
         // Bind the parameters into output BSON
         builder.appendAs( parameters.getParam( _paramIndex ),
                           getOperatorStr() ) ;
      }
   }

   INT32 _mthMatchOpNode::_addPredicate ( rtnPredicateSet & predicateSet,
                                          const CHAR * fieldName,
                                          const rtnParamList * paramList )
   {
      BSONElement toMatch = _toMatch ;
      INT8 paramIndex = _paramIndex ;
      BOOLEAN addToParam = mthEnabledParameterized() ;

      if ( NULL != paramList && !paramList->isEmpty() && _paramIndex >= 0 )
      {
         toMatch = paramList->getParam( _paramIndex ) ;
         paramIndex = -1 ;
         addToParam = FALSE ;
      }

      return predicateSet.addParamPredicate(
                  fieldName, toMatch, getBSONOpType(), _isUnderLogicNot,
                  mthEnabledMixCmp(), addToParam, paramIndex, -1,
                  isTotalConverted() ? _evalCPUCost() : 0 ) ;
   }

   //*******************_mthMatchFuzzyOpNode  ******************
   static BSONObj _mthFuzzyOptrObj ( BOOLEAN inclusive )
   {
      BSONObjBuilder b ;
      b.appendBool( "", inclusive ) ;
      return b.obj() ;
   }

   BSONObj _mthFuzzyIncOptr = _mthFuzzyOptrObj( TRUE ) ;
   BSONObj _mthFuzzyExcOptr = _mthFuzzyOptrObj( FALSE ) ;

   _mthMatchFuzzyOpNode::_mthMatchFuzzyOpNode ( _mthNodeAllocator *allocator,
                                                const mthNodeConfig *config )
   : _mthMatchOpNode( allocator, config )
   {
      // Exclusive is the default value
      _fuzzyOpType = MTH_FUZZY_TYPE_EXCLUSIVE ;
   }

   _mthMatchFuzzyOpNode::~_mthMatchFuzzyOpNode ()
   {
      clear() ;
   }

   BOOLEAN _mthMatchFuzzyOpNode::isTotalConverted()
   {
      if ( _mthMatchOpNode::isTotalConverted() )
      {
         if ( _toMatch.type() == Array )
         {
            // Should not generate rtnPredicate for array
            return FALSE ;
         }
         else
         {
            return TRUE ;
         }
      }
      return FALSE ;
   }

   void _mthMatchFuzzyOpNode::setFuzzyOpType ( EN_MATCH_OP_FUNC_TYPE nodeType )
   {
      INT8 fuzzyType = MTH_FUZZY_TYPE_EXCLUSIVE ;

      if ( mthEnabledFuzzyOptr() )
      {
         // Fuzzy operator is enabled, check the operator whether it could
         // be fuzzy operator
         if ( EN_MATCH_OPERATOR_LTE == nodeType ||
              EN_MATCH_OPERATOR_GTE == nodeType )
         {
            fuzzyType = MTH_FUZZY_TYPE_FUZZY_INC ;
         }
         else if ( EN_MATCH_OPERATOR_LT == nodeType ||
                   EN_MATCH_OPERATOR_GT == nodeType )
         {
            fuzzyType = MTH_FUZZY_TYPE_FUZZY_EXC ;
         }
      }
      else
      {
         // Fuzzy operator is not enabled, use normal inclusive or exclusive
         // type
         if ( EN_MATCH_OPERATOR_LTE == nodeType ||
              EN_MATCH_OPERATOR_GTE == nodeType )
         {
            fuzzyType = MTH_FUZZY_TYPE_INCLUSIVE ;
         }
         else
         {
            fuzzyType = MTH_FUZZY_TYPE_EXCLUSIVE ;
         }
      }

      if ( MTH_FUZZY_TYPE_FUZZY_EXC == fuzzyType ||
           MTH_FUZZY_TYPE_FUZZY_INC == fuzzyType )
      {
         // Could use fuzzy operator only when it could be parameterized
         if ( _canSelfParameterize() )
         {
            _fuzzyOpType = fuzzyType ;
         }
         else if ( MTH_FUZZY_TYPE_FUZZY_EXC == fuzzyType )
         {
            _fuzzyOpType = MTH_FUZZY_TYPE_EXCLUSIVE ;
         }
         else
         {
            _fuzzyOpType = MTH_FUZZY_TYPE_INCLUSIVE ;
         }
      }
      else
      {
         _fuzzyOpType = fuzzyType ;
      }
   }

   INT32 _mthMatchFuzzyOpNode::_valueMatch ( const BSONElement &left,
                                             const BSONElement &right,
                                             BOOLEAN mixCmp,
                                             _mthMatchTreeContext &context,
                                             BOOLEAN &result )
   {
      INT32 rc = SDB_OK ;

      switch ( _fuzzyOpType )
      {
         case MTH_FUZZY_TYPE_EXCLUSIVE :
         case MTH_FUZZY_TYPE_FUZZY_EXC :
         {
            rc = _excValueMatch( left, right, mixCmp, context, result ) ;
            break ;
         }
         case MTH_FUZZY_TYPE_INCLUSIVE :
         case MTH_FUZZY_TYPE_FUZZY_INC :
         {
            rc = _incValueMatch( left, right, mixCmp, context, result ) ;
            break ;
         }
         default :
         {
            PD_CHECK( _fuzzyOpType >= 0, SDB_INVALIDARG, error, PDERROR,
                      "Wrong fuzzy type of operator [%s]", getOperatorStr() ) ;
            BSONElement inclusive = context.getParameter( _fuzzyOpType ) ;
            if ( inclusive.booleanSafe() )
            {
               rc = _incValueMatch( left, right, mixCmp, context, result ) ;
            }
            else
            {
               rc = _excValueMatch( left, right, mixCmp, context, result ) ;
            }
         }
      }

   done :
      return rc ;
   error :
      goto done ;
   }

   void _mthMatchFuzzyOpNode::_toParamBson ( BSONObjBuilder &builder,
                                             const rtnParamList &parameters )
   {
      SDB_ASSERT( -1 != _paramIndex, "_paramIndex is invalid" ) ;

      if ( parameters.isEmpty() )
      {
         return _mthMatchOpNode::_toParamBson( builder, parameters ) ;
      }
      else
      {
         const CHAR *opStr = NULL ;
         if ( _fuzzyOpType >= 0 )
         {
            if ( parameters.getParam( _fuzzyOpType ).booleanSafe() )
            {
               opStr = _getIncOperatorStr() ;
            }
            else
            {
               opStr = _getExcOperatorStr() ;
            }
         }
         else if ( MTH_FUZZY_TYPE_INCLUSIVE == _fuzzyOpType ||
                   MTH_FUZZY_TYPE_FUZZY_INC == _fuzzyOpType )
         {
            opStr = _getIncOperatorStr() ;
         }
         else
         {
            opStr = _getExcOperatorStr() ;
         }
         builder.appendAs( parameters.getParam( _paramIndex ), opStr ) ;
      }
   }

   INT32 _mthMatchFuzzyOpNode::_addPredicate ( rtnPredicateSet & predicateSet,
                                               const CHAR * fieldName,
                                               const rtnParamList * paramList )
   {
      BSONElement toMatch = _toMatch ;
      INT8 paramIndex = _paramIndex ;
      INT8 fuzzyOpType = _fuzzyOpType >= 0 ? _fuzzyOpType : -1 ;
      INT32 opType = getBSONOpType() ;
      BOOLEAN addToParam = mthEnabledParameterized() ;

      if ( NULL != paramList && !paramList->isEmpty() && _paramIndex >= 0 )
      {
         toMatch = paramList->getParam( _paramIndex ) ;
         paramIndex = -1 ;
         fuzzyOpType = -1 ;
         if ( _fuzzyOpType >= 0 )
         {
            if ( paramList->getParam( _fuzzyOpType ).booleanSafe() )
            {
               opType = _getIncBSONOpType() ;
            }
            else
            {
               opType = _getExcBSONOpType() ;
            }
         }
         addToParam = FALSE ;
      }

      return predicateSet.addParamPredicate(
                  fieldName, toMatch, opType, _isUnderLogicNot,
                  mthEnabledMixCmp(), addToParam, paramIndex, fuzzyOpType,
                  isTotalConverted() ? _evalCPUCost() : 0 ) ;
   }

   //*******************_mthMatchOpNodeET***********************
   _mthMatchOpNodeET::_mthMatchOpNodeET( _mthNodeAllocator *allocator,
                                         const mthNodeConfig *config )
                     :_mthMatchOpNode( allocator, config )
   {
   }

   _mthMatchOpNodeET::~_mthMatchOpNodeET()
   {
      clear() ;
   }

   INT32 _mthMatchOpNodeET::getType()
   {
      return ( INT32 )EN_MATCH_OPERATOR_ET ;
   }

   INT32 _mthMatchOpNodeET::getBSONOpType ()
   {
      return BSONObj::Equality ;
   }

   const CHAR* _mthMatchOpNodeET::getOperatorStr()
   {
      return MTH_OPERATOR_STR_ET ;
   }

   UINT32 _mthMatchOpNodeET::getWeight()
   {
      return MTH_WEIGHT_EQUAL ;
   }

   BOOLEAN _mthMatchOpNodeET::isTotalConverted()
   {
      if ( !_mthMatchOpNode::isTotalConverted() )
      {
         return FALSE ;
      }

      if ( _toMatch.type() == Array )
      {
         return FALSE ;
      }

      return TRUE ;
   }

   INT32 _mthMatchOpNodeET::extraEqualityMatches( BSONObjBuilder &builder,
                                                  const rtnParamList *parameters )
   {
      BSONElement ele = _toMatch ;

      if ( _funcList.size() > 0 )
      {
         return SDB_OK ;
      }

      // Bind parameters
      if ( -1 != _paramIndex && NULL != parameters )
      {
         ele = parameters->getParam( _paramIndex ) ;
      }

      if ( !ele.eoo() )
      {
         string fieldName = _fieldName.getFieldName() ;
         if ( string::npos == fieldName.find( '$', 0 ) )
         {
            builder.appendAs( ele, fieldName ) ;
         }
      }

      return SDB_OK ;
   }

   INT32 _mthMatchOpNodeET::_valueMatch( const BSONElement &left,
                                         const BSONElement &right,
                                         BOOLEAN mixCmp,
                                         _mthMatchTreeContext &context,
                                         BOOLEAN &result )
   {
      // No need to check mix-compare mode
      if ( left.canonicalType() == right.canonicalType() )
      {
         if ( 0 == compareElementValues( left, right ) )
         {
            result = TRUE ;
            return SDB_OK ;
         }
      }

      result = FALSE ;
      return SDB_OK ;
   }

   void _mthMatchOpNodeET::_evalEstimation ( const optCollectionStat *pCollectionStat,
                                             double &selectivity,
                                             UINT32 &cpuCost )
   {
      cpuCost = _evalCPUCost() ;

      selectivity = OPT_MTH_OPTR_DEFAULT_SELECTIVITY ;
      if ( _funcList.empty() && pCollectionStat )
      {
         selectivity = pCollectionStat->evalETOpterator( _fieldName.getFieldName() ,
                                                         _toMatch ) ;
      }
   }

   //**************_mthMatchOpNodeNE********************************
   _mthMatchOpNodeNE::_mthMatchOpNodeNE( _mthNodeAllocator *allocator,
                                         const mthNodeConfig *config )
                     :_mthMatchOpNodeET( allocator, config )
   {
   }

   _mthMatchOpNodeNE::~_mthMatchOpNodeNE()
   {
      clear() ;
   }

   INT32 _mthMatchOpNodeNE::getType()
   {
      return EN_MATCH_OPERATOR_NE ;
   }

   INT32 _mthMatchOpNodeNE::getBSONOpType ()
   {
      return BSONObj::NE ;
   }

   const CHAR* _mthMatchOpNodeNE::getOperatorStr()
   {
      return MTH_OPERATOR_STR_NE ;
   }

   UINT32 _mthMatchOpNodeNE::getWeight()
   {
      return MTH_WEIGHT_NE ;
   }

   BOOLEAN _mthMatchOpNodeNE::isTotalConverted()
   {
      // We need further matching to exclusive multiple-element array
      // with given value after index-scan
      // e.g. $ne:3, we don't want [3,5] to be returned
      return FALSE ;
   }

   INT32 _mthMatchOpNodeNE::extraEqualityMatches( BSONObjBuilder &builder,
                                                  const rtnParamList *parameters )
   {
      return SDB_OK ;
   }

   INT32 _mthMatchOpNodeNE::execute( const BSONObj &obj,
                                     _mthMatchTreeContext &context,
                                     BOOLEAN &result )
   {
      INT32 rc = SDB_OK ;

      BOOLEAN tmpResult = FALSE ;
      BOOLEAN gotUndefined = FALSE ;

      if ( _hasReturnMatch )
      {
         context.setReturnMatchExecuted( TRUE ) ;
      }

      rc = _mthMatchOpNodeET::_execute( _fieldName.getFieldName(), obj, FALSE,
                                        context, tmpResult, gotUndefined ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to execute _mthMatchOpNodeNE:rc=%d", rc ) ;
         goto error ;
      }

      if ( gotUndefined )
      {
         // Exclude undefined
         result = FALSE ;
      }
      else
      {
         result = !tmpResult ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   void _mthMatchOpNodeNE::_evalEstimation ( const optCollectionStat *pCollectionStat,
                                             double &selectivity,
                                             UINT32 &cpuCost )
   {
      cpuCost = _evalCPUCost() ;

      selectivity = OPT_MTH_OPTR_DEFAULT_SELECTIVITY ;
      if ( _funcList.empty() && pCollectionStat )
      {
         selectivity = pCollectionStat->evalETOpterator( _fieldName.getFieldName() ,
                                                         _toMatch ) ;
      }
      selectivity = 1.0 - selectivity ;
   }

   //**************_mthMatchOpNodeLT********************************
   _mthMatchOpNodeLT::_mthMatchOpNodeLT( _mthNodeAllocator *allocator,
                                         const mthNodeConfig *config )
                     :_mthMatchFuzzyOpNode( allocator, config )
   {
   }

   _mthMatchOpNodeLT::~_mthMatchOpNodeLT()
   {
      clear() ;
   }

   INT32 _mthMatchOpNodeLT::_incValueMatch ( const BSONElement &left,
                                             const BSONElement &right,
                                             BOOLEAN mixCmp,
                                             _mthMatchTreeContext &context,
                                             BOOLEAN &result )
   {
      // Special cases for minKey and maxKey
      if ( right.canonicalType() == MinKey )
      {
         // $lte:$minKey returns only $minKey
         result = ( left.canonicalType() == MinKey ) ;
         return SDB_OK ;
      }
      else if ( right.canonicalType() == MaxKey )
      {
         // $lte:$maxKey returns everything
         result = TRUE ;
         return SDB_OK ;
      }

      // If in mix-compare mode, left and right could have different canonical
      // types, otherwise, they should have the same canonical types
      if ( left.canonicalType() == right.canonicalType() )
      {
         if ( compareElementValues ( left, right ) <= 0 )
         {
            result = TRUE ;
            return SDB_OK ;
         }
      }
      else if ( mixCmp )
      {
         if ( left.type() == Array && right.type() != Array )
         {
            // Let the caller split array
            result = FALSE ;
            return SDB_OK ;
         }
         if ( left.woCompare( right, FALSE ) <= 0 )
         {
            result = TRUE ;
            return SDB_OK ;
         }
      }

      result = FALSE ;
      return SDB_OK ;
   }

   INT32 _mthMatchOpNodeLT::_excValueMatch ( const BSONElement &left,
                                             const BSONElement &right,
                                             BOOLEAN mixCmp,
                                             _mthMatchTreeContext &context,
                                             BOOLEAN &result )
   {
      // Special cases for minKey and maxKey
      if ( right.canonicalType() == MinKey )
      {
         // $lt:$minKey returns nothing
         result = FALSE ;
         return SDB_OK ;
      }
      else if ( right.canonicalType() == MaxKey )
      {
         // $lt:$maxKey returns everything except for $maxKey
         result = ( left.canonicalType() != MaxKey ) ;
         return SDB_OK ;
      }

      // If in mix-compare mode, left and right could have different canonical
      // types, otherwise, they should have the same canonical types
      if ( left.canonicalType() == right.canonicalType() )
      {
         if ( compareElementValues ( left, right ) < 0 )
         {
            result = TRUE ;
            return SDB_OK ;
         }
      }
      else if ( mixCmp )
      {
         if ( left.type() == Array && right.type() != Array )
         {
            // Let the caller split array
            result = FALSE ;
            return SDB_OK ;
         }
         if ( left.woCompare( right, FALSE ) < 0 )
         {
            result = TRUE ;
            return SDB_OK ;
         }
      }

      result = FALSE ;
      return SDB_OK ;
   }

   void _mthMatchOpNodeLT::_evalEstimation ( const optCollectionStat *pCollectionStat,
                                             double &selectivity,
                                             UINT32 &cpuCost )
   {
      cpuCost = _evalCPUCost() ;

      selectivity = OPT_MTH_OPTR_DEFAULT_SELECTIVITY ;
      if ( _funcList.empty() && pCollectionStat )
      {
         selectivity = pCollectionStat->evalLTOpterator(
               _fieldName.getFieldName(), _toMatch,
               _isExclusive() ? FALSE : TRUE ) ;
      }
   }

   //**************_mthMatchOpNodeGT*****************************
   _mthMatchOpNodeGT::_mthMatchOpNodeGT( _mthNodeAllocator *allocator,
                                         const mthNodeConfig *config )
                     :_mthMatchFuzzyOpNode( allocator, config )
   {
   }

   _mthMatchOpNodeGT::~_mthMatchOpNodeGT()
   {
      clear() ;
   }

   INT32 _mthMatchOpNodeGT::_incValueMatch ( const BSONElement &left,
                                             const BSONElement &right,
                                             BOOLEAN mixCmp,
                                             _mthMatchTreeContext &context,
                                             BOOLEAN &result )
   {
      // Special cases for minKey and maxKey
      if ( right.canonicalType() == MinKey )
      {
         // $gte:$minKey returns everything
         result = TRUE ;
         return SDB_OK ;
      }
      else if ( right.canonicalType() == MaxKey )
      {
         // $gte:$maxKey returns $maxKey
         result = ( left.canonicalType() == MaxKey ) ;
         return SDB_OK ;
      }

      // If in mix-compare mode, left and right could have different canonical
      // types, otherwise, they should have the same canonical types
      if ( left.canonicalType() == right.canonicalType() )
      {
         if ( compareElementValues ( left, right ) >= 0 )
         {
            result = TRUE ;
            return SDB_OK ;
         }
      }
      else if ( mixCmp )
      {
         if ( left.type() == Array && right.type() != Array )
         {
            // Let the caller split array
            result = FALSE ;
            return SDB_OK ;
         }
         if ( left.woCompare( right, FALSE ) >= 0 )
         {
            result = TRUE ;
            return SDB_OK ;
         }
      }

      result = FALSE ;
      return SDB_OK ;
   }

   INT32 _mthMatchOpNodeGT::_excValueMatch ( const BSONElement &left,
                                             const BSONElement &right,
                                             BOOLEAN mixCmp,
                                             _mthMatchTreeContext &context,
                                             BOOLEAN &result )
   {
      // Special cases for minKey and maxKey
      if ( right.canonicalType() == MinKey )
      {
         // $gt:$minKey returns everything except for $minKey
         result = ( left.canonicalType() != MinKey ) ;
         return SDB_OK ;
      }
      else if ( right.canonicalType() == MaxKey )
      {
         // $gt:$maxKey returns nothing
         result = FALSE ;
         return SDB_OK ;
      }

      // If in mix-compare mode, left and right could have different canonical
      // types, otherwise, they should have the same canonical types
      if ( left.canonicalType() == right.canonicalType() )
      {
         if ( compareElementValues ( left, right ) > 0 )
         {
            result = TRUE ;
            return SDB_OK ;
         }
      }
      else if ( mixCmp )
      {
         if ( left.type() == Array && right.type() != Array )
         {
            // Let the caller split array
            result = FALSE ;
            return SDB_OK ;
         }
         if ( left.woCompare( right, FALSE ) > 0 )
         {
            result = TRUE ;
            return SDB_OK ;
         }
      }

      result = FALSE ;
      return SDB_OK ;
   }

   void _mthMatchOpNodeGT::_evalEstimation ( const optCollectionStat *pCollectionStat,
                                             double &selectivity,
                                             UINT32 &cpuCost )
   {
      cpuCost = _evalCPUCost() ;

      selectivity = OPT_MTH_OPTR_DEFAULT_SELECTIVITY ;
      if ( _funcList.empty() && pCollectionStat )
      {
         selectivity = pCollectionStat->evalGTOpterator(
               _fieldName.getFieldName(), _toMatch,
               _isExclusive() ? FALSE : TRUE ) ;
      }
   }

   //**************_mthMatchOpNodeIN*****************************
   _mthMatchOpNodeIN::_mthMatchOpNodeIN( _mthNodeAllocator *allocator,
                                         const mthNodeConfig *config )
                     :_mthMatchOpNode( allocator, config )
   {
   }

   _mthMatchOpNodeIN::~_mthMatchOpNodeIN()
   {
      clear() ;
   }

   INT32 _mthMatchOpNodeIN::_init( const CHAR *fieldName,
                                   const BSONElement &element )
   {
      INT32 rc = SDB_OK ;
      if ( element.type() != Array )
      {
         //element's type must be array
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "element is not Array:element=%s,rc=%d",
                 element.toString().c_str(), rc ) ;
         goto error ;
      }

      {
         BSONObjIterator iter ( element.embeddedObject() ) ;
         while ( iter.more() )
         {
            BSONElement subEle = iter.next() ;
            if ( _flagExpandRegex() &&
                 ( subEle.type() == RegEx ||
                   ( subEle.type() == Object &&
                     !subEle.embeddedObject()[ "$regex" ].eoo() ) ) )
            {
               _mthMatchNode *node             = NULL ;
               _mthMatchOpNodeRegex *regexNode = NULL ;
               node = mthGetMatchNodeFactory()->createOpNode(
                                             _allocator, getMatchConfigPtr(),
                                             EN_MATCH_OPERATOR_REGEX ) ;
               if ( NULL == node )
               {
                  rc = SDB_INVALIDARG ;
                  PD_LOG( PDERROR, "createOpNodeByOp failed:type=%d,rc=%d",
                          EN_MATCH_OPERATOR_REGEX, rc ) ;
                  goto error ;
               }

               regexNode = dynamic_cast< _mthMatchOpNodeRegex * > ( node ) ;
               if ( NULL == regexNode )
               {
                  rc = SDB_INVALIDARG ;
                  PD_LOG( PDERROR, "dynamic_cast(OpNode->OpNodeRegex) failed:"
                          "node=%s,rc=%d", node->toString().c_str(), rc ) ;
                  mthGetMatchNodeFactory()->releaseNode( node ) ;
                  goto error ;
               }

               if ( subEle.type() == RegEx )
               {
                  rc = regexNode->init( fieldName, subEle.regex(),
                                        subEle.regexFlags() ) ;
               }
               else
               {
                  // {$regex:'xxx'} or {$options:'xxx',$regex:'xxx'}
                  rc = regexNode->init(
                        fieldName,
                        subEle.embeddedObject()["$regex"].valuestrsafe(),
                        subEle.embeddedObject()["$options"].valuestrsafe() ) ;
               }
               if ( SDB_OK != rc )
               {
                  PD_LOG( PDERROR, "init regexNode failed:regex=%s,rc=%d",
                          subEle.toString().c_str(), rc ) ;
                  mthGetMatchNodeFactory()->releaseNode( node ) ;
                  goto error ;
               }

               _regexVector.push_back( regexNode ) ;
            }

            _valueSet.insert( subEle ) ;
         }
      }

      _isCompareField = FALSE ;
      _cmpFieldName   = NULL ;

   done:
      return rc ;
   error:
      goto done ;
   }

   void _mthMatchOpNodeIN::_clear()
   {
      UINT32 i = 0 ;
      for ( i = 0 ; i < _regexVector.size() ; i++ )
      {
         _regexVector[i]->clear() ;
         mthGetMatchNodeFactory()->releaseNode( _regexVector[i] ) ;
      }

      _regexVector.clear() ;
      _valueSet.clear() ;
   }

   INT32 _mthMatchOpNodeIN::getType()
   {
      return EN_MATCH_OPERATOR_IN ;
   }

   INT32 _mthMatchOpNodeIN::getBSONOpType ()
   {
      return BSONObj::opIN ;
   }

   const CHAR* _mthMatchOpNodeIN::getOperatorStr()
   {
      return MTH_OPERATOR_STR_IN ;
   }

   UINT32 _mthMatchOpNodeIN::getWeight()
   {
      return MTH_WEIGHT_IN ;
   }

   BOOLEAN _mthMatchOpNodeIN::isTotalConverted()
   {
      if ( _mthMatchOpNode::isTotalConverted() )
      {
         if ( _regexVector.size() == 0 )
         {
            return TRUE ;
         }
      }

      return FALSE ;
   }

   BOOLEAN _mthMatchOpNodeIN::_isMatch ( const RTN_ELEMENT_SET * valueSet,
                                         const BSONElement & ele )
   {
      UINT32 i = 0 ;

      INT32 count = valueSet->count( ele ) ;
      if ( count > 0 )
      {
         return TRUE ;
      }

      for ( i = 0 ; i < _regexVector.size() ; i++ )
      {
         if ( _regexVector[i]->matches( ele ) )
         {
            return TRUE ;
         }
      }

      return FALSE ;
   }

   INT32 _mthMatchOpNodeIN::_valueMatch( const BSONElement &left,
                                         const BSONElement &right,
                                         BOOLEAN mixCmp,
                                         _mthMatchTreeContext &context,
                                         BOOLEAN &result )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN tmpResult = FALSE ;
      const RTN_ELEMENT_SET * valueSet = NULL ;

      if ( -1 != _paramIndex )
      {
         valueSet = context.getParamValueSet( _paramIndex ) ;
      }
      if ( NULL == valueSet )
      {
         valueSet = ( &_valueSet ) ;
      }

      if ( valueSet->size() == 0 &&
           _regexVector.size() == 0 &&
           left.type() == Array )
      {
         BSONObj obj = left.embeddedObject() ;
         if ( obj.nFields() == 0 )
         {
            result = TRUE ;
            if ( _hasReturnMatch )
            {
               context.setIsUseElement( TRUE ) ;
            }

            goto done ;
         }
      }

      if ( Array != left.type() )
      {
         result = _isMatch( valueSet, left ) ;
         goto done ;
      }

      {
         INT32 index = 0 ;
         BSONObjIterator iter( left.embeddedObject() ) ;
         while ( iter.more() )
         {
            BOOLEAN isMatch = FALSE ;
            BSONElement ele = iter.next() ;
            isMatch = _isMatch( valueSet, ele ) ;
            rc = _saveElement( context, isMatch, index ) ;
            PD_RC_CHECK( rc, PDERROR, "_saveElement failed:rc=%d", rc ) ;

            if ( isMatch )
            {
               tmpResult = isMatch ;
               if ( !_hasReturnMatch )
               {
                  break ;
               }
            }

            index++ ;
         }
      }

      if ( _hasReturnMatch )
      {
         rc = context.subElements( _offset, _len ) ;
         PD_RC_CHECK( rc, PDERROR, "set subElements failed:rc=%d", rc ) ;
      }

      result = tmpResult ;

   done:
      return rc ;
   error:
      goto done ;
   }

   UINT32 _mthMatchOpNodeIN::_evalCPUCost () const
   {
      if ( _valueSet.empty() && _regexVector.empty() )
      {
         return OPT_MTH_FIELD_EXTRACT_CPU_COST + OPT_MTH_OPTR_BASE_CPU_COST ;
      }
      return OPT_MTH_FIELD_EXTRACT_CPU_COST + OPT_MTH_OPTR_BASE_CPU_COST *
             ( _valueSet.size() + _regexVector.size() ) ;
   }

   void _mthMatchOpNodeIN::_evalEstimation ( const optCollectionStat *pCollectionStat,
                                             double &selectivity,
                                             UINT32 &cpuCost )
   {
      selectivity = OPT_MTH_OPTR_DEFAULT_SELECTIVITY ;
      cpuCost = _evalCPUCost() ;
   }

   //**************_mthMatchOpNodeNIN*****************************
   _mthMatchOpNodeNIN::_mthMatchOpNodeNIN( _mthNodeAllocator *allocator,
                                           const mthNodeConfig *config )
                      :_mthMatchOpNodeIN( allocator, config )
   {
   }

   _mthMatchOpNodeNIN::~_mthMatchOpNodeNIN()
   {
      clear() ;
   }

   INT32 _mthMatchOpNodeNIN::getType()
   {
      return EN_MATCH_OPERATOR_NIN ;
   }

   INT32 _mthMatchOpNodeNIN::getBSONOpType ()
   {
      return BSONObj::NIN ;
   }

   const CHAR* _mthMatchOpNodeNIN::getOperatorStr()
   {
      return MTH_OPERATOR_STR_NIN ;
   }

   UINT32 _mthMatchOpNodeNIN::getWeight()
   {
      return MTH_WEIGHT_NIN ;
   }

   BOOLEAN _mthMatchOpNodeNIN::isTotalConverted()
   {
      return FALSE ;
   }

   INT32 _mthMatchOpNodeNIN::_valueMatch( const BSONElement &left,
                                          const BSONElement &right,
                                          BOOLEAN mixCmp,
                                          _mthMatchTreeContext &context,
                                          BOOLEAN &result )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN tmpResult = TRUE ;
      if ( _valueSet.size() == 0 && _regexVector.size() == 0 &&
           left.type() == Array )
      {
         BSONObj obj = left.embeddedObject() ;
         if ( obj.nFields() == 0 )
         {
            result = FALSE ;
            goto done ;
         }
      }

      if ( Array != left.type() )
      {
         result = !_isMatch( &_valueSet, left ) ;
         goto done ;
      }

      {
         INT32 index = 0 ;
         BSONObjIterator iter( left.embeddedObject() ) ;
         while ( iter.more() )
         {
            BOOLEAN isFound = FALSE ;
            BSONElement ele = iter.next() ;
            isFound = _isMatch( &_valueSet, ele ) ;
            rc = _saveElement( context, !isFound, index ) ;
            PD_RC_CHECK( rc, PDERROR, "_saveElement failed:rc=%d", rc ) ;

            if ( isFound )
            {
               // if we can find one. the total result is false
               tmpResult = !isFound ;
               if ( !_hasReturnMatch )
               {
                  break ;
               }
            }

            index++ ;
         }
      }

      if ( _hasReturnMatch )
      {
         rc = context.subElements( _offset, _len ) ;
         PD_LOG( PDERROR, "set subElements failed:rc=%d", rc ) ;
      }

      result = tmpResult ;

   done:
      return rc ;
   error:
      goto done ;
   }

   //**************_mthMatchOpNodeALL*****************************
   _mthMatchOpNodeALL::_mthMatchOpNodeALL( _mthNodeAllocator *allocator,
                                           const mthNodeConfig *config )
                      :_mthMatchOpNodeIN( allocator, config )
   {
   }

   _mthMatchOpNodeALL::~_mthMatchOpNodeALL()
   {
      clear() ;
   }

   INT32 _mthMatchOpNodeALL::getType()
   {
      return EN_MATCH_OPERATOR_ALL ;
   }

   INT32 _mthMatchOpNodeALL::getBSONOpType ()
   {
      return BSONObj::opALL ;
   }

   const CHAR* _mthMatchOpNodeALL::getOperatorStr()
   {
      return MTH_OPERATOR_STR_ALL ;
   }

   UINT32 _mthMatchOpNodeALL::getWeight()
   {
      return MTH_WEIGHT_ALL ;
   }

   BOOLEAN _mthMatchOpNodeALL::isTotalConverted()
   {
      return FALSE ;
   }

   INT32 _mthMatchOpNodeALL::extraEqualityMatches( BSONObjBuilder &builder,
                                                   const rtnParamList *parameters )
   {
      BSONElement ele = _toMatch ;

      if ( _funcList.size() > 0 )
      {
         return SDB_OK ;
      }

      // Bind parameters
      if ( -1 != _paramIndex && NULL != parameters )
      {
         ele = parameters->getParam( _paramIndex ) ;
      }

      if ( !ele.eoo() )
      {
         string fieldName = _fieldName.getFieldName() ;
         if ( string::npos == fieldName.find( '$', 0 ) )
         {
            builder.appendAs( ele, fieldName ) ;
         }
      }

      return SDB_OK ;
   }

   BOOLEAN _mthMatchOpNodeALL::_isMatchSingle( const BSONElement &ele )
   {
      UINT32 i = 0 ;
      RTN_ELEMENT_SET::iterator iterSet ;
      iterSet = _valueSet.begin() ;
      while ( iterSet != _valueSet.end() )
      {
         // all values in _valueSet must equals left
         if ( ele.woCompare( *iterSet, FALSE ) != 0 )
         {
            return FALSE ;
         }

         iterSet++ ;
      }

      for ( i = 0 ; i < _regexVector.size() ; i++ )
      {
         // all regexs in _regexVector must equals left
         if ( !_regexVector[i]->matches( ele ) )
         {
            return FALSE ;
         }
      }

      return TRUE ;
   }

   INT32 _mthMatchOpNodeALL::_valueMatchWithReturnMatch(
                                                 const BSONElement &left,
                                                 const BSONElement &right,
                                                 _mthMatchTreeContext &context,
                                                 BOOLEAN &result )
   {
      INT32 rc = SDB_OK ;
      RTN_ELEMENT_SET::iterator iterSet ;

      UINT32 i = 0 ;

      if ( left.type() != Array )
      {
         result = _isMatchSingle( left ) ;
         goto done ;
      }

      iterSet = _valueSet.begin() ;
      while ( iterSet != _valueSet.end() )
      {
         BOOLEAN tmpResult = FALSE ;
         // all values in _valueSet must exist in array left
         INT32 index = 0 ;
         BSONObj tmpObj = left.embeddedObject() ;
         BSONObjIterator iter( tmpObj ) ;
         while ( iter.more() )
         {
            BOOLEAN singleResult = FALSE ;
            BSONElement ele = iter.next() ;
            singleResult = ( 0 == ele.woCompare( *iterSet, FALSE ) ) ;
            if ( singleResult )
            {
               tmpResult = singleResult ;
            }

            rc = _saveElement( context, singleResult, index ) ;
            PD_RC_CHECK( rc, PDERROR, "_saveElement failed:rc=%d", rc ) ;

            index++ ;
         }

         if ( !tmpResult )
         {
            result = FALSE ;
            goto done ;
         }

         iterSet++ ;
      }

      for ( i = 0 ; i < _regexVector.size() ; i++ )
      {
         // all regexs in _regexVector must exist in leftValueSet
         BOOLEAN tmpResult = FALSE ;

         INT32 index = 0 ;
         BSONObj tmpObj = left.embeddedObject() ;
         BSONObjIterator iter( tmpObj ) ;
         while ( iter.more() )
         {
            BOOLEAN singleResult = FALSE ;
            BSONElement ele = iter.next() ;
            singleResult = _regexVector[i]->matches( ele ) ;
            if ( singleResult )
            {
               tmpResult = singleResult ;
            }

            rc = _saveElement( context, singleResult, index ) ;
            PD_RC_CHECK( rc, PDERROR, "_saveElement failed:rc=%d", rc ) ;

            index++ ;
         }

         if ( !tmpResult )
         {
            result = FALSE ;
            goto done ;
         }
      }

      result = TRUE ;

      if ( _hasReturnMatch )
      {
         rc = context.subElements( _offset, _len ) ;
         PD_LOG( PDERROR, "set subElements failed:rc=%d", rc ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   BOOLEAN _mthMatchOpNodeALL::_valueMatchNoReturnMatch(
                                                    const BSONElement &left,
                                                    const BSONElement &right )
   {
      UINT32 i = 0 ;
      RTN_ELEMENT_SET::iterator iterSet ;
      RTN_ELEMENT_SET leftValueSet ;
      if ( Array != left.type() )
      {
         leftValueSet.insert( left ) ;
      }
      else
      {
         BSONObjIterator iter( left.embeddedObject() ) ;
         while ( iter.more() )
         {
            BSONElement ele = iter.next() ;
            leftValueSet.insert( ele ) ;
         }
      }

      iterSet = _valueSet.begin() ;
      while ( iterSet != _valueSet.end() )
      {
         // all values in _valueSet must exist in lefValueSet
         INT32 count = leftValueSet.count( *iterSet ) ;
         if ( count == 0 )
         {
            return FALSE ;
         }

         iterSet++ ;
      }

      for ( i = 0 ; i < _regexVector.size() ; i++ )
      {
         // all regexs in _regexVector must exist in leftValueSet
         BOOLEAN isMatch = FALSE ;
         iterSet = leftValueSet.begin() ;
         while ( iterSet != leftValueSet.end() )
         {
            if ( _regexVector[i]->matches( *iterSet ) )
            {
               isMatch = TRUE ;
               break ;
            }

            iterSet++ ;
         }

         if ( !isMatch )
         {
            return FALSE ;
         }
      }

      return TRUE ;
   }

   INT32 _mthMatchOpNodeALL::_valueMatch( const BSONElement &left,
                                          const BSONElement &right,
                                          BOOLEAN mixCmp,
                                          _mthMatchTreeContext &context,
                                          BOOLEAN &result )
   {
      // all _toMatch elements must exist in left ;
      INT32 rc = SDB_OK ;

      if ( Array != left.type() && Object != left.type() )
      {
         if ( _valueSet.size() == 0 && _regexVector.size() == 0 )
         {
            // {a:1} do not match {a:{$all:[]}}, while {a:[1]} do
            result = FALSE ;
            goto done ;
         }
      }

      if ( !_hasReturnMatch )
      {
         result = _valueMatchNoReturnMatch( left, right ) ;
      }
      else
      {
         context.setIsUseElement( TRUE ) ;
         rc = _valueMatchWithReturnMatch( left, right, context, result ) ;
         PD_RC_CHECK( rc, PDERROR, "_valueMatchWithReturn failed:rc=%d", rc ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   //**************_mthMatchOpNodeEXISTS*****************************
   _mthMatchOpNodeEXISTS::_mthMatchOpNodeEXISTS( _mthNodeAllocator *allocator,
                                                 const mthNodeConfig *config )
                         :_mthMatchOpNode( allocator, config )
   {
   }

   _mthMatchOpNodeEXISTS::~_mthMatchOpNodeEXISTS()
   {
      clear() ;
   }

   INT32 _mthMatchOpNodeEXISTS::getType()
   {
      return EN_MATCH_OPERATOR_EXISTS ;
   }

   INT32 _mthMatchOpNodeEXISTS::getBSONOpType ()
   {
      return BSONObj::opEXISTS ;
   }

   const CHAR* _mthMatchOpNodeEXISTS::getOperatorStr()
   {
      return MTH_OPERATOR_STR_EXISTS ;
   }

   UINT32 _mthMatchOpNodeEXISTS::getWeight()
   {
      return MTH_WEIGHT_EXISTS ;
   }

   BOOLEAN _mthMatchOpNodeEXISTS::isTotalConverted()
   {
      return FALSE ;
   }

   INT32 _mthMatchOpNodeEXISTS::_valueMatch( const BSONElement &left,
                                             const BSONElement &right,
                                             BOOLEAN mixCmp,
                                             _mthMatchTreeContext &context,
                                             BOOLEAN &result )
   {
      if ( left.eoo() )
      {
         if ( _toMatch.trueValue() )
         {
            //expect exists
            result = FALSE ;
            return SDB_OK ;
         }

         result = TRUE ;
         return SDB_OK ;
      }
      else
      {
         if ( _toMatch.trueValue() )
         {
            result = TRUE ;
            return SDB_OK ;
         }

         result = FALSE ;
         return SDB_OK ;
      }
   }

   //**************_mthMatchOpNodeMOD*****************************
   _mthMatchOpNodeMOD::_mthMatchOpNodeMOD( _mthNodeAllocator *allocator,
                                           const mthNodeConfig *config )
                      :_mthMatchOpNode( allocator, config )
   {
   }

   _mthMatchOpNodeMOD::~_mthMatchOpNodeMOD()
   {
      clear() ;
   }

   BOOLEAN _mthMatchOpNodeMOD::_isModValid( const BSONElement &modmEle )
   {
      return mthIsModValid( modmEle ) ;
   }

   INT32 _mthMatchOpNodeMOD::_init( const CHAR *fieldName,
                                    const BSONElement &element )
   {
      INT32 rc = SDB_OK ;
      BSONObj o ;

      if ( element.type() != Array )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG ( PDERROR, "$mod's element must be Array:element=%s,rc=%d",
                  element.toString().c_str(), rc ) ;
         goto error ;
      }

      o = element.embeddedObject() ;
      if ( o.nFields() != 2 )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG ( PDERROR, "$mod's element must have two fields:element=%s,"
                  "rc=%d", element.toString().c_str(), rc ) ;
         goto error ;
      }

      if ( !o["0"].isNumber() || !o["1"].isNumber() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG ( PDERROR, "$mod's fields must be number type:element=%s,"
                  "rc=%d", element.toString().c_str(), rc ) ;
         goto error ;
      }

      if ( !_isModValid( o["0"] ) )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG ( PDERROR, "Modulo is invalid:Modulo=%s,rc=%d",
                  o["0"].toString().c_str(), rc ) ;
         goto error ;
      }

      _mod       = o["0"] ;
      _modResult = o["1"] ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _mthMatchOpNodeMOD::getType()
   {
      return EN_MATCH_OPERATOR_MOD ;
   }

   INT32 _mthMatchOpNodeMOD::getBSONOpType ()
   {
      return BSONObj::opMOD ;
   }

   const CHAR* _mthMatchOpNodeMOD::getOperatorStr()
   {
      return MTH_OPERATOR_STR_MOD ;
   }

   UINT32 _mthMatchOpNodeMOD::getWeight()
   {
      return MTH_WEIGHT_MOD ;
   }

   BOOLEAN _mthMatchOpNodeMOD::isTotalConverted()
   {
      return FALSE ;
   }

   INT32 _mthMatchOpNodeMOD::_valueMatch( const BSONElement &left,
                                          const BSONElement &right,
                                          BOOLEAN mixCmp,
                                          _mthMatchTreeContext &context,
                                          BOOLEAN &result )
   {
      INT32 rc = SDB_OK ;
      if ( !left.isNumber() )
      {
         result = FALSE ;
         goto done ;
      }

      if ( NumberDecimal == left.type() || NumberDecimal == _mod.type() )
      {
         bsonDecimal decimal ;
         bsonDecimal decimalMod ;
         bsonDecimal decimalModm ;
         bsonDecimal resultDecimal ;

         decimal    = left.numberDecimal() ;
         decimalMod = _mod.numberDecimal() ;
         rc         = decimal.mod( decimalMod, resultDecimal ) ;
         PD_RC_CHECK( rc, PDERROR, "failed to mod decimal:%s mod %s,rc=%d",
                      decimal.toString().c_str(),
                      decimalMod.toString().c_str(), rc ) ;

         decimalModm = _modResult.numberDecimal() ;
         result = ( 0 == resultDecimal.compare( decimalModm ) ) ;
      }
      else if ( NumberDouble == left.type()
                && NumberDouble == _mod.type() )
      {
         FLOAT64 v = MTH_MOD( left.numberDouble(),
                              _mod.numberDouble() ) ;
         result = ( fabs( v - _modResult.numberDouble() ) <= OSS_EPSILON ) ;
      }
      else if ( NumberDouble != left.type()
                && NumberDouble == _mod.type() )
      {
         FLOAT64 v = MTH_MOD( left.numberLong(),
                              _mod.numberDouble() ) ;
         result = ( fabs( v - _modResult.numberDouble() ) <= OSS_EPSILON ) ;
      }
      else if ( NumberDouble == left.type()
                && NumberDouble != _mod.type())
      {
         FLOAT64 v = MTH_MOD( left.numberDouble(),
                              _mod.numberLong() ) ;
         result = ( fabs( v - _modResult.numberDouble() ) <= OSS_EPSILON ) ;
      }
      else
      {
         result = ( ( left.numberLong() % _mod.numberLong() )
                                            == _modResult.numberLong() ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   //**************_mthMatchOpNodeTYPE*****************************
   _mthMatchOpNodeTYPE::_mthMatchOpNodeTYPE( _mthNodeAllocator *allocator,
                                             const mthNodeConfig *config )
                       :_mthMatchOpNode( allocator, config )
   {
   }

   _mthMatchOpNodeTYPE::~_mthMatchOpNodeTYPE()
   {
      clear() ;
   }

   INT32 _mthMatchOpNodeTYPE::_init( const CHAR *fieldName,
                                     const BSONElement &element )
   {
      _type = element.numberInt() ;
      return SDB_OK ;
   }

   INT32 _mthMatchOpNodeTYPE::getType()
   {
      return EN_MATCH_OPERATOR_TYPE ;
   }

   INT32 _mthMatchOpNodeTYPE::getBSONOpType ()
   {
      return BSONObj::opTYPE ;
   }

   const CHAR* _mthMatchOpNodeTYPE::getOperatorStr()
   {
      return MTH_OPERATOR_STR_TYPE ;
   }

   UINT32 _mthMatchOpNodeTYPE::getWeight()
   {
      return MTH_WEIGHT_TYPE ;
   }

   BOOLEAN _mthMatchOpNodeTYPE::isTotalConverted()
   {
      return FALSE ;
   }

   INT32 _mthMatchOpNodeTYPE::_valueMatch( const BSONElement &left,
                                           const BSONElement &right,
                                           BOOLEAN mixCmp,
                                           _mthMatchTreeContext &context,
                                           BOOLEAN &result )
   {
      result = left.type() == _type ;
      return SDB_OK ;
   }

   //**************_mthMatchOpNodeISNULL*****************************
   _mthMatchOpNodeISNULL::_mthMatchOpNodeISNULL( _mthNodeAllocator *allocator,
                                                 const mthNodeConfig *config )
                         :_mthMatchOpNode( allocator, config )
   {
   }

   _mthMatchOpNodeISNULL::~_mthMatchOpNodeISNULL()
   {
      clear() ;
   }

   INT32 _mthMatchOpNodeISNULL::getType()
   {
      return EN_MATCH_OPERATOR_ISNULL ;
   }

   INT32 _mthMatchOpNodeISNULL::getBSONOpType ()
   {
      return BSONObj::opISNULL ;
   }

   const CHAR* _mthMatchOpNodeISNULL::getOperatorStr()
   {
      return MTH_OPERATOR_STR_ISNULL ;
   }

   UINT32 _mthMatchOpNodeISNULL::getWeight()
   {
      return MTH_WEIGHT_ISNULL ;
   }

   BOOLEAN _mthMatchOpNodeISNULL::isTotalConverted()
   {
      return FALSE ;
   }

   INT32 _mthMatchOpNodeISNULL::_valueMatch( const BSONElement &left,
                                             const BSONElement &right,
                                             BOOLEAN mixCmp,
                                             _mthMatchTreeContext &context,
                                             BOOLEAN &result )
   {
      if ( _toMatch.trueValue() )
      {
         if ( left.eoo() || left.isNull() )
         {
            result = TRUE ;
         }
         else
         {
            result = FALSE ;
         }
      }
      else
      {
         if ( left.eoo() || left.isNull() )
         {
            result = FALSE ;
         }
         else
         {
            result = TRUE ;
         }
      }

      return SDB_OK ;
   }


   //**************_mthMatchOpNodeEXPAND*****************************
   _mthMatchOpNodeEXPAND::_mthMatchOpNodeEXPAND( _mthNodeAllocator *allocator,
                                                 const mthNodeConfig *config )
                         :_mthMatchOpNode( allocator, config )
   {
   }

   _mthMatchOpNodeEXPAND::~_mthMatchOpNodeEXPAND()
   {
      clear() ;
   }

   INT32 _mthMatchOpNodeEXPAND::getType()
   {
      return EN_MATCH_ATTR_EXPAND ;
   }

   INT32 _mthMatchOpNodeEXPAND::getBSONOpType ()
   {
      return -1 ;
   }

   const CHAR* _mthMatchOpNodeEXPAND::getOperatorStr()
   {
      return MTH_ATTR_STR_EXPAND ;
   }

   UINT32 _mthMatchOpNodeEXPAND::getWeight()
   {
      return 0 ;
   }

   BOOLEAN _mthMatchOpNodeEXPAND::isTotalConverted()
   {
      return FALSE ;
   }

   INT32 _mthMatchOpNodeEXPAND::calcPredicate( rtnPredicateSet &predicateSet,
                                               const rtnParamList * paramList )
   {
      // $expand has no predicate
      return SDB_OK ;
   }

   INT32 _mthMatchOpNodeEXPAND::_valueMatch( const BSONElement &left,
                                             const BSONElement &right,
                                             BOOLEAN mixCmp,
                                             _mthMatchTreeContext &context,
                                             BOOLEAN &result )
   {
      result = TRUE ;
      return SDB_OK ;
   }

   //**************_mthMatchOpNodeELEMMATCH*****************************
   _mthMatchOpNodeELEMMATCH::_mthMatchOpNodeELEMMATCH(
                                              _mthNodeAllocator *allocator,
                                              const mthNodeConfig *config )
                            :_mthMatchOpNode( allocator, config ),
                             _subTree( NULL )
   {
   }

   _mthMatchOpNodeELEMMATCH::~_mthMatchOpNodeELEMMATCH()
   {
      clear() ;
   }

   INT32 _mthMatchOpNodeELEMMATCH::_init( const CHAR *fieldName,
                                          const BSONElement &element )
   {
      INT32 rc = SDB_OK ;

      //BSONElement m = e ;
      if ( element.type() != Object )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG ( PDERROR, "$elemMatch's element must be Object:element=%s,"
                  "rc=%d", element.toString().c_str(), rc ) ;
         goto error ;
      }

      _subTree = mthGetMatchNodeFactory()->createTree() ;
      if ( NULL == _subTree )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "create subTree failed:rc=%d", rc) ;
         goto error ;
      }

      _subTree->setMthEnableMixCmp( mthEnabledMixCmp() ) ;

      rc = _subTree->loadPattern( element.embeddedObject(), FALSE ) ;
      PD_RC_CHECK( rc, PDERROR, "failed to loadPattern:obj=%s,rc=%d",
                   element.embeddedObject().toString().c_str(), rc ) ;

      if ( _subTree->hasDollarFieldName() )
      {
         _hasDollarFieldName = TRUE ;
      }

      if ( _subTree->hasExpand() || _subTree->hasReturnMatch() )
      {
         rc = SDB_INVALIDARG ;
         PD_RC_CHECK( rc, PDERROR, "subTree can't support %s or %s",
                      MTH_ATTR_STR_EXPAND, MTH_ATTR_STR_RETURNMATCH ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   void _mthMatchOpNodeELEMMATCH::_clear()
   {
      if ( NULL != _subTree )
      {
         _subTree->clear() ;
         mthGetMatchNodeFactory()->releaseTree( _subTree ) ;
      }

      _subTree = NULL ;
   }

   INT32 _mthMatchOpNodeELEMMATCH::getType()
   {
      return EN_MATCH_OPERATOR_ELEMMATCH ;
   }

   INT32 _mthMatchOpNodeELEMMATCH::getBSONOpType ()
   {
      return BSONObj::opELEM_MATCH ;
   }

   const CHAR* _mthMatchOpNodeELEMMATCH::getOperatorStr()
   {
      return MTH_OPERATOR_STR_ELEMMATCH ;
   }

   UINT32 _mthMatchOpNodeELEMMATCH::getWeight()
   {
      return MTH_WEIGHT_ELEMMATCH ;
   }

   BOOLEAN _mthMatchOpNodeELEMMATCH::isTotalConverted()
   {
      return FALSE ;
   }

   INT32 _mthMatchOpNodeELEMMATCH::_valueMatch( const BSONElement &left,
                                                const BSONElement &right,
                                                BOOLEAN mixCmp,
                                                _mthMatchTreeContext &context,
                                                BOOLEAN &result )
   {
      // for eleMatch, such like {a:{$eleMatch:{b:1}}}, this will
      // match {a:{b:1}}
      // or {a:{$eleMatch:{$and:[{b:1},{c:1}]}}}
      // this will match {a:{b:1,c:1}}
      // we do not support {a:{$eleMatch:{$lt:1}}} at the moment. The
      // object in eleMatch must be a full matching condition
      INT32 rc = SDB_OK ;
      _mthMatchTreeContext subContext ;

      if ( left.type() != Object && left.type() != Array )
      {
         result = FALSE ;
         goto done ;
      }

      if ( context.isDollarListEnabled() )
      {
         subContext.enableDollarList() ;
      }

      // It might have a different mix-cmp mode
      if ( mixCmp != mthEnabledMixCmp() )
      {
         _subTree->setMthEnableMixCmp( mixCmp ) ;
      }

      if ( Array == left.type() )
      {
         BOOLEAN tmpResult = FALSE ;
         BSONObjIterator iter( left.embeddedObject() ) ;
         while ( iter.more() )
         {
            BSONElement innerEle = iter.next() ;
            if ( innerEle.type() == Object || innerEle.type() == Array )
            {
               //do not clear dollarlist flag
               subContext.clearRecordInfo() ;
               rc = _subTree->matches( innerEle.embeddedObject(), result,
                                       &subContext ) ;
               PD_RC_CHECK( rc, PDERROR, "matches subtree failed:rc=%d", rc ) ;
               context.appendDollarList( subContext._dollarList ) ;
            }
            else
            {
               result = FALSE ;
            }

            if ( result )
            {
               tmpResult = result ;
               if ( !context.isDollarListEnabled() )
               {
                  break ;
               }
            }
         }

         result = tmpResult ;
      }
      else
      {
         //Object
         rc = _subTree->matches( left.embeddedObject(), result, &subContext ) ;
         PD_RC_CHECK( rc, PDERROR, "matches subtree failed:rc=%d", rc ) ;
         context.appendDollarList( subContext._dollarList ) ;
      }

      if ( mixCmp != mthEnabledMixCmp() )
      {
         _subTree->setMthEnableMixCmp( mthEnabledMixCmp() ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   void _mthMatchOpNodeELEMMATCH::_evalEstimation ( const optCollectionStat *pCollectionStat,
                                                    double &selectivity,
                                                    UINT32 &cpuCost )
   {
      double tempSelectivity = OPT_MTH_OPTR_DEFAULT_SELECTIVITY ;
      UINT32 tempCPUCost = OPT_MTH_OPTR_BASE_CPU_COST ;

      if ( _subTree )
      {
         // The fields in _subTree is not the root level
         _subTree->evalEstimation( NULL, tempSelectivity, tempCPUCost ) ;
      }

      selectivity = OPT_ROUND_SELECTIVITY( tempSelectivity ) ;
      cpuCost = OPT_MTH_FIELD_EXTRACT_CPU_COST + OPT_MTH_OPTR_BASE_CPU_COST +
                tempCPUCost ;
   }

   //**************_mthMatchOpNodeRegex*****************************
   _mthMatchOpNodeRegex::_mthMatchOpNodeRegex( _mthNodeAllocator *allocator,
                                               const mthNodeConfig *config )
   : _mthMatchOpNode( allocator, config ),
     _regex( NULL ),
     _options( NULL ),
     _isSimpleMatch( FALSE )
   {
   }

   _mthMatchOpNodeRegex::~_mthMatchOpNodeRegex()
   {
      clear() ;
   }

   INT32 _mthMatchOpNodeRegex::init( const CHAR *fieldName,
                                     const BSONElement &element )
   {
      SDB_ASSERT( FALSE, "do not called this init function" ) ;
      return SDB_INVALIDARG ;
   }

   BOOLEAN _mthMatchOpNodeRegex::_isPureWords( const char* regex,
                                               const char* options )
   {
      BOOLEAN extended = FALSE ;
      if( options )
      {
         while ( *options )
         {
            switch ( *( options++ ) )
            {
            case 'm': // multiline
            case 's':
               continue ;
            case 'x': // extended
               extended = TRUE ;
               continue ;
            default:
               return FALSE ;
            }
         }
      }

      if( regex )
      {
         //check if the regex contains metacharacters
         while( *regex )
         {
            CHAR c = *( regex++ ) ;
            if( ossStrchr( "|?*\\^$.[()+{", c ) ||
                ( ossStrchr( "# ", c ) && extended ) )
            {
               return FALSE ;
            }
         }
      }
      else
      {
         return FALSE ;
      }

      return TRUE ;
   }

   INT32 _mthMatchOpNodeRegex::_flags2options( const char* options,
                                               pcrecpp::RE_Options &reOptions )
   {
      INT32 rc = SDB_OK ;
      reOptions.set_utf8( true ) ;
      while ( options && *options )
      {
         if ( *options == 'i' )
         {
            reOptions.set_caseless( true ) ;
         }
         else if ( *options == 'm' )
         {
            reOptions.set_multiline( true ) ;
         }
         else if ( *options == 'x' )
         {
            reOptions.set_extended( true ) ;
         }
         else if ( *options == 's' )
         {
            reOptions.set_dotall( true ) ;
         }
         else
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Invalid options[%s]:rc=%d", options, rc ) ;
            goto error ;
         }

         options++ ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   //use this to init _mthMatchOpNodeRegex.(not graceful here)
   INT32 _mthMatchOpNodeRegex::init( const CHAR *fieldName, const CHAR *regex,
                                     const CHAR *options )
   {
      INT32 rc = SDB_OK ;
      pcrecpp::RE_Options reOptions ;
      BSONObjBuilder builder ;
      rc = _fieldName.setFieldName( fieldName ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "setFieldName failed:fieldName=%s,rc=%d",
                 fieldName, rc ) ;
         goto error ;
      }

      if ( NULL != ossStrstr( fieldName, ".$" ) )
      {
         _hasDollarFieldName = TRUE ;
      }

      _regex = regex ;
      if ( NULL == options )
      {
         _options = "" ;
      }
      else
      {
         _options = options ;
      }

      rc = _flags2options( _options, reOptions ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to parse option[%s]:rc=%d", _options,
                   rc ) ;

      builder.appendRegex( fieldName, _regex, _options ) ;
      _matchObj = builder.obj() ;
      _toMatch  = _matchObj.firstElement() ;

      _isSimpleMatch = _isPureWords( _regex, _options ) ;
      _re.reset( new RE( _regex, reOptions ) ) ;

   done:
      return rc ;
   error:
      clear() ;
      goto done ;
   }

   INT32 _mthMatchOpNodeRegex::getType()
   {
      return ( INT32 ) EN_MATCH_OPERATOR_REGEX ;
   }

   INT32 _mthMatchOpNodeRegex::getBSONOpType ()
   {
      return BSONObj::opREGEX ;
   }

   const CHAR* _mthMatchOpNodeRegex::getOperatorStr()
   {
      return MTH_OPERATOR_STR_REGEX ;
   }

   void _mthMatchOpNodeRegex::_clear()
   {
      _regex   = NULL ;
      _options = NULL ;
      _isSimpleMatch = FALSE ;
      _re.reset() ;
   }

   BSONObj _mthMatchOpNodeRegex::_toBson ( const rtnParamList &parameters )
   {
      BSONObjBuilder builder ;
      if ( _funcList.size() == 0 && !_hasReturnMatch && !_hasExpand )
      {
         builder.appendRegex( _fieldName.getFieldName(), _regex, _options ) ;
      }
      else
      {
         MTH_FUNC_LIST::iterator iter ;
         BSONObjBuilder b( builder.subobjStart( _fieldName.getFieldName() ) ) ;

         if ( _hasExpand )
         {
            b.append( MTH_ATTR_STR_EXPAND, 1 ) ;
         }

         if ( _hasReturnMatch )
         {
            BSONObj tmp = BSON( "0" << _offset << "1" << _len ) ;
            b.appendArray( MTH_ATTR_STR_RETURNMATCH, tmp ) ;
         }

         iter = _funcList.begin() ;
         while ( iter != _funcList.end() )
         {
            _mthMatchFunc *func = *iter ;
            BSONObj obj = func->toBson() ;
            BSONElement ele = obj.firstElement() ;
            if ( ele.type() == Object || ele.type() == Array )
            {
               b.appendElements( ele.embeddedObject() ) ;
            }

            iter++ ;
         }

         b.append( MTH_OPERATOR_STR_REGEX, _regex ) ;
         b.append( MTH_OPERATOR_STR_OPTIONS, _options ) ;

         b.doneFast() ;
      }

      return builder.obj() ;
   }

   BOOLEAN _mthMatchOpNodeRegex::isTotalConverted()
   {
      return FALSE ;
   }

   UINT32 _mthMatchOpNodeRegex::getWeight()
   {
      return MTH_WEIGHT_REGEX ;
   }

   INT32 _mthMatchOpNodeRegex::_valueMatch( const BSONElement &left,
                                            const BSONElement &right,
                                            BOOLEAN mixCmp,
                                            _mthMatchTreeContext &context,
                                            BOOLEAN &result )
   {
      result = matches( left ) ;
      return SDB_OK ;
   }

   BOOLEAN _mthMatchOpNodeRegex::matches( const BSONElement &ele )
   {
      switch ( ele.type() )
      {
      case String:
      case Symbol:
         if ( _isSimpleMatch )
         {
            return ossStrstr( ele.valuestr(), _regex ) != NULL ;
         }
         else
         {
            return _re->PartialMatch( ele.valuestr() ) ;
         }
      default:
         return FALSE ;
      }
   }

   void _mthMatchOpNodeRegex::_evalEstimation ( const optCollectionStat *pCollectionStat,
                                                double &selectivity,
                                                UINT32 &cpuCost )
   {
      selectivity = OPT_MTH_OPTR_DEFAULT_SELECTIVITY ;
      cpuCost = _evalCPUCost() ;
   }
}



