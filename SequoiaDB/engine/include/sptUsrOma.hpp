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

   Source File Name = sptUsrOma.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          18/08/2014  XJH Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef SPT_USROMA_HPP__
#define SPT_USROMA_HPP__

#include "sptApi.hpp"
#include "sptUsrOmaAssit.hpp"

namespace engine
{

   /*
      _sptUsrOma define
   */
   class _sptUsrOma : public SDBObject
   {
   JS_DECLARE_CLASS( _sptUsrOma )

   public:
      _sptUsrOma() ;
      virtual ~_sptUsrOma() ;

   public:
      INT32 construct( const _sptArguments &arg,
                       _sptReturnVal &rval,
                       bson::BSONObj &detail) ;

      INT32 destruct() ;

      INT32 toString( const _sptArguments &arg,
                      _sptReturnVal &rval,
                      bson::BSONObj &detail ) ;

      static INT32 help( const _sptArguments &arg,
                         _sptReturnVal &rval,
                         bson::BSONObj &detail ) ;

      INT32 createCoord( const _sptArguments &arg,
                         _sptReturnVal &rval,
                         bson::BSONObj &detail ) ;

      INT32 removeCoord( const _sptArguments &arg,
                         _sptReturnVal &rval,
                         bson::BSONObj &detail ) ;

      INT32 createData( const _sptArguments &arg,
                        _sptReturnVal &rval,
                        bson::BSONObj &detail ) ;

      INT32 removeData( const _sptArguments &arg,
                        _sptReturnVal &rval,
                        bson::BSONObj &detail ) ;

      INT32 createOM( const _sptArguments &arg,
                      _sptReturnVal &rval,
                      bson::BSONObj &detail ) ;

      INT32 removeOM( const _sptArguments &arg,
                      _sptReturnVal &rval,
                      bson::BSONObj &detail ) ;

      INT32 startNode( const _sptArguments &arg,
                       _sptReturnVal &rval,
                       bson::BSONObj &detail ) ;

      INT32 stopNode( const _sptArguments &arg,
                      _sptReturnVal &rval,
                      bson::BSONObj &detail ) ;

      INT32 runCommand( const _sptArguments &arg,
                        _sptReturnVal &rval,
                        bson::BSONObj &detail ) ;

      INT32 close( const _sptArguments &arg,
                   _sptReturnVal &rval,
                   bson::BSONObj &detail ) ;

      /*
         static functions
      */
      static INT32 getOmaInstallInfo( const _sptArguments &arg,
                                      _sptReturnVal &rval,
                                      bson::BSONObj &detail ) ;

      static INT32 getOmaInstallFile( const _sptArguments &arg,
                                      _sptReturnVal &rval,
                                      bson::BSONObj &detail ) ;

      static INT32 getOmaConfigFile( const _sptArguments &arg,
                                     _sptReturnVal &rval,
                                     bson::BSONObj &detail ) ;

      static INT32 getOmaConfigs( const _sptArguments &arg,
                                  _sptReturnVal &rval,
                                  bson::BSONObj &detail ) ;

      static INT32 setOmaConfigs( const _sptArguments &arg,
                                  _sptReturnVal &rval,
                                  bson::BSONObj &detail ) ;

      static INT32 getAOmaSvcName( const _sptArguments &arg,
                                   _sptReturnVal &rval,
                                   bson::BSONObj &detail ) ;

      static INT32 addAOmaSvcName( const _sptArguments &arg,
                                   _sptReturnVal &rval,
                                   bson::BSONObj &detail ) ;

      static INT32 delAOmaSvcName( const _sptArguments &arg,
                                   _sptReturnVal &rval,
                                   bson::BSONObj &detail ) ;

      static INT32 start( const _sptArguments &arg,
                          _sptReturnVal &rval,
                          bson::BSONObj &detail ) ;

   protected:
      INT32 _createNode( const _sptArguments &arg,
                         _sptReturnVal &rval,
                         bson::BSONObj &detail,
                         const CHAR *pNodeStr ) ;

      INT32 _removeNode( const _sptArguments &arg,
                         _sptReturnVal &rval,
                         bson::BSONObj &detail,
                         const CHAR *pNodeStr ) ;

      INT32 _mergeArg( const _sptArguments &arg,
                       bson::BSONObj &detail,
                       string &command,
                       bson::BSONObj *mergeObj ) ;

      static INT32 _startSdbcm ( list<const CHAR*> &argv,
                                 OSSPID &pid,
                                 BOOLEAN asProc ) ;

   private:
      sptUsrOmaAssit          _assit ;
      string                  _hostname ;
      string                  _svcname ;

   } ;

}

#endif // SPT_USROMA_HPP__

