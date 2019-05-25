/****************************************************************************
 *
 * Name: common.cpp
 * Description: Common functions for sample programs
 *              This file does NOT include main function
 *
 ******************************************************************************/
#if defined (_WIN32)
#define _CRT_RAND_S
#endif
#include <stdlib.h>
#include <string>
#include <assert.h>
#include <iostream>
#if defined (_LINUX)
#include <unistd.h>
#endif
#include "common.hpp"

using namespace std;

#define TAG_START_CHAR '['
#define TAG_END_CHAR   ']'
#define COMMENT_CHAR   '#'

namespace sample
{

    static BOOLEAN _hasSrand = FALSE;
#if defined (_LINUX) || defined (_AIX)
    static UINT32 _randSeed = 0;
#endif

    static void _srand()
    {
        if ( !_hasSrand )
        {
#if defined (_WIN32)
            srand ( (UINT32) time ( NULL ) );
#elif defined (_LINUX) || defined (_AIX)
            _randSeed = time ( NULL );
#endif
            _hasSrand = TRUE ;
        }
    }

    UINT32 randNum()
    {
        UINT32 randVal = 0 ;
        if ( !_hasSrand )
        {
            _srand() ;
        }
#if defined (_WIN32)
        rand_s ( &randVal ) ;
#elif defined (_LINUX) || defined (_AIX)
        randVal = rand_r ( &_randSeed ) ;
#endif
        return randVal ;
    }

    void waiting(UINT32 milliseconds)
    {
#if defined (_WIN32)
        Sleep(milliseconds);
#else
        usleep(milliseconds*1000);
#endif
    }

    /* connect to a given database */
    INT32 connectTo ( const CHAR *pHostName,
        const CHAR *pServiceName,
        const CHAR *pUser,
        const CHAR *pPasswd,
        sdb &connection )
    {
        return connection.connect ( pHostName, pServiceName, pUser, pPasswd ) ;
    }

    /* get collection space, if the collection does not exist it will try to create
    * one */
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

    /* get a collection, if the collection does not exist, it will try to create
    * one */
    INT32 getCollection ( sdb &connection,
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
            printf ( "%s\n", obj.toString().c_str() ) ;
            ++ count ;
        }
        return rc ;
    }

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

    // create record list
    void createRecordList ( vector<BSONObj> &objlist, INT32 listSize )
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
                fromjson ( "{firstName:\"John\",lastName:\"Smith\",age:50}",
                    obj ) ;
                break ;
            case 1:
                fromjson ( "{firstName:\"Tom\",lastName:\"Johnson\",age:27}",
                    obj ) ;
                break ;
            case 2:
                fromjson ( "{ \"姓名\" : \"李四\", \"年龄\" : 30,"
                    "\"电话\" : [ \"18390378790\",\"13801598000\" ] }", obj ) ;
                break ;
            }
            objlist.push_back ( obj ) ;
        }
    }

    string toJson( const BSONObj &b )
    {
        return b.toString() ;
    }


    void displayRecord( sdbCursor &cursor )
    {
        int rc = SDB_OK ;
        BSONObj obj ;
        while( !( rc=cursor.next( obj ) ) )
        {
            cout << obj.toString() << endl ;
        }
        if( rc!=SDB_DMS_EOC )
        {
            cout<<"Failed to display records, rc = "<<rc<<endl ;
        }
    }


} // namespace sample

