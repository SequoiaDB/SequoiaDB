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

   Source File Name = omToolCmdBase.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/09/2019  HJW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef OMTOOL_CMD_BASE_HPP_
#define OMTOOL_CMD_BASE_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "ossUtil.hpp"
#include "omToolOptions.hpp"
#include <iostream>
#include <string>

using namespace std ;

namespace omTool
{
   #define DECLARE_OMTOOL_CMD_AUTO_REGISTER()                           \
      public:                                                           \
         static omToolCmdBase* newThis() ;  \

   #define IMPLEMENT_OMTOOL_CMD_AUTO_REGISTER(theClass)                       \
      omToolCmdBase* theClass::newThis()          \
      {                                                                       \
         return SDB_OSS_NEW theClass() ;        \
      }                                                                       \
      _omToolCmdAssit theClass##Assit( theClass::newThis ) ;

   class omToolCmdBase : public SDBObject
   {
   public:
      omToolCmdBase() ;

      virtual ~omToolCmdBase() ;

      void setOptions( omToolOptions *options ) ;

      virtual INT32 doCommand() = 0 ;

      virtual const CHAR *name() = 0 ;

   protected:
      void _setErrorMsg( const CHAR *pMsg ) ;
      void _setErrorMsg( const string &msg ) ;

   protected:
      omToolOptions *_options ;

   private:
      string _errMsg ;
   } ;

   typedef omToolCmdBase* (*OMTOOL_NEW_FUNC)() ;

   class _omToolCmdAssit : public SDBObject
   {
   public:
      _omToolCmdAssit( OMTOOL_NEW_FUNC ) ;
      virtual ~_omToolCmdAssit() ;
   } ;

   struct _classComp
   {
      bool operator()( const CHAR *lhs, const CHAR *rhs ) const
      {
         return ossStrcasecmp( lhs, rhs ) < 0 ;
      }
   } ;

   typedef map<const CHAR*, OMTOOL_NEW_FUNC, _classComp> MAP_CMD ;

#if defined (_WINDOWS)
   typedef MAP_CMD::iterator MAP_CMD_IT ;
#else
   typedef map<const CHAR*, OMTOOL_NEW_FUNC>::iterator MAP_CMD_IT ;
#endif // _WINDOWS

   class _omToolCmdBuilder : public SDBObject
   {
   friend class _omToolCmdAssit ;

   public:
      _omToolCmdBuilder () ;
      ~_omToolCmdBuilder () ;

   public:
      omToolCmdBase *create( const CHAR *command ) ;

      void release( omToolCmdBase *&pCommand ) ;

      INT32 _register( const CHAR *name, OMTOOL_NEW_FUNC pFunc ) ;

      OMTOOL_NEW_FUNC _find( const CHAR *name ) ;

   private:
      MAP_CMD _cmdMap ;
   } ;

   _omToolCmdBuilder* getOmToolCmdBuilder() ;
}

#endif /* OMT_CMD_BASE_HPP_ */