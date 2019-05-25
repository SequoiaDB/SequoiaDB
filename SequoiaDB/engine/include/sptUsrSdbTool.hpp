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

   Source File Name = sptUsrSdbTool.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          18/08/2014  XJH Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef SPT_USR_SDBTOOL_HPP__
#define SPT_USR_SDBTOOL_HPP__

#include "sptApi.hpp"
#include "pmdDef.hpp"
#include "utilNodeOpr.hpp"
#include <string>
#include <vector>

using namespace std ;

namespace engine
{

   struct _sdbToolListParam
   {
      vector< string >     _svcnames ;
      INT32                _typeFilter ;
      INT32                _modeFilter ;
      INT32                _roleFilter ;
      BOOLEAN              _showAlone ;
      BOOLEAN              _expand ;

      _sdbToolListParam()
      {
         _typeFilter    = SDB_TYPE_DB ;
         _modeFilter    = RUN_MODE_RUN ;
         _roleFilter    = -1 ;
         _showAlone     = FALSE ;
         _expand        = FALSE ;
      }
   } ;

   enum SPT_MATCH_PRED
   {
      SPT_MATCH_AND,
      SPT_MATCH_OR,
      SPT_MATCH_NOT
   } ;

   /*
      _sptUsrSdbTool define
   */
   class _sptUsrSdbTool : public SDBObject
   {
   JS_DECLARE_CLASS( _sptUsrSdbTool )

   public:
      _sptUsrSdbTool() ;
      virtual ~_sptUsrSdbTool() ;

   public:

      INT32 construct( const _sptArguments &arg,
                       _sptReturnVal &rval,
                       bson::BSONObj &detail ) ;

      static INT32 help( const _sptArguments &arg,
                         _sptReturnVal &rval,
                         bson::BSONObj &detail ) ;
      /*
         static functions
      */

      static INT32 listNodes( const _sptArguments &arg,
                              _sptReturnVal &rval,
                              bson::BSONObj &detail ) ;

   protected:
      static INT32 _parseListParam( const bson::BSONObj &option,
                                    _sdbToolListParam &param,
                                    bson::BSONObj &detail ) ;
      static bson::BSONObj _nodeInfo2Bson( const utilNodeInfo &info,
                                           const bson::BSONObj &conf ) ;

      static bson::BSONObj _getConfObj( const CHAR *rootPath,
                                        const CHAR *localPath,
                                        const CHAR *svcname,
                                        INT32 type ) ;

      static BOOLEAN _match( const bson::BSONObj &obj,
                             const bson::BSONObj &filter,
                             SPT_MATCH_PRED pred ) ;

   private:

   } ;

}

#endif // SPT_USR_SDBTOOL_HPP__

