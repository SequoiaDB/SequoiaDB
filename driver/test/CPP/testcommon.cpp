#include "testcommon.hpp"
#include <string>
#include <iostream>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

using namespace std;

#define TAG_START_CHAR '['
#define TAG_END_CHAR   ']'
#define COMMENT_CHAR   '#'

/* connect to a given database */
INT32 connectTo ( const CHAR *pHostName,
                  const CHAR *pServiceName,
                  const CHAR *pUser,
                  const CHAR *pPasswd,
                  sdb &connection )
{
   return connection.connect ( pHostName, pServiceName, pUser, pPasswd ) ;
}

/* get cs and cl, if not exist it will try to create one */
INT32 getCollectionSpace ( sdb &connection,
                           const CHAR *pCSName,
                           sdbCollectionSpace &collectionSpace )
{
   INT32 rc                    = SDB_OK ;
   rc = connection.getCollectionSpace ( pCSName, collectionSpace ) ;
   /* verify whether the collection space exists */
   if ( SDB_DMS_CS_NOTEXIST == rc )
   {
      /* if the collection space does not exist, we are going to create one */
      printf ( "Collectionspace %s does not exist, creating a new \
collectionspace" OSS_NEWLINE,
               pCSName ) ;
      rc = connection.createCollectionSpace ( pCSName, SDB_PAGESIZE_DEFAULT,
                                              collectionSpace ) ;
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
INT32 getCollection ( sdbCollectionSpace &cs,
                      const CHAR *pCollectionName,
                      sdbCollection &cl )
{
   INT32 rc                     = SDB_OK ;
   rc = cs.getCollection( pCollectionName, cl ) ;
   if ( SDB_DMS_NOTEXIST == rc )
   {
      rc = cs.createCollection ( pCollectionName, cl ) ;
      if ( rc )
      {
         goto error ;
      }
   }
done :
   return rc ;
error :
   goto done ;
}

/* get cl from connection, if the collection does not exist, it will try to create
 * one */
INT32 getCollection1 ( sdb &connection,
                       const CHAR *pCollectionFullName,
                       sdbCollection &collection )
{
   INT32 rc                    = SDB_OK ;
   sdbCollectionSpace collectionSpace ;
   rc = connection.getCollection ( pCollectionFullName, collection ) ;
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
      rc = connection.getCollectionSpace ( pStr, collectionSpace ) ;
      if ( rc )
      {
         printf ( "Failed to get collectionspace %s, rc = %d" OSS_NEWLINE,
                  pStr, rc ) ;
      }
      else
      {
         rc = collectionSpace.createCollection ( pTmp, collection ) ;
         if ( rc )
         {
           /* if we failed to create collection, we are going to display rc */
            printf ( "Failed to create new collection %s, rc = %d" OSS_NEWLINE,
                     pCollectionFullName, rc ) ;
         }
         else
         {
            /* if we successfully created collection */
            printf ( "Successfully created new collection %s\n",
                     pCollectionFullName ) ;
         }
      }
      free ( pStr ) ;
   }
   else if ( rc )
   {
      /* for any other error, we display the error code */
      printf ( "Failed to get collection %s, rc = %d" OSS_NEWLINE,
               pCollectionFullName, rc ) ;
   }
   /* return any error code we received */
   return rc ;
}

/* insert record into collection */
INT32 insertRecord ( sdbCollection &collection,
                     BSONObj &obj )
{
   return collection.insert ( obj ) ;
}

/* delete records from collection */
INT32 deleteRecords ( sdbCollection &collection,
                      BSONObj &cond,
                      BSONObj &hint )
{
   return collection.del ( cond, hint ) ;
}

/* update records from collection */
INT32 updateRecords ( sdbCollection &collection,
                      BSONObj &rule,
                      BSONObj &cond,
                      BSONObj &hint )
{
   return collection.update ( rule, cond, hint ) ;
}

/* find indexes on a given collection */
INT32 getIndexes ( sdbCollection &collection,
                   const CHAR *pIndexName,
                   sdbCursor &handle )
{
   return collection.getIndexes ( handle, pIndexName ) ;
}

/* create index on a given collection */
INT32 createIndex ( sdbCollection &collection,
                    BSONObj &indexdef,
                    const CHAR *pIndexName,
                    BOOLEAN isUnique,
                    BOOLEAN isEnforced )
{
   return collection.createIndex ( indexdef, pIndexName, isUnique, isEnforced ) ;
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

BOOLEAN loadJSON ( CHAR *pString, BSONObj &obj )
{
   return TRUE ;
}

INT32 fetchRecords ( sdbCollection &collection,
                     BSONObj &condition,
                     BSONObj &selector,
                     BSONObj &orderBy,
                     BSONObj &hint,
                     INT64 skip,
                     INT64 numReturn )
{
   INT32 rc = SDB_OK ;
   INT32 count = 0 ;
   sdbCursor cursor ;
   BSONObj obj ;
   rc = collection.query ( cursor, condition, selector, orderBy, hint, skip,
                           numReturn ) ;
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
      rc = cursor.next ( obj ) ;
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
      cout << obj.toString() << endl ;
      ++ count ;
   }
   return rc ;
}

string toJson( const BSONObj &b )
{
   return b.toString( true, true ) ;
}

/* initialize environment */
int initEnv( void )
{
   sdb connection ;
   sdbCollectionSpace cs ;
   sdbCollection cl ;
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;

   BSONObj conf ;
   BSONObjBuilder bob ;
   BSONObj indexDef ;
   int rc = SDB_OK ;
   rc = connectTo ( pHostName, pPort, pUsr, pPasswd, connection ) ;
   if ( rc )
   {
      cout << "Failed to connect to database" << endl ;
      goto error ;
   }
   rc = getCollectionSpace ( connection, COLLECTION_SPACE_NAME, cs  ) ;
   if ( rc )
   {
      cout << "Failed to get cs" << endl ;
      goto error ;
   }
   rc =  connection.dropCollectionSpace ( COLLECTION_SPACE_NAME ) ;
   if ( rc )
   {
      cout << "Failed to drop cs" << endl ;
      goto error ;
   }
   rc = connection.createCollectionSpace ( COLLECTION_SPACE_NAME, SDB_PAGESIZE_4K, cs ) ;
   if ( rc )
   {
      cout << "Failed to create cs" << endl ;
      goto error ;
   }
   bob.append ( "ReplSize", 0 ) ;
   conf = bob.done() ;
   rc = cs.createCollection ( COLLECTION_NAME, conf, cl ) ;
   if ( rc )
   {
      cout << "Failed to create cl" << endl ;
      goto error ;
   }
   indexDef = BSON( "age" << 1 ) ;
   rc = cl.createIndex ( indexDef, INDEX_NAME, false, false ) ;
   if ( rc )
   {
      cout << "Failed to create index" << endl ;
      goto error ;
   }
   connection.disconnect() ;
done :
   return rc ;
error :
   goto done ;
}

/* create En and Cn record */
void createEnglishRecord ( BSONObj &obj )
{
   const char *r ="{firstName:\"Sam\",\
                    lastName:\"Smith\",age:25,id:\"count\",\
                    address:{streetAddress: \"25 3ndStreet\",\
                    city:\"NewYork\",state:\"NY\",postalCode:\"10021\"},\
                    phoneNumber:[{type: \"home\",number:\"212555-1234\"}]}" ;
   fromjson ( r, obj ) ;
}
void createChineseRecord ( BSONObj &obj )
{
   const char *r ="{ \"姓名\" : \"张三\", \"年龄\" : 25, \"id\" : 2001,\
                           \"电话\" : [ \"1808835242\",\"1835923246\" ] }" ;
   fromjson ( r, obj ) ;
}

/* create name and record list */
void createNameList ( vector<BSONObj> &objlist, INT32 listSize )
{
   INT32 count = 0 ;
   BSONObj obj ;
   if ( 0 >= listSize )
      return ;
   for ( ; count < listSize; ++count )
   {
      obj = BSON ( "firstName" << "John" <<
                   "lastName" << "Smith" <<
                   "age" << 50 ) ;
      objlist.push_back ( obj ) ;
   }
}
void createRecordList ( vector<BSONObj> objlist, INT32 listSize )
{
   INT32 count = 0 ;
   BSONObj obj ;
   if ( 0 >= listSize )
      return ;
   for ( ; count < listSize; ++count )
   {
      switch( count%3 )
      {
         case 0:
            obj = BSON ( "firstName" << "John" <<
                         "lastName" << "Smith" <<
                         "age" << 50 ) ;
            break ;
         case 1:
            obj = BSON ( "firstName" << "Tom" <<
                         "lastName" << "Johnson" <<
                         "age" << 27 ) ;
            break ;
         case 2:
            fromjson ( "{ \"姓名\" : \"李四\", \"年龄\" : 30,\
                       \"电话\" : [ \"18390378790\",\"13801598000\" ] }",
                       obj ) ;
            break ;
      }
      objlist.push_back ( obj ) ;
   }
}

/* insert some record into database */
INT32 insertRecords ( sdbCollection &cl, SINT64 num )
{
   INT32 rc                       = SDB_OK ;
   INT32 count                    = 0;
   vector<BSONObj> objList ;
   BSONObj obj ;
   createNameList( objList, num ) ;
   for ( count = 0; count < num; count++ )
   {
      rc = cl.insert( objList[count] ) ;
      if ( rc )
      {
         printf( "Failed to insert record, rc = %d" OSS_NEWLINE, rc ) ;
      }
   }
done :
   return rc ;
error :
   goto done ;
}

/* gen records and insert into database */
CHAR *stringGen ( CHAR *pString, INT32 len )
{
   if ( len < 0 )
      return NULL ;
   pString[len] = 0 ;
   while ( len > 0 )
   {
      INT32 num = rand() % 16 ;
      pString[--len]=num<10?'0'+num:'a'+num-10 ;
   }
   return pString ;
}
void recordAppendHistory ( BSONObj &obj, INT32 numEle )
{
   BSONObjBuilder ob ;
   CHAR buffer[128] ;
   CHAR b[64] ;
   INT32 total = numEle ;
   while ( numEle > 0 )
   {
      memset ( b, 0, sizeof(b) ) ;
      sprintf ( b, "%d", total - numEle ) ;
      ob.append ( "ip", stringGen ( buffer, 8 ) ) ;
      ob.appendTimestamp ( "timestamp", time(NULL)*1000, 0 ) ;
      --numEle ;
   }
   obj = ob.obj () ;
}
void recordGen ( BSONObj &obj, CHAR *pMacBuffer, SINT64 serialNum )
{
   BSONObjBuilder ob ;
   BSONObjBuilder b ;
   BSONObj subObj ;
   CHAR buffer[128] ;
   stringGen ( pMacBuffer, 12 ) ;
   ob.append ( "age", rand()%100 ) ;
   ob.appendIntOrLL ( "id", serialNum ) ;
   ob.append ( "mac", pMacBuffer ) ;
   ob.append ( "model", stringGen( buffer, 16 ) ) ;
   ob.append ( "version", stringGen ( buffer, rand()%64 ) ) ;
   ob.appendTimestamp ( "lastUpdata", time(NULL)*1000, 0 ) ;
   ob.appendArray ( "history", subObj ) ;
   obj = ob.obj() ;
}
INT32 insertToDB ( sdbCollection &cl, SINT64 numToInsert )
{
   INT32 rc = SDB_OK ;
   BSONObj obj ;
   CHAR macBuffer [ 32 ] ;
   SINT64 sNo = 0 ;
   while ( numToInsert > 0 )
   {
      recordGen ( obj, macBuffer, sNo ) ;
      rc = cl.insert ( obj ) ;
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
INT32 genRecord ( sdbCollection &cl, SINT64 num )
{
   INT32 rc = 0 ;
   srand(time(NULL)) ;
   rc = insertToDB ( cl, num ) ;
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
SINT64 getRecordNum ( sdbCursor &cursor )
{
   INT32 rc = SDB_OK ;
   INT64 num = 0 ;
   BSONObj obj ;
   while ( !( rc = cursor.next ( obj ) ) )
   {
      num++ ;
   }
   return num ;
}

/* display record */
void displayRecord( sdbCursor &cursor )
{
   INT32 rc = SDB_OK ;
   BSONObj obj ;
   while( !( rc=cursor.next( obj ) ) )
   {
      cout << toJson ( obj ) << endl ;
   }
   if( rc!=SDB_DMS_EOC )
   {
      cout<<"Failed to display records, rc = "<<rc<<endl ;
   }
}


/******************************************************
 * The follow functions are use in cluster environment
 ******************************************************/


/* add group */
INT32 addGroup ( sdb &connection, const CHAR *newGroupName, sdbReplicaGroup &shard )
{
   INT32 rc = SDB_OK ;
   if ( strlen ( newGroupName ) > 127 )
   {
      rc = SDB_INVALIDARG ;
      printf ( "Group name is too long" OSS_NEWLINE ) ;
      goto error ;
   }
   rc = connection.createReplicaGroup ( newGroupName, shard ) ;
   if ( rc )
   {
      printf ( "Failed to create group, rc = %d" OSS_NEWLINE, rc ) ;
      goto error ;
   }
done :
   return rc ;
error :
   goto done ;
}

/* get host name */
INT32 getHostName ( CHAR* buffer, INT32 len )
{
   INT32 rc = SDB_OK ;
   rc = gethostname ( buffer, len ) ;
   if ( rc )
   {
      printf ( "Failed to get host name, rc = %d" OSS_NEWLINE, rc ) ;
      goto error ;
   }
done:
   return rc ;
error:
   goto done ;
}

/* get data path */
void getDataPath ( CHAR* buffer, INT32 len, const CHAR *dp1, const CHAR *dp2 )
{
   INT32 namelen = 256 ;
   CHAR host[namelen] ;
   getHostName ( host, namelen ) ;
   strcpy ( buffer,strncmp( host, "ubuntu-dev1", strlen("ubuntu-dev1") )
            == 0 ? dp1 : dp2 ) ;
}

INT32 delete_space( string &dest, const CHAR *src )
{
   if ( 0 == src )
      return SDB_INVALIDARG ;
   while( *src != '\0' )
   {
      if (*src != ' ' )
         dest = dest + *src ;
      src++ ;
   }
   return SDB_OK ;
}

/*******************************************************************************
*@Description : Give your module name add head[sdbtest_] and tail[pid].
*@Parameter   : modName : your module name[type:cons CHAR *]
*@Modify List :
*               2014-7-15   xiaojun Hu   Init
*******************************************************************************/
void getUniqueName( const CHAR *modName, CHAR *getName, INT32 len )
{
   const CHAR *uniqName = "sdbtest_" ;
   pid_t pid ;
   pid = getpid() ;
   snprintf( getName, len, "%s%s_%d", uniqName, modName, (unsigned int)pid ) ;
}

BOOLEAN isCluster( sdb &db )
{
   INT32 rc = SDB_OK ;
   sdbCursor cursor ;

   rc = db.listReplicaGroups( cursor ) ;
   if ( SDB_OK == rc )
   {
      return TRUE ;
   }
   return FALSE ;
}
