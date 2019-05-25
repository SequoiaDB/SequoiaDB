/*******************************************************************************

   Copyright (C) 2012-2014 SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = pd.hpp

   Descriptive Name = Problem Determination Header

   When/how to use: this program may be used on binary and text-formatted
   versions of PD component. This file contains declare of PD functions.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef PD_HPP_
#define PD_HPP_

#include "pdErr.hpp"

#define PD_DFT_FILE_NUM             (20)
#define PD_DFT_FILE_SZ              (100)
#define PD_DFT_DIAGLOG              "sdbdiag.log"
#define PD_DFT_AUDIT                "sdbaudit.log"

#define PD_LOG_STRINGMAX            ( 4096 )

#ifdef SDB_CLIENT
   #ifdef _DEBUG
      #include <assert.h>
      #define SDB_ASSERT(cond,str)  assert(cond)
   #else
      #define SDB_ASSERT(cond,str)  do{ if( !(cond)) {} } while ( 0 )
   #endif // _DEBUG
#else
   #ifdef _DEBUG
      #define SDB_ASSERT(cond,str)  \
      do { \
         if( !(cond) ) { pdassert(str,__FUNC__,__FILE__,__LINE__) ; } \
      } while ( 0 )
   #else
      #define SDB_ASSERT(cond,str)  do{ if( !(cond)) {} } while ( 0 )
   #endif // _DEBUG
#endif // SDB_CLIENT

#define SDB_VALIDATE_GOTOERROR(cond, ret, str) \
   do { \
      if( !(cond) ) { \
         pdLog(PDERROR, __FUNC__, __FILE__, __LINE__, str) ; \
         rc=ret ; \
         goto error ; \
         } \
      } while ( 0 )

#define PD_LOG(level, fmt, ...) \
   do { \
      if ( getPDLevel() >= level ) \
      { \
         pdLog(level, __FUNC__, __FILE__, __LINE__, fmt, ##__VA_ARGS__); \
      } \
   }while (0)

#define PD_LOG_MSG(level, fmt, ...) \
   do { \
      if ( level <= PDERROR ) \
      { \
         _pmdEDUCB *cb = pmdGetThreadEDUCB() ; \
         if ( cb ) \
         { \
            cb->printInfo ( EDU_INFO_ERROR, fmt, ##__VA_ARGS__ ) ; \
         } \
      } \
      if ( getPDLevel() >= level ) \
      { \
         pdLog(level, __FUNC__, __FILE__, __LINE__, fmt, ##__VA_ARGS__); \
      } \
   } while ( 0 )

#define PD_LOG_RAW(level, msg) \
   do { \
      if ( getPDLevel() >= level ) \
      { \
         pdLogRaw(level, msg); \
      } \
   }while (0)

#define PD_CHECK(cond, retCode, gotoLabel, level, fmt, ...) \
   do {                                                     \
      if ( !(cond) )                                        \
      {                                                     \
         rc = (retCode) ;                                   \
         PD_LOG ( (level), fmt, ##__VA_ARGS__) ;            \
         goto gotoLabel ;                                   \
      }                                                     \
   } while ( 0 )                                            \


#define PD_RC_CHECK(rc, level, fmt, ...)                    \
   do {                                                     \
      PD_CHECK ( (SDB_OK == (rc)), (rc), error, (level),    \
                 fmt, ##__VA_ARGS__) ;                      \
   } while ( 0 )                                            \


enum PDLEVEL
{
   PDSEVERE = 0,
   PDERROR,
   PDEVENT,
   PDWARNING,
   PDINFO,
   PDDEBUG
} ;

PDLEVEL& getPDLevel() ;
PDLEVEL  setPDLevel( PDLEVEL newLevel ) ;
const CHAR* getDialogName() ;
const CHAR* getDialogPath() ;
const CHAR* getPDLevelDesp ( PDLEVEL level ) ;

void setDiagFileNum( INT32 fileMaxNum ) ;
void setAuditFileNum( INT32 fileMaxNum ) ;

void sdbEnablePD( const CHAR *pdPathOrFile,
                  INT32 fileMaxNum = PD_DFT_FILE_NUM,
                  UINT32 fileMaxSize = PD_DFT_FILE_SZ ) ;
void sdbDisablePD() ;

BOOLEAN sdbIsPDEnabled() ;


#ifdef _DEBUG
void pdassert( const CHAR* string, const CHAR* func,
               const CHAR* file, UINT32 line) ;
void pdcheck( const CHAR* string, const CHAR* func,
              const CHAR* file, UINT32 line) ;
#else
#define pdassert(str1,str2,str3,str4)
#define pdcheck(str1,str2,str3,str4)
#endif

/*
   pdLog function define
*/
void pdLog( PDLEVEL level, const CHAR* func, const CHAR* file,
            UINT32 line, const CHAR* format, ...) ;

void pdLog( PDLEVEL level, const CHAR* func, const CHAR* file,
            UINT32 line, const std::string &message ) ;

void pdLogRaw( PDLEVEL level, const CHAR* pData ) ;

/*
   PD Audit Define
*/
enum AUDIT_TYPE
{
   AUDIT_ACCESS         = 1,     /// User login, logout
   AUDIT_CLUSTER        = 2,     /// Group, Domain, Node operations
   AUDIT_SYSTEM         = 3,     /// System Info

   AUDIT_DML            = 8,     /// Insert, Update, Delete operation, but
   AUDIT_DDL            = 9,     /// Create/Drop Collection, and so on
   AUDIT_DCL            = 10,    /// Create User, Drop User and so on
   AUDIT_DQL            = 11,    /// Query, Explain

   AUDIT_DELETE         = 20,    /// The detail of delete records
   AUDIT_UPDATE         = 21,    /// The detail of update records
   AUDIT_INSERT         = 22,    /// The detail of insert records

   AUDIT_OTHER          = 255    /// Other
} ;

#define AUDIT_MASK_ACCESS     0x00000001
#define AUDIT_MASK_CLUSTER    0x00000002
#define AUDIT_MASK_SYSTEM     0x00000004
#define AUDIT_MASK_DML        0x00000010
#define AUDIT_MASK_DDL        0x00000020
#define AUDIT_MASK_DCL        0x00000040
#define AUDIT_MASK_DQL        0x00000080
#define AUDIT_MASK_DELETE     0x00000100
#define AUDIT_MASK_UPDATE     0x00000200
#define AUDIT_MASK_INSERT     0x00000400
#define AUDIT_MASK_OTHER      0x00001000

#define AUDIT_MASK_DEFAULT    ( AUDIT_MASK_SYSTEM | AUDIT_MASK_DDL | AUDIT_MASK_DCL )
#define AUDIT_MASK_DFT_STR    "SYSTEM|DDL|DCL"
#define AUDIT_MASK_ALL        0xFFFFFFFF

/*
   AUDIT_OBJ_TYPE define
*/
enum AUDIT_OBJ_TYPE
{
   AUDIT_OBJ_SYSTEM        = 0,
   AUDIT_OBJ_CS,
   AUDIT_OBJ_CL,
   AUDIT_OBJ_GROUP,
   AUDIT_OBJ_NODE,
   AUDIT_OBJ_DOMAIN,
   AUDIT_OBJ_PROCEDURE,
   AUDIT_OBJ_FILE,
   AUDIT_OBJ_SESSION,
   AUDIT_OBJ_USER,

   AUDIT_OBJ_MAX
} ;
const CHAR* pdAuditObjType2String( AUDIT_OBJ_TYPE objtype ) ;

#define PD_AUDIT(type, username, ipAddr, port, action, objtype, objname, result, fmt, ...) \
   do { \
      if ( getCurAuditMask() & pdAuditType2Mask( type ) ) \
      { \
         try { \
            pdAudit(type, username, ipAddr, port, action, objtype, \
                    objname, result, __FUNC__, __FILE__, __LINE__, \
                    fmt, ##__VA_ARGS__); \
         } catch( ... ) {} \
      } \
   }while( 0 )

#define PD_AUDIT_SYSTEM(action,objtype,objname,result,fmt, ...) \
   do { \
      PD_AUDIT(AUDIT_SYSTEM,"","",0,action,objtype,objname,result,fmt,##__VA_ARGS__) ; \
   }while( 0 )

#define PD_AUDIT_OP(type,optype,objtype,objname,result,fmt, ...)\
   do { \
         const CHAR *pUserName = "" ; \
         const CHAR *fromIP = "" ; \
         UINT16 fromPort = 0 ;\
         _pmdEDUCB *cb = pmdGetThreadEDUCB() ; \
         if ( cb ) \
         { \
            pUserName = cb->getUserName() ; \
            ISession *pSession = cb->getSession() ; \
            if ( pSession && pSession->getClient() ) \
            { \
               fromIP = pSession->getClient()->getFromIPAddr() ; \
               fromPort = pSession->getClient()->getFromPort() ; \
            } \
         } \
      PD_AUDIT(type,pUserName,fromIP,fromPort,msgType2String((MSG_TYPE)optype),\
               objtype,objname,result,fmt,##__VA_ARGS__) ; \
   }while( 0 )

#define PD_AUDIT_OP_WITHNAME(type,opname,objtype,objname,result,fmt, ...)\
   do { \
         const CHAR *pUserName = "" ; \
         const CHAR *fromIP = "" ; \
         UINT16 fromPort = 0 ;\
         _pmdEDUCB *cb = pmdGetThreadEDUCB() ; \
         if ( cb ) \
         { \
            pUserName = cb->getUserName() ; \
            ISession *pSession = cb->getSession() ; \
            if ( pSession && pSession->getClient() ) \
            { \
               fromIP = pSession->getClient()->getFromIPAddr() ; \
               fromPort = pSession->getClient()->getFromPort() ; \
            } \
         } \
      PD_AUDIT(type,pUserName,fromIP,fromPort,opname,\
               objtype,objname,result,fmt,##__VA_ARGS__) ; \
   }while( 0 )

#define PD_AUDIT_COMMAND(type,commandstr,objtype,objname,result,fmt, ... ) \
   do { \
         if ( AUDIT_DDL == type && SDB_OK != result ) { break ; } \
         const CHAR *pUserName = "" ; \
         const CHAR *fromIP = "" ; \
         UINT16 fromPort = 0 ;\
         _pmdEDUCB *cb = pmdGetThreadEDUCB() ; \
         if ( cb ) \
         { \
            pUserName = cb->getUserName() ; \
            ISession *pSession = cb->getSession() ; \
            if ( pSession && pSession->getClient() ) \
            { \
               fromIP = pSession->getClient()->getFromIPAddr() ; \
               fromPort = pSession->getClient()->getFromPort() ; \
            } \
         } \
         CHAR tmp[ 100 ] = { 0 } ;\
         ossSnprintf( tmp, sizeof(tmp)-1, "%s(%s)", \
                      msgType2String(MSG_BS_QUERY_REQ, TRUE), commandstr ) ;\
         PD_AUDIT(type,pUserName,fromIP,fromPort,tmp,objtype,objname,result,fmt, ##__VA_ARGS__) ;\
   }while( 0 )

UINT32 pdAuditType2Mask( AUDIT_TYPE auditType ) ;
const CHAR* pdGetAuditTypeDesp( AUDIT_TYPE auditType ) ;
INT32  pdString2AuditMask( const CHAR *pStr, UINT32 &mask ) ;

UINT32&     getAuditMask() ;
UINT32      setAuditMask( UINT32 newMask ) ;
UINT32&     getCurAuditMask() ;
UINT32      setCurAuditMask( UINT32 newMask ) ;
void        initCurAuditMask( UINT32 newMask ) ;

const CHAR* getAuditName() ;
const CHAR* getAuditPath() ;

void sdbEnableAudit( const CHAR *pdPathOrFile,
                     INT32 fileMaxNum = PD_DFT_FILE_NUM,
                     UINT32 fileMaxSize = PD_DFT_FILE_SZ ) ;
void sdbDisableAudit() ;

BOOLEAN sdbIsAuditEnabled() ;

void pdAudit( AUDIT_TYPE type, const CHAR *pUserName,
              const CHAR* ipAddr, UINT16 port,
              const CHAR *pAction, AUDIT_OBJ_TYPE objType,
              const CHAR *pObjName, INT32 result,
              const CHAR* func, const CHAR* file,
              UINT32 line, const CHAR* format, ...) ;

void pdAudit( AUDIT_TYPE type, const CHAR *pUserName,
              const CHAR* ipAddr, UINT16 port,
              const CHAR *pAction, AUDIT_OBJ_TYPE objType,
              const CHAR *pObjName, INT32 result,
              const CHAR* func, const CHAR* file,
              UINT32 line, const std::string &message ) ;

void pdAuditRaw( AUDIT_TYPE type, const CHAR* pData ) ;

#endif // PD_HPP_

