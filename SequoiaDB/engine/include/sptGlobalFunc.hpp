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

   Source File Name = sptGlobalFunc.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          31/03/2014  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef SPT_BLOBALFUNC_HPP_
#define SPT_BLOBALFUNC_HPP_

#include "sptApi.hpp"

namespace engine
{
   class _sptGlobalFunc : public SDBObject
   {
   JS_DECLARE_CLASS( _sptGlobalFunc )

   public:
      _sptGlobalFunc() {}
      virtual ~_sptGlobalFunc() {}

   public:
      static INT32 getLastErrorMsg( const _sptArguments &arg,
                                    _sptReturnVal &rval,
                                    bson::BSONObj &detail ) ;

      static INT32 setLastErrorMsg( const _sptArguments &arg,
                                    _sptReturnVal &rval,
                                    bson::BSONObj &detail ) ;

      static INT32 getLastError( const _sptArguments &arg,
                                 _sptReturnVal &rval,
                                 bson::BSONObj &detail ) ;

      static INT32 setLastError( const _sptArguments &arg,
                                 _sptReturnVal &rval,
                                 bson::BSONObj &detail ) ;

      static INT32 setLastErrorObj( const _sptArguments &arg,
                                    _sptReturnVal &rval,
                                    bson::BSONObj &detail ) ;

      static INT32 getLastErrorObj( const _sptArguments &arg,
                                    _sptReturnVal &rval,
                                    bson::BSONObj &detail ) ;

      static INT32 print( const _sptArguments &arg,
                          _sptReturnVal &rval,
                          bson::BSONObj &detail ) ;

      static INT32 sleep( const _sptArguments &arg,
                          _sptReturnVal &rval,
                          bson::BSONObj &detail ) ;

      static INT32 traceFmt( const _sptArguments &arg,
                             _sptReturnVal &rval,
                             bson::BSONObj &detail ) ;

      static INT32 globalHelp( const _sptArguments &arg,
                               _sptReturnVal &rval,
                               bson::BSONObj &detail ) ;

      static INT32 displayMethod( const _sptArguments &arg,
                                  _sptReturnVal &rval,
                                  BSONObj &detail ) ;

      static INT32 displayManual( const _sptArguments &arg,
                                  _sptReturnVal &rval,
                                  BSONObj &detail ) ;

      static INT32 showClass( const _sptArguments &arg,
                              _sptReturnVal &rval,
                              bson::BSONObj &detail ) ;

      static INT32 showClassfull( const _sptArguments &arg,
                                  _sptReturnVal &rval,
                                  bson::BSONObj &detail ) ;

      static INT32 forceGC( const _sptArguments &arg,
                            _sptReturnVal &rval,
                            bson::BSONObj &detail ) ;

      static INT32 importJSFile( const _sptArguments &arg,
                                 _sptReturnVal &rval,
                                 bson::BSONObj &detail ) ;

      static INT32 importJSFileOnce( const _sptArguments &arg,
                                     _sptReturnVal &rval,
                                     bson::BSONObj &detail ) ;

      static INT32 getExePath( const _sptArguments &arg,
                               _sptReturnVal &rval,
                               bson::BSONObj &detail ) ;

      static INT32 getSelfPath( const _sptArguments &arg,
                                _sptReturnVal &rval,
                                bson::BSONObj &detail ) ;

      static INT32 getRootPath( const _sptArguments &arg,
                                _sptReturnVal &rval,
                                bson::BSONObj &detail ) ;

      static INT32 catPath( const _sptArguments &arg,
                            _sptReturnVal &rval,
                            bson::BSONObj &detail ) ;

   protected:
      static INT32 _showClassInner( const _sptArguments &arg,
                                    const string &className,
                                    BOOLEAN showHide,
                                    _sptReturnVal &rval,
                                    bson::BSONObj &detail ) ;
      static INT32 _evalFile( BOOLEAN importOnce,
                              const _sptArguments &arg,
                              _sptReturnVal &rval,
                              bson::BSONObj &detail ) ;
   } ;
   typedef class _sptGlobalFunc sptGlobalFunc ;
}

#endif

