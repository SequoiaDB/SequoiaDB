/******************************************************************************
 *
 *@ Name : common.c
 *@ Description : Common functions for sample programs
 *                This file does NOT include main function
 *
 ******************************************************************************/
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>
#include "testcommon.h"

#define TAG_START_CHAR '['
#define TAG_END_CHAR   ']'
#define COMMENT_CHAR   '#'


/* display syntax error */
void displaySyntax ( CHAR *pCommand )
{
   printf ( "Syntax: %s <hostname> <servicename> <username> <password>"
            OSS_NEWLINE, pCommand ) ;
}

/* create record list */
void createRecordList ( bson *objlist, INT32 listSize )
{
   INT32 count = 0 ;
   if ( !objlist || 0 >= listSize )
      return ;
   for ( ; count < listSize; ++count )
   {
     if ( !jsonToBson ( objlist,"{firstName:\"John\",\
                                  lastName:\"Smith\",age:50}" ) )
      {
         printf ( "Failed to convert json to bson\n" ) ;
         continue ;
      }
      objlist ++ ;
   }
}


/* connect to a given database */
INT32 connectTo ( const CHAR *pHostName,
                  const CHAR *pServiceName,
                  const CHAR *pUsr,
                  const CHAR *pPasswd,
                  sdbConnectionHandle *connection )
{
   return sdbConnect ( pHostName, pServiceName, pUsr, pPasswd, connection ) ;
}

/* get collection space, if the collection does not exist it will try to create
 * one */
INT32 getCollectionSpace ( sdbConnectionHandle connection,
                           const CHAR *pCSName,
                           sdbCSHandle *collectionSpace )
{
   INT32 rc                    = SDB_OK ;
   rc = sdbGetCollectionSpace ( connection, pCSName, collectionSpace ) ;
   /* verify whether the collection space exists */
   if ( SDB_DMS_CS_NOTEXIST == rc )
   {
      /* if the collection space does not exist, we are going to create one */
      printf ( "Collectionspace %s does not exist, creating a new \
collectionspace" OSS_NEWLINE,
               pCSName ) ;
      rc = sdbCreateCollectionSpace ( connection, pCSName,
                                      SDB_PAGESIZE_DEFAULT, collectionSpace ) ;
      if ( rc )
      {
         /* if we failed to create new collectionspace */
         printf ( "Failed to create collection space %s, rc = %d"OSS_NEWLINE,
                  pCSName, rc ) ;
      }
      else
      {
         /* if we successfully created new collectionspace */
         printf ( "Collectionspace %s has been created"OSS_NEWLINE,
                  pCSName ) ;
      }
   }
   return rc ;
}

/* get a collection, if the collection does not exist, it will try to create
 * one */
INT32 getCollection ( sdbConnectionHandle connection,
                      const CHAR *pCollectionFullName,
                      sdbCollectionHandle *collection )
{
   INT32 rc                    = SDB_OK ;
   sdbCSHandle collectionSpace = 0 ;
   rc = sdbGetCollection ( connection, pCollectionFullName,
                           collection ) ;
   /* verify whether the collection exists */
   if ( ( SDB_DMS_NOTEXIST == rc ) ||
        ( SDB_DMS_CS_NOTEXIST == rc ) )
   {
      CHAR *pStr = strdup ( pCollectionFullName ) ;
      CHAR *pTmp = NULL ;
      if ( !pStr )
      {
         printf ( "Error: failed to allocate memory for new string"OSS_NEWLINE ) ;
         return SDB_OOM ;
      }
      /* if the collection does not exist, we are going to create one */
      printf ( "Info: collection %s does not exist, creating a new collection"
               OSS_NEWLINE,
               pCollectionFullName ) ;
      /* get collection space first */
      /* find . and replace to '\0' to splite colleciton space name */
      pTmp = strchr ( pStr, '.' ) ;
      if ( pTmp )
      {
         *pTmp = 0 ;
         pTmp = pTmp+1 ;
      }
      /* get the collection space */
      rc = getCollectionSpace ( connection, pStr, &collectionSpace ) ;
      if ( rc )
      {
         printf ( "Error: failed to get collectionspace %s, rc = %d"OSS_NEWLINE,
                  pStr, rc ) ;
      }
      else
      {
         rc = sdbCreateCollection ( collectionSpace, pTmp,
                                    collection ) ;
         if ( rc )
         {
           /* if we failed to create collection, we are going to display rc */
            printf ( "Error: failed to create new collection %s, rc = %d" OSS_NEWLINE,
                     pCollectionFullName, rc ) ;
         }
         else
         {
            /* if we successfully created collection */
            printf ( "Info: successfully created new collection %s" OSS_NEWLINE,
                     pCollectionFullName ) ;
         }
      }
      free ( pStr ) ;
   }
   sdbReleaseCS ( collectionSpace ) ;
   /* return any error code we received */
   return rc ;
}

/* insert record into collection */
INT32 insertRecord ( sdbCollectionHandle collection,
                     bson *obj )
{
   return sdbInsert ( collection, obj ) ;
}

/* bulkinsert records into collection */
INT32 bulkInsertRecords ( sdbCollectionHandle collection,
                          SINT32 flags,
                          bson **obj, SINT32 num )
{
   return sdbBulkInsert ( collection, flags, obj, num ) ;
}

/* delete records from collection */
INT32 deleteRecords ( sdbCollectionHandle collection,
                      bson *cond,
                      bson *hint )
{
   return sdbDelete ( collection, cond, hint ) ;
}

/* update records from collection */
INT32 updateRecords ( sdbCollectionHandle collection,
                      bson *rule,
                      bson *cond,
                      bson *hint )
{
   return sdbUpdate ( collection, rule, cond, hint ) ;
}

/* drop the collection space */
INT32 dropCollectionSpace ( sdbConnectionHandle cHandle,
                            const CHAR *pCollectionSpaceName )
{
   return sdbDropCollectionSpace ( cHandle,
                                   pCollectionSpaceName ) ;
}

INT32 query ( sdbCollectionHandle cHandle,
              bson *condition,
              bson *select,
              bson *orderBy,
              bson *hint,
              INT64 numToSkip,
              INT64 numToReturn,
              sdbCursorHandle *handle )
{
   return sdbQuery ( cHandle, condition, select, orderBy,
                     hint, numToSkip, numToReturn, handle ) ;
}

/* find indexes on a given collection */
INT32 getIndexes ( sdbCollectionHandle collection,
                   const CHAR *pIndexName,
                   sdbCursorHandle *handle )
{
   return sdbGetIndexes ( collection, pIndexName, handle ) ;
}

/* create index on a given collection */
INT32 createIndex ( sdbCollectionHandle collection,
                    bson *indexdef,
                    const CHAR *pIndexName,
                    BOOLEAN isUnique,
                    BOOLEAN isEnforced )
{
   return sdbCreateIndex ( collection, indexdef, pIndexName,
                           isUnique, isEnforced ) ;
}

/* truncate a given string, remove all space and tabs from head and tail */
/* returns the first non-space/tab character */
CHAR *truncateString ( CHAR *pString )
{
   CHAR *pPos = NULL ;
   CHAR *pPosEnd = NULL ;
   INT32 count = 0 ;
   if ( !pString )
      return NULL ;
   for ( count = strlen ( pString )-1; count >= 0; --count )
   {
      if ( ( 0x0D != pString[count] ) && ( 0x0A != pString[count] ) &&
           ( ' ' != pString[count] ) && ( '\t' != pString[count] ) )
      {
         if ( pPosEnd )
         {
            *pPosEnd = '\0' ;
         }
         break ;
      }
      pPosEnd = &pString[count] ;
   }
   /* return NULL if it's empty string */
   if ( 0 == count )
   {
      return NULL ;
   }
   for ( count = 0; count < strlen ( pString ); ++count )
   {
      if ( ( 0x0D != pString[count] ) && ( 0x0A != pString[count] ) &&
           ( ' ' != pString[count] ) && ( '\t' != pString[count] ) )
      {
         pPos = &pString[count] ;
         break ;
      }
   }
   return pPos ;
}

/* load tag string, tag string has form "[xxxx]" */
CHAR *loadTag ( CHAR *pString )
{
   CHAR *pPos = truncateString ( pString ) ;
   if ( !pPos )
      return NULL ;
   if ( ( TAG_START_CHAR == pPos[0] ) &&
        ( TAG_END_CHAR == pPos[strlen(pPos)-1] ) )
   {
      pPos[strlen(pPos)-1] = '\0' ;
      pPos++ ;
   }
   else
   {
      return NULL ;
   }
   return truncateString ( pPos ) ;
}

BOOLEAN isComment ( CHAR *pString )
{
   CHAR *pPos = truncateString ( pString ) ;
   if ( !pPos )
      return FALSE ;
   return COMMENT_CHAR == pPos[0] ;
}

BOOLEAN loadJSON ( CHAR *pString, bson *obj )
{
   return TRUE ;
}

INT32 fetchRecords ( sdbCollectionHandle collection,
                     bson *condition,
                     bson *selector,
                     bson *orderBy,
                     bson *hint,
                     INT64 skip,
                     INT64 numReturn )
{
   INT32 rc = SDB_OK ;
   INT32 count = 0 ;
   sdbCursorHandle cursor ;
   bson obj ;
   rc = sdbQuery ( collection, condition, selector, orderBy, hint,
                   skip, numReturn, &cursor ) ;
   if ( rc )
   {
      if ( SDB_DMS_EOC != rc )
      {
         printf ( "Failed to query from collection, rc = %d"
                  OSS_NEWLINE, rc ) ;
      }
      else
      {
         printf ( "No records can be read" OSS_NEWLINE ) ;
      }
      return rc ;
   }
   while ( TRUE )
   {
      bson_init ( &obj ) ;
      rc = sdbNext ( cursor, &obj ) ;
      if ( rc )
      {
         if ( SDB_DMS_EOC != rc )
         {
            printf ( "Failed to fetch next record from collection, rc = %d"
                     OSS_NEWLINE, rc ) ;
         }
         break ;
      }
      printf ( "Record Read [ %d ]: " OSS_NEWLINE, count ) ;
      bson_print ( &obj ) ;
      ++ count ;
   }
   sdbReleaseCursor ( cursor ) ;
   return rc ;
}
/* initialize the environment */
INT32 initEnv( const CHAR* host, const CHAR* server,
               const CHAR* user, const CHAR* passwd )
{
   sdbConnectionHandle connection    = 0 ;
   sdbCSHandle cs                    = 0 ;
   sdbCollectionHandle cl            = 0 ;
   sdbCursorHandle cursor            = 0 ;
   INT32 rc                          = SDB_OK ;
   INT32 count                       = 0;
   bson conf ;
   bson obj ;
   bson objList [ NUM_RECORD ] ;
   rc = sdbConnect ( host, server, user, passwd, &connection ) ;
   if( SDB_OK != rc )
      return rc ;
   rc = getCollectionSpace( connection, COLLECTION_SPACE_NAME, &cs ) ;
   if( SDB_OK != rc )
      return rc ;
   sdbReleaseCS( cs ) ;
   cs = 0 ;
   rc = sdbDropCollectionSpace ( connection, COLLECTION_SPACE_NAME ) ;
   if( SDB_OK != rc )
      return rc ;
   rc = sdbCreateCollectionSpace ( connection, COLLECTION_SPACE_NAME,
                                   SDB_PAGESIZE_4K, &cs );
   if( SDB_OK != rc )
      return rc ;
   bson_init( &conf ) ;
   bson_append_int ( &conf, "ReplSize", 0 ) ;
   bson_finish ( &conf );
   rc = sdbCreateCollection1 ( cs, COLLECTION_NAME, &conf, &cl ) ;
   bson_destroy ( &conf ) ;
   if( SDB_OK != rc )
      return rc ;

   sdbDisconnect ( connection ) ;
   sdbReleaseCursor ( cursor ) ;
   sdbReleaseCollection ( cl ) ;
   sdbReleaseCS( cs ) ;
   sdbReleaseConnection ( connection ) ;
   return rc ;
}

/* insert record into database */
void insertRecords( sdbCollectionHandle cl, INT32 num )
{
   INT32 rc                  = SDB_OK ;
   INT32 count              = 0;
   bson obj ;
   bson objList [ num ] ;
   createNameList ( &objList[0], num ) ;
   for ( count = 0; count < num; count++ )
   {
      rc = sdbInsert ( cl, &objList[count] ) ;
      if ( rc )
      {
         printf ( "Failed to insert record, rc = %d" OSS_NEWLINE, rc ) ;
      }
      bson_destroy ( &objList[count] ) ;
   }
}

/* insert record into database */
void insertRecords1( sdbCollectionHandle cl, INT32 num )
{
   INT32 rc                  = SDB_OK ;
   INT32 count              = 0;
   bson obj ;
   bson_init ( &obj );
   for ( count = 0; count < num; count++ )
   {
      bson_append_int ( &obj, "Id", count ) ;
      bson_append_int ( &obj, "age", rand()%100) ;
      bson_append_start_object ( &obj, "phone" );
      bson_append_int ( &obj, "0", count ) ;
      bson_append_int ( &obj, "1", count + 1 ) ;
      bson_append_finish_object ( &obj ) ;
      bson_append_string ( &obj, "str", "sequoiadb" ) ;
      rc = bson_finish ( &obj ) ;
      if ( rc )
      {
         printf ( "Failed to build bson.\n" ) ;
         return ;
      }
      rc = sdbInsert ( cl, &obj ) ;
      if ( rc )
      {
         printf ( "Failed to insert record, rc = %d" OSS_NEWLINE, rc ) ;
      }
      bson_destroy ( &obj ) ;
      bson_init ( &obj ) ;
   }
   bson_destroy ( &obj ) ;
}


/* create record list */
void createRecordList1 ( bson *objlist, INT32 listSize )
{
   INT32 count = 0 ;
   if ( !objlist || 0 >= listSize )
      return ;
   for ( ; count < listSize; ++count )
   {
      if ( !jsonToBson(objlist,"{firstName:\"John\",\
                                lastName:\"Smith\",age:50}" ))
      {
         printf ( "Failed to convert json to bson\n" ) ;
      }
      objlist ++ ;
   }
   count = 0 ;
   for ( ; count < listSize; ++count )
   {
      if ( !jsonToBson(objlist,"{firstName:\"Tom\",\
                                lastName:\"Johnson\",age:27}" ))
      {
         printf ( "Failed to convert json to bson\n" ) ;
      }
      objlist ++ ;
   }
   count = 0 ;
   for ( ; count < listSize; ++count )
   {
      if ( !jsonToBson(objlist,"{ \"姓名\" : \"李四\", \"年龄\" : 30,\
                           \"电话\" : [ \"18390378790\",\"13801598000\" ] }" ))
      {
         printf ( "Failed to convert json to bson\n" ) ;
      }
      objlist ++ ;
   }
}

/* create English record */
void createEnglishRecord ( bson *obj )
{
   const char *r ="{firstName:\"Sam\",\
                    lastName:\"Smith\",age:25,id:\"count\",\
                    address:{streetAddress: \"25 3ndStreet\",\
                    city:\"NewYork\",state:\"NY\",postalCode:\"10021\"},\
                    phoneNumber:[{type: \"home\",number:\"212555-1234\"}]}" ;
   bson_init ( obj ) ;
   jsonToBson ( obj, r ) ;
}
/* create Chinese record */
void createChineseRecord ( bson *obj )
{
   const char *r ="{ \"姓名\" : \"张三\", \"年龄\" : 25, \"id\" : 2001,\
                           \"电话\" : [ \"1808835242\",\"1835923246\" ] }" ;
   bson_init ( obj ) ;
   jsonToBson ( obj, r ) ;
}

void createNameList ( bson *objlist, INT32 listSize )
{
   INT32 count = 0 ;
   if ( !objlist || 0 >= listSize )
   {
      printf ( "Failed to create name list because of wrong argument\n" ) ;
      return ;
   }
   for ( ; count < listSize; ++count )
   {
      if ( !jsonToBson( objlist,"{firstName:\"John\",\
lastName:\"Smith\",age:50,\"Id\":999}" ))
      {
         printf ( "Failed to convert json to bson\n" ) ;
      }
      objlist ++ ;
   }
}

void displayRecord( sdbCursorHandle *cursor )
{
   int rc = SDB_OK ;
   bson obj ;
   bson_init(&obj);
   while( !( rc=sdbNext( *cursor, &obj ) ) )
   {
      bson_print( &obj ) ;
      bson_destroy( &obj ) ;
      bson_init( &obj );
   }
   bson_destroy( &obj ) ;
   if( rc != SDB_DMS_EOC )
   {
      printf("Failed to display records.\n") ;
   }
   else
   {
      printf("All the records have been display.\n") ;
   }
}

/* genRecord */
char *stringGen ( char *pString, int len )
{
   if ( len < 0 )
      return NULL ;
   pString[len] = 0 ;
   while ( len > 0 )
   {
      int num = rand() % 16 ;
      pString[--len]=num<10?'0'+num:'a'+num-10 ;
   }
   return pString ;
}
void getTime ( bson_timestamp_t *t )
{
   struct timeval tv ;
   if ( !t )
      return ;
   if ( -1 == gettimeofday ( &tv, NULL ) )
   {
      t->i = 0 ;
      t->t = 0 ;
   }
   else
   {
      t->t = tv.tv_sec ;
      t->i = tv.tv_usec ;
   }
}
void recordAppendHistory ( bson *obj, int numEle )
{
   char buffer[128] ;
   char b[64] ;
   bson_timestamp_t t ;
   int total = numEle ;
   while ( numEle > 0 )
   {
      memset ( b, 0, sizeof(b) ) ;
      sprintf ( b, "%d", total - numEle ) ;
      bson_append_start_object ( obj, b ) ;
      bson_append_string ( obj, "ip", stringGen ( buffer, 8 ) ) ;
      getTime ( &t ) ;
      bson_append_timestamp ( obj, "timestamp", &t ) ;
      bson_append_finish_object ( obj ) ;
      --numEle ;
   }
}
void recordGen ( bson *obj, char *pMacBuffer, long serialNum )
{
   bson_timestamp_t t ;
   char buffer[128] ;
   if ( !obj )
      return ;
   stringGen ( pMacBuffer, 12 ) ;
   bson_init ( obj ) ;
   bson_append_int ( obj, "age", rand()%100 ) ;
   bson_append_int ( obj, "id", serialNum ) ;
   bson_append_string ( obj, "mac", pMacBuffer ) ;
   bson_append_string ( obj, "model", stringGen ( buffer, 16 ) ) ;
   bson_append_string ( obj, "version", stringGen ( buffer, rand()%64 ) ) ;
   getTime ( &t ) ;
   bson_append_timestamp ( obj, "lastUpdate", &t ) ;
   bson_append_start_array ( obj, "history" ) ;
   recordAppendHistory ( obj, 1 ) ;
   bson_append_finish_array ( obj ) ;
   bson_finish ( obj ) ;
}
int insertToDB ( sdbConnectionHandle *sdb, const char *clFullName, long numToInsert )
{
   int rc = SDB_OK ;
   sdbCollectionHandle cl = 0 ;
   bson obj ;
   char macBuffer [ 32 ] ;
   long sNo = 0 ;
   rc = getCollection ( *sdb, clFullName, &cl  ) ;
   if ( rc )
   {
      printf ( "Failed to get collection %s, rc = %d\n",
               COLLECTION_NAME, rc ) ;
      return rc ;
   }
   while ( numToInsert > 0 )
   {
      recordGen ( &obj, macBuffer, sNo ) ;
      rc = sdbInsert ( cl, &obj ) ;
      bson_destroy ( &obj ) ;
      if ( rc )
      {
         if ( SDB_IXM_DUP_KEY == rc )
         {
            continue ;
         }
         else
         {
            printf ( "Failed to insert into collection, rc = %d\n",
                     rc ) ;
            return rc ;
         }
      }
      -- numToInsert ;
      ++ sNo ;
   }
   return rc ;
}
int genRecord ( sdbConnectionHandle *sdb, const char* clFullName, long num )
{
   int rc = 0 ;
   srand(time(NULL)) ;
   rc = insertToDB ( sdb, clFullName, num ) ;
   if ( rc )
   {
      goto error ;
   }
done :
   return rc ;
error :
   goto done ;
}

/* get the acount of record in cursor */
long getRecordNum ( sdbCursorHandle cursor )
{
   int rc = SDB_OK ;
   long num = 0 ;
   bson obj ;
   bson_init ( &obj ) ;
   while ( !( rc = sdbNext( cursor, &obj ) ) )
   {
      num++ ;
      bson_destroy ( &obj ) ;
      bson_init ( &obj ) ;
   }
   bson_destroy ( &obj ) ;
   return num ;
}

/*******************************************************************************
*@Description : Give your module name add head[sdbtest_] and tail[pid].
*@Parameter   : modName : your module name[type:cons CHAR *]
*@Modify List :
*               2014-7-15   xiaojun Hu   Init
*******************************************************************************/
void getUniqueName( const CHAR *modName, CHAR *pBuffer )
{
   const CHAR *uniqName = "sdbtest_" ;
   pid_t pid ;
   pid = getpid() ;
   sprintf( pBuffer, "%s%s_%d", uniqName, modName, (unsigned int)pid ) ;
}

BOOLEAN isCluster( sdbConnectionHandle db )
{
   INT32 rc = SDB_OK ;
   sdbCursorHandle cur ;

   rc = sdbListReplicaGroups( db, &cur ) ;
   if ( SDB_OK != rc )
   {
      return FALSE ;
   }
   return TRUE ;
}

INT32 isTranOn( sdbConnectionHandle db, BOOLEAN *flag )
{
   INT32 rc = SDB_OK ;
   INT32 tmpRC = SDB_OK ;
   sdbCollectionHandle cl = 0 ;
   const CHAR *pTranTmpCSName = "tran_tmp" ;
   const CHAR *pTranTmpCLFullName = "tran_tmp.tram_tmp" ;
   BOOLEAN tranOn = FALSE ;
   bson obj ;
   bson_init( &obj ) ;
   bson_finish( &obj ) ;
   
   rc = getCollection( db, pTranTmpCLFullName, &cl ) ;
   if ( rc )
   {
      goto error ;
   }
   rc = sdbTransactionBegin( db ) ;
   if ( rc )
   {
      goto error ;
   }
   tranOn = TRUE ;
   rc = sdbInsert( cl, &obj ) ;
   if ( SDB_DPS_TRANS_DIABLED == rc )
   {
      *flag = FALSE ;
      rc = SDB_OK ;
      goto done ;
   }
   else if ( SDB_OK == rc )
   {
      *flag = TRUE ;
      goto done ;
   }
   else
   {
      goto error ;
   }
   
final:
   bson_destroy( &obj ) ;
   if ( TRUE == tranOn )
   {
      tmpRC = sdbTransactionRollback( db ) ;
      if ( tmpRC && SDB_OK == rc )
      {
         rc = tmpRC ;
      } 
   }
   tmpRC = sdbDropCollectionSpace( db, pTranTmpCSName ) ;
   if ( tmpRC && SDB_OK == rc )
   {
      rc = tmpRC ;
      goto done ;
   }
done:
   return rc ;
error:
   goto done ;
}

INT32 gettid()
{
#if defined(_AIX)
   return (INT32)pthread_self() ;
#else
   return (INT32)syscall(SYS_gettid) ;
#endif
}

INT32 bson_compare(const CHAR *pStr, const bson *b)
{
   INT32 rc  = 0 ;
   CHAR *p = NULL ;
   INT32 bufferSize = bson_sprint_length ( b ) ;
   INT32 strLen = strlen( pStr ) ;
   if (strLen > bufferSize)
      return 1;
   p = (CHAR*)malloc(bufferSize) ;
   if ( !p )
      return -2;
   bson_sprint ( p, bufferSize, b ) ;
   rc = strncmp( pStr, p, bufferSize ) ;
   free ( p ) ;
   return rc == 0 ? 0 : ( rc > 0 ? 1 : -1 );
}

