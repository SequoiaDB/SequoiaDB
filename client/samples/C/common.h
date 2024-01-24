/******************************************************************************
 *
 * Name: common.h
 * Description: Common functions declaration for sample programs
 *
 ******************************************************************************/
#ifndef COMMON_H__
#define COMMON_H__
#include "client.h"

SDB_EXTERN_C_START

#define CHECK_RC( rc, msg )                                            \
do {                                                                   \
   if ( rc )                                                           \
   {                                                                   \
      printf( "%s[%d]: %s, rc = %d\n", __FILE__, __LINE__, msg, rc ) ; \
      goto error ;                                                     \
   }                                                                   \
} while ( 0 ) ;

/* display syntax error */
void displaySyntax ( CHAR *pCommand );

/* create record list */
void createRecordList ( bson *objlist, INT32 listSize ) ;

/* connect to a given database */
INT32 connectTo ( const CHAR *pHostName,
                  const CHAR *pServiceName,
                  const CHAR *pUsr,
                  const CHAR *pPasswd,
                  sdbConnectionHandle *connection ) ;

/* get collection space, if the collection does not exist it will try to create
 * one */
INT32 getCollectionSpace ( sdbConnectionHandle connection,
                           const CHAR *pCSName,
                           sdbCSHandle *collectionSpace ) ;

/* drop the collection space */
INT32 dropCollectionSpace ( sdbConnectionHandle cHandle,
                            const CHAR *pCollectionSpaceName ) ;

/* get a collection, if the collection does not exist, it will try to create
 * one */
INT32 getCollection ( sdbConnectionHandle connection,
                      const CHAR *pCollectionFullName,
                      sdbCollectionHandle *collection ) ;

/* insert record into collection */
INT32 insertRecord ( sdbCollectionHandle collection,
                     bson *obj ) ;

INT32 bulkInsertRecords ( sdbCollectionHandle collection,
                          SINT32 flags, bson **obj, SINT32 num ) ;

CHAR *loadTag ( CHAR *pString ) ;

BOOLEAN isComment ( CHAR *pString ) ;

BOOLEAN loadJSON ( CHAR *pString, bson *obj ) ;

INT32 query ( sdbCollectionHandle cHandle,
              bson *condition,
              bson *select,
              bson *orderBy,
              bson *hint,
              INT64 numToSkip,
              INT64 numToReturn,
              sdbCursorHandle *handle ) ;

INT32 getIndexes ( sdbCollectionHandle collection,
                   const CHAR *pIndexName,
                   sdbCursorHandle *handle ) ;

INT32 deleteRecords ( sdbCollectionHandle collection,
                      bson *cond,
                      bson *hint ) ;

INT32 createIndex ( sdbCollectionHandle collection,
                    bson *indexdef,
                    const CHAR *pIndexName,
                    BOOLEAN isUnique,
                    BOOLEAN isEnforced ) ;

INT32 fetchRecords ( sdbCollectionHandle collection,
                     bson *condition,
                     bson *selector,
                     bson *orderBy,
                     bson *hint,
                     INT64 skip,
                     INT64 numReturn ) ;

/* create record list */
void createRecordList1 ( bson *objlist, INT32 listSize ) ;

/* create English record */
void createEnglishRecord ( bson *obj ) ;

/* create Chinese record */
void createChineseRecord ( bson *obj ) ;

void createNameList ( bson *objlist, INT32 listSize ) ;

void displayRecord( sdbCursorHandle *cursor ) ;

void waiting(int second ) ;


SDB_EXTERN_C_END

#endif
