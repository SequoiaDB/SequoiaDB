/******************************************************************************
 *
 * Name: common.c
 * Description: Common functions for sample programs
 *              This file does NOT include main function
 *
 ******************************************************************************/
#include "common.h"

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
         printf ( "Failed to create collection space %s, rc = %d" OSS_NEWLINE,
                  pCSName, rc ) ;
      }
      else
      {
         /* if we successfully created new collectionspace */
         printf ( "Collectionspace %s has been created" OSS_NEWLINE,
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
         printf ( "Failed to allocate memory for new string" OSS_NEWLINE ) ;
         return SDB_OOM ;
      }
      /* if the collection does not exist, we are going to create one */
      printf ( "Collection %s does not exist, creating a new collection"
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
         printf ( "Failed to get collectionspace %s, rc = %d" OSS_NEWLINE,
                  pStr, rc ) ;
      }
      else
      {
         rc = sdbCreateCollection ( collectionSpace, pTmp,
                                    collection ) ;
         if ( rc )
         {
           /* if we failed to create collection, we are going to display rc */
            printf ( "Failed to create new collection %s, rc = %d" OSS_NEWLINE,
                     pCollectionFullName, rc ) ;
         }
         else
         {
            /* if we successfully created collection */
            printf ( "Successfully created new collection %s" OSS_NEWLINE,
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
      return ;
   for ( ; count < listSize; ++count )
   {
      if ( !jsonToBson( objlist,"{firstName:\"John\",\
lastName:\"Smith\",age:50}" ))
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
   if( rc==SDB_DMS_EOC )
   {
      printf("Failed to display records.\n") ;
   }
}

void waiting ( int second )
{
   #if defined (_LINUX)
      sleep ( second ) ;
   #elif defined (_WINDOWS)
      Sleep ( second ) ;
   #endif	
}

