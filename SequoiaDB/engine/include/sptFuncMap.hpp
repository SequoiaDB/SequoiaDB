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

   Source File Name = sptFuncMap.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          31/03/2014  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef SPT_FUNCMAP_HPP_
#define SPT_FUNCMAP_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "sptInvokeDef.hpp"
#include "sptSPDef.hpp"
#include <map>
#include <string>
#include <set>

namespace engine
{
   using namespace JS_INVOKER ;
   using namespace std ;

   struct _sptFuncInfo
   {
      JS_INVOKER::MEMBER_FUNC    _pFunc ;
      UINT32                     _attr ;

      _sptFuncInfo( JS_INVOKER::MEMBER_FUNC func = NULL,
                    UINT32 attr = SPT_FUNC_DEFAULT )
      {
         _pFunc = func ;
         _attr = attr ;
      }
   } ;
   typedef _sptFuncInfo  sptFuncInfo ;

   /*
      _sptFuncMap define
   */
   class _sptFuncMap : public SDBObject
   {
   public:
      _sptFuncMap()
      :_construct(NULL),
       _destruct(NULL),
       _resolve(NULL)
      {
      }

      virtual ~_sptFuncMap()
      {
         _construct = NULL ;
         _destruct = NULL ;
         _resolve = NULL ;
         _normal.clear() ;
      }
   public:
      typedef std::map<std::string, sptFuncInfo>
              NORMAL_FUNCS ;
   public:
      BOOLEAN isMemberFunc( const CHAR *funcName ) const
      {
         return NULL == funcName ?
                FALSE : 0 < _normal.count( funcName ) ;
      }

      JS_INVOKER::MEMBER_FUNC
      getMemberFunc( const CHAR *funcName ) const
      {
         JS_INVOKER::MEMBER_FUNC func = NULL ;
         if ( NULL != funcName )
         {
            NORMAL_FUNCS::const_iterator itr =
                        _normal.find( funcName ) ;
            if ( _normal.end() != itr )
            {
               func = itr->second._pFunc ;
            }
         }

         return func ;
      }

      const NORMAL_FUNCS &getMemberFuncs()const
      {
         return _normal ;
      }

      void getMemberFuncNames( set<string> &setFuncs,
                               BOOLEAN showHide = FALSE ) const
      {
         NORMAL_FUNCS::const_iterator itr = _normal.begin() ;
         while( itr != _normal.end() )
         {
            if ( showHide || ( itr->second._attr & SPT_PROP_ENUMERATE ) )
            {
               setFuncs.insert( itr->first ) ;
            }
            ++itr ;
         }
      }

      const NORMAL_FUNCS &getStaticFuncs() const
      {
         return _static ;
      }

      void getStaticFuncNames( set<string> &setFuncs,
                               BOOLEAN showHide = FALSE ) const
      {
         NORMAL_FUNCS::const_iterator itr = _static.begin() ;
         while( itr != _static.end() )
         {
            if ( showHide || ( itr->second._attr & SPT_PROP_ENUMERATE ) )
            {
               setFuncs.insert( itr->first ) ;
            }
            ++itr ;
         }
      }

      BOOLEAN addMemberFunc( const CHAR *name,
                             JS_INVOKER::MEMBER_FUNC f,
                             UINT32 attr = SPT_FUNC_DEFAULT )
      {
         return ( NULL != name && NULL != f ) ?
                _normal.insert( std::make_pair( name, sptFuncInfo( f, attr ) ) ).second :
                FALSE ;
      }

      BOOLEAN addStaticFunc( const CHAR *name,
                             JS_INVOKER::MEMBER_FUNC f,
                             UINT32 attr = SPT_FUNC_DEFAULT )
      {
         return ( NULL != name && NULL != f ) ?
                _static.insert( std::make_pair( name, sptFuncInfo( f, attr ) ) ).second :
                FALSE ;
      }

      void setConstructor( JS_INVOKER::MEMBER_FUNC f )
      {
         _construct = f ;
      }

      void setDestructor( JS_INVOKER::DESTRUCT_FUNC f )
      {
         _destruct = f ;
      }

      void setResolver( JS_INVOKER::RESLOVE_FUNC f )
      {
         _resolve = f ;
      }

      JS_INVOKER::MEMBER_FUNC getConstructor() const
      {
         return _construct ;
      }

      JS_INVOKER::DESTRUCT_FUNC getDestructor() const
      {
         return _destruct ;
      }

      JS_INVOKER::RESLOVE_FUNC getResolver() const
      {
         return _resolve ;
      }
   private:

      NORMAL_FUNCS _normal ;
      NORMAL_FUNCS _static ;
      JS_INVOKER::MEMBER_FUNC _construct ;
      JS_INVOKER::DESTRUCT_FUNC _destruct ;
      JS_INVOKER::RESLOVE_FUNC _resolve ;
   } ;
   typedef class _sptFuncMap sptFuncMap ;
}

#endif // SPT_FUNCMAP_HPP_

