#ifndef SDB_CL__H
#define SDB_CL__H

#include <mysql/psi/mysql_thread.h>
#include "include/client.hpp"
#include "sdb_def.h"
#include "sdb_conn_ptr.h"

class sdb_conn_auto_ptr ;
class sdb_cl
{
public:

   sdb_cl() ;

   ~sdb_cl() ;

   int init( sdb_conn *connection,
             char *cs, char *cl,
             bool create = FALSE,
             const bson::BSONObj &options = sdbclient::_sdbStaticObject ) ;

   int re_init( bool create = FALSE,
                const bson::BSONObj &options = sdbclient::_sdbStaticObject ) ;

   int begin_transaction() ;

   int commit_transaction() ;

   int rollback_transaction() ;

   bool is_transaction() ;

   char *get_cs_name() ;

   char *get_cl_name() ;

   int query( const bson::BSONObj &condition = sdbclient::_sdbStaticObject,
              const bson::BSONObj &selected  = sdbclient::_sdbStaticObject,
              const bson::BSONObj &orderBy   = sdbclient::_sdbStaticObject,
              const bson::BSONObj &hint      = sdbclient::_sdbStaticObject,
              INT64 numToSkip          = 0,
              INT64 numToReturn        = -1,
              INT32 flags              = QUERY_WITH_RETURNDATA ) ;

   int query_one( bson::BSONObj &obj,
                  const bson::BSONObj &condition = sdbclient::_sdbStaticObject,
                  const bson::BSONObj &selected  = sdbclient::_sdbStaticObject,
                  const bson::BSONObj &orderBy   = sdbclient::_sdbStaticObject,
                  const bson::BSONObj &hint      = sdbclient::_sdbStaticObject,
                  INT64 numToSkip          = 0,
                  INT32 flags              = QUERY_WITH_RETURNDATA ) ;

   int current( bson::BSONObj &obj ) ;

   int next( bson::BSONObj &obj ) ;

   int insert( bson::BSONObj &obj ) ;

   int update( const bson::BSONObj &rule,
               const bson::BSONObj &condition = sdbclient::_sdbStaticObject,
               const bson::BSONObj &hint      = sdbclient::_sdbStaticObject,
               INT32 flag = 0 ) ;

   int del( const bson::BSONObj &condition = sdbclient::_sdbStaticObject,
            const bson::BSONObj &hint      = sdbclient::_sdbStaticObject ) ;

   int create_index( const bson::BSONObj &indexDef,
                     const CHAR *pName,
                     BOOLEAN isUnique,
                     BOOLEAN isEnforced ) ;

   int drop_index( const char *pName ) ;

   int truncate() ;

   void close() ; // close cursor

   my_thread_id get_tid() ;

   int drop() ;


private:

   int check_connect( int rc ) ;

private:
   sdb_conn                                  *p_conn ;
   sdbclient::sdbCollection                  cl ;
   sdbclient::sdbCursor                      cursor ;
   char                                      cs_name[CS_NAME_MAX_SIZE + 1] ;
   char                                      cl_name[CL_NAME_MAX_SIZE + 1] ;
} ;
#endif