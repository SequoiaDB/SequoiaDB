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

   Source File Name = utilSdb.hpp

   Descriptive Name =

   When/how to use: parse Data util

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          05/02/2014  JW  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef UTIL_SDB_HPP_
#define UTIL_SDB_HPP_

#include "core.hpp"
#include "pd.hpp"
#include "pdTrace.hpp"
#include "utilTrace.hpp"
#include "ossSignal.hpp"
#include "ossPrimitiveFileOp.hpp"
#include "ossStackDump.hpp"
#if defined (_LINUX)
#include <execinfo.h>
#endif
#include <vector>
#include <boost/program_options.hpp>
#include <boost/program_options/parsers.hpp>
namespace po = boost::program_options ;

typedef INT32 (*sdb_templet_cb) ( void *pData ) ;

#define OPTION_HOSTNAME          "hostname"
#define OPTION_SVCNAME           "svcname"
#define OPTION_USER              "user"
#define OPTION_PASSWORD          "password"

#define APPENDARGINT( obj, key, cmd, explain, require, min, max, defaultValue )\
{\
   rc = obj.appendArgInt( key, cmd, explain, require, min, max, defaultValue ) ;\
   if ( rc )\
   {\
      goto error ;\
   }\
}

#define APPENDARGCHAR( obj, key, cmd, explain, require, defaultValue )\
{\
   rc = obj.appendArgChar( key, cmd, explain, require, defaultValue ) ;\
   if ( rc )\
   {\
      goto error ;\
   }\
}

#define APPENDARGSTRING( obj, key, cmd, explain, require, size, defaultValue )\
{\
   rc = obj.appendArgString( key, cmd, explain, require, size, defaultValue ) ;\
   if ( rc )\
   {\
      goto error ;\
   }\
}

#define APPENDARGBOOL( obj, key, cmd, explain, require, defaultValue )\
{\
   rc = obj.appendArgBool( key, cmd, explain, require, defaultValue ) ;\
   if ( rc )\
   {\
      goto error ;\
   }\
}

#define APPENDARGSWITCH( obj, key, cmd, explain, require, switchs, switchNum, defaultValue )\
{\
   rc = obj.appendArgSwitch( key, cmd, explain, require, switchs, switchNum, defaultValue ) ;\
   if ( rc )\
   {\
      goto error ;\
   }\
}

struct util_sdb_settings
{
   sdb_templet_cb on_init ;
   sdb_templet_cb on_preparation ;
   sdb_templet_cb on_main ;
   sdb_templet_cb on_end ;
} ;

enum UTIL_VAR_TYPE
{
   UTIL_VAR_INT = 0,
   UTIL_VAR_BOOL,
   UTIL_VAR_STRING,
   UTIL_VAR_SWITCH,
   UTIL_VAR_CHAR
} ;

class utilSdbTemplet : public SDBObject
{
private:
   struct util_var : public SDBObject
   {
      CHAR  varChar ;
      INT32 varInt ;
      INT32 maxInt ;
      INT32 minInt ;
      INT32 maxStringSize ;
      INT32 switchNum ;
      BOOLEAN varBool ;
      BOOLEAN require ;
      BOOLEAN stringIsMy ;
      UTIL_VAR_TYPE varType ;
      CHAR *pVarString ;
      const CHAR *pKey ;
      const CHAR *pCmd ;
      const CHAR *pExplain ;
      const CHAR **ppSwitch ;

      util_var() : varChar(0),
                   varInt(0),
                   maxInt(0),
                   minInt(0),
                   maxStringSize(-1),
                   switchNum(0),
                   varBool(TRUE),
                   require(TRUE),
                   stringIsMy(FALSE),
                   varType(UTIL_VAR_INT),
                   pVarString(NULL),
                   pKey(NULL),
                   pCmd(NULL),
                   pExplain(NULL),
                   ppSwitch(NULL)
      {
      }
   } ;

#if defined (_LINUX)
   INT32 _utilSetupSignalHandler() ;
#endif
   void _initArg ( po::options_description &desc ) ;
   void _displayArg ( po::options_description &desc ) ;
   INT32 _resolveArgument ( po::options_description &desc,
                            INT32 argc, CHAR **argv,
                            const CHAR *pPName ) ;
   void *_findKey( const CHAR *pKey ) ;
private:
   void *_pData ;
   util_sdb_settings _setting ;
   std::vector<util_var *> _argList ;
public:
   utilSdbTemplet() ;
   ~utilSdbTemplet() ;
   INT32 appendArgInt( const CHAR *pKey,
                       const CHAR *pCmd,
                       const CHAR *pExplain,
                       BOOLEAN require,
                       INT32 minInt,
                       INT32 maxInt,
                       INT32 defaultInt = 0 ) ;
   INT32 appendArgChar( const CHAR *pKey,
                        const CHAR *pCmd,
                        const CHAR *pExplain,
                        BOOLEAN require,
                        CHAR defaultChar = 0 ) ;
   INT32 appendArgBool( const CHAR *pKey,
                        const CHAR *pCmd,
                        const CHAR *pExplain,
                        BOOLEAN require,
                        BOOLEAN defaultBool = TRUE ) ;
   INT32 appendArgString( const CHAR *pKey,
                          const CHAR *pCmd,
                          const CHAR *pExplain,
                          BOOLEAN require,
                          INT32 maxStringSize = -1,
                          CHAR *pDefaultString = NULL ) ;
   INT32 appendArgSwitch( const CHAR *pKey,
                          const CHAR *pCmd,
                          const CHAR *pExplain,
                          BOOLEAN require,
                          const CHAR **ppSwitch,
                          INT32 switchNum,
                          INT32 defaultValue = 0 ) ;
   INT32 getArgInt( const CHAR *pKey, INT32 *pVarValue ) ;
   INT32 getArgChar( const CHAR *pKey, CHAR *pVarValue ) ;
   INT32 getArgBool( const CHAR *pKey, BOOLEAN *pVarValue ) ;
   INT32 getArgString( const CHAR *pKey, CHAR **ppVarValue ) ;
   INT32 getArgSwitch( const CHAR *pKey, INT32 *pVarValue ) ;
   INT32 init( util_sdb_settings &setting, void *pData ) ;
   INT32 run( INT32 argc, CHAR **argv, const CHAR *pPName ) ;
} ;

#endif
