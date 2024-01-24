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

   Source File Name = omagentCmdBase.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/30/2014  TZB Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef OMAGENT_CMD_BASE_HPP_
#define OMAGENT_CMD_BASE_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "ossTypes.h"
#include "ossUtil.h"
#include "../bson/bson.h"
#include "ossMem.h"
#include "ossSocket.hpp"
#include "omagentDef.hpp"
#include "omagentMsgDef.hpp"
#include "omagent.hpp"
#include "sptScope.hpp"
#include <map>
#include <string>

using namespace bson ;
using namespace std ;

namespace engine
{
   #define DECLARE_OACMD_AUTO_REGISTER()                       \
      public:                                                  \
         static _omaCommand *newThis () ;                      \

   #define IMPLEMENT_OACMD_AUTO_REGISTER(theClass)             \
      _omaCommand* theClass::newThis ()                        \
      {                                                        \
         return SDB_OSS_NEW theClass() ;                       \
      }                                                        \
      _omaCmdAssit theClass##Assit ( theClass::newThis ) ;     \

   /*
      _omaCommand
   */
   class _omaCommand : public SDBObject
   {
      public:
         _omaCommand () ;
         virtual ~_omaCommand () ;

         virtual BOOLEAN needCheckBusiness() const { return TRUE ; }

      public:

         INT32 addUserDefineVar( const CHAR* pVariable ) ;

         virtual const CHAR * name () = 0 ;

         virtual INT32 prime () ; 

         virtual INT32 init ( const CHAR *pInstallInfo ) ;

         virtual INT32 doit ( BSONObj &retObj ) ;

         virtual INT32 final( BSONObj &rval, BSONObj &retObj ) ;

         virtual INT32 setJsFile ( const CHAR *fileName ) ;
         
         virtual INT32 addJsFile ( const CHAR *filename,
                                   const CHAR *bus = NULL,
                                   const CHAR *sys = NULL,
                                   const CHAR *env = NULL,
                                   const CHAR *other = NULL ) ;

         virtual INT32 getExcuteJsContent ( string &content ) ;

      public:
         // async task command callback
         virtual INT32 setRuningStatus( const BSONObj& itemInfo,
                                        BSONObj& taskInfo ) ;

         virtual INT32 convertResult( const BSONObj& retObj,
                                      BSONObj& newRetObj ) ;

      protected:
         CHAR                            _jsFileName[ OSS_MAX_PATHSIZE + 1 ] ;
         string                          _jsFileArgs ;
         CHAR                            *_fileBuff ;
         UINT32                          _buffSize ;
         UINT32                          _readSize ;
         vector<BSONObj>                 _hosts ;
         string                          _content ;
         vector<string>                  _userDefineVar ;
         vector< pair<string, string> >  _jsFiles ;
         _sptScope            *_scope ;
   } ;

   typedef _omaCommand* (*OA_NEW_FUNC) () ;

   /*
      _omaCmdAssit
   */
   class _omaCmdAssit : public SDBObject
   {
      public:
         _omaCmdAssit ( OA_NEW_FUNC ) ;
         virtual ~_omaCmdAssit () ;
   } ;

   struct _classComp
   {
      bool operator()( const CHAR *lhs, const CHAR *rhs ) const
      {
         return ossStrcmp( lhs, rhs ) < 0 ;
      }
   } ;

   typedef map<const CHAR*, OA_NEW_FUNC, _classComp>     MAP_OACMD ;
#if defined (_WINDOWS)
   typedef MAP_OACMD::iterator                           MAP_OACMD_IT ;
#else
   typedef map<const CHAR*, OA_NEW_FUNC>::iterator       MAP_OACMD_IT ;
#endif // _WINDOWS

   /*
      _omaCmdBuilder
   */
   class _omaCmdBuilder : public SDBObject
   {
      friend class _omaCmdAssit ;

      public:
         _omaCmdBuilder () ;
         ~_omaCmdBuilder () ;

      public:
         _omaCommand *create ( const CHAR *command ) ;

         void release ( _omaCommand *&pCommand ) ;

         INT32 _register ( const CHAR *name, OA_NEW_FUNC pFunc ) ;

         OA_NEW_FUNC _find ( const CHAR * name ) ;

      private:
         MAP_OACMD _cmdMap ;
   } ;

   /*
      get omagent command builder
   */
   _omaCmdBuilder* getOmaCmdBuilder() ;

   class _omaTaskMgr ;


} // namespace engine


#endif // OMAGENT_CMD_BASE_HPP_