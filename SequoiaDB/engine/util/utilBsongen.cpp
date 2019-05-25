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

   Source File Name = utilBsongen.cpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          12/1/2014  ly  Initial Draft

   Last Changed =

*******************************************************************************/
#include "core.hpp"
#include <set>
#include "ossMem.hpp"
#include "ossUtil.hpp"
#include "utilBsongen.hpp"
#include  <iostream>
#include "pd.hpp"
#include "pdTrace.hpp"
#include "utilTrace.hpp"

#define BSONGEN_DFT_FIELD_LEN  4096
#define BSONGEN_DFT_STRING_LEN 4096
#define ID_KEY_FIELD_NAME      "_id"


#define GEN_CHECK_RC(rc)\
   if (rc != SDB_OK)\
      goto error ;\

static UINT32 genFieldName ( CHAR*,UINT32 ) ;
static INT32 genObject ( UINT32, UINT32, UINT32, UINT32, BSONObj&,
                         BOOLEAN forceOID = FALSE, INT32 curLevel = 0 ) ;
static INT32 genArray ( BSONArrayBuilder&, UINT32, UINT32, UINT32, UINT32 ) ;
static FLOAT64 randDouble () ;



enum ObjTypes
{
   _FLOAT = 0,
   _STRING,
   _DOC,
   _ARRAY,
   _FALSE,
   _TRUE,
   _INT32,
   _TIMESTAMP,
   _INT64,
   _NULLVALUE,
   _OBJMAXTYPE
} ;


/*
 * This is the function to generate a record random.
 * @maxDepth,maximal nest depth of an record.
 * */
PD_TRACE_DECLARE_FUNCTION ( SDB_BSONGEN_GENRAND, "genRandomRecord" )
INT32 genRandomRecord ( UINT32 maxFieldNum,
                        UINT32 maxFieldNameLength,
                        UINT32 maxStringLength,
                        UINT32 maxDepth,
                        BSONObj &out,
                        BOOLEAN forceOID )
{
   INT32 rc = SDB_OK ;
   SDB_ASSERT ( 0 != maxFieldNum, "max field num can't be 0" ) ;
   SDB_ASSERT ( ossStrlen ( ID_KEY_FIELD_NAME ) <= maxFieldNameLength,
                "max field len can't be less than id key length" ) ;
   SDB_ASSERT ( 0 != maxStringLength, "max string len can't be 0" ) ;
   SDB_ASSERT ( 0 != maxDepth, "max depth can't be 0" ) ;
   PD_TRACE_ENTRY ( SDB_BSONGEN_GENRAND ) ;
   try
   {
      rc = genObject ( maxFieldNum,
                       maxFieldNameLength,
                       maxStringLength,
                       maxDepth,
                       out,
                       forceOID ) ;
   }
   catch(...)
   {
      rc = SDB_SYS ;
   }
   GEN_CHECK_RC ( rc ) ;
done:
   PD_TRACE_EXITRC( SDB_BSONGEN_GENRAND, rc ) ;
   return rc ;
error:
   goto done ;
}

FLOAT64 randDouble ()
{
   UINT32 dv = 0 ;
   while ( !dv )
   {
      dv = ossRand() ;
   }
   return ( (ossRand()%2)?(-1):(1) )*(FLOAT64)ossRand() +
          ( (FLOAT64)ossRand() ) / dv ;
}


PD_TRACE_DECLARE_FUNCTION ( SDB_BSONGEN_GENOBJ, "genObject" )
INT32 genObject ( UINT32 maxFieldNum,
                  UINT32 maxFieldNameLength,
                  UINT32 maxStringLength,
                  UINT32 maxDepth,
                  BSONObj &out,
                  BOOLEAN forceOID,
                  INT32  curLevel )
{
   INT32 rc = SDB_OK ;
   PD_TRACE_ENTRY ( SDB_BSONGEN_GENOBJ ) ;
   BSONObjBuilder builder;
   CHAR fieldName [ BSONGEN_DFT_FIELD_LEN + 1 ] = {0} ;
   CHAR *pFieldName = &fieldName[0] ;
   CHAR str[BSONGEN_DFT_STRING_LEN + 1] = {0} ;
   CHAR *pStr = &str[0];
   UINT32 type ;
   UINT32 num = 0 ;
   BOOLEAN idCreated = FALSE ;
   OID oid = OID::gen() ;
   if ( forceOID && curLevel == 0 )
   {
      builder.appendOID( ID_KEY_FIELD_NAME, &oid ) ;
      idCreated = TRUE ;
   }

   std::set<UINT32> fdset;
   std::set<UINT32>::iterator it;

   if ( 0 == maxFieldNameLength )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   if ( BSONGEN_DFT_FIELD_LEN < maxFieldNameLength )
   {
      pFieldName = (CHAR*)SDB_OSS_MALLOC ( maxFieldNameLength + 1 ) ;
      if ( !pFieldName )
      {
         rc = SDB_OOM ;
         goto error ;
      }
   }

   do
   {
      num = ossRand() % maxFieldNum ;
   } while ( 0 == curLevel && 0 == num ) ;

   while ( num > 0 )
   {
      UINT32 fieldNameLen = 0 ;
      if ( 0 == curLevel && !idCreated )
      {
         ossStrncpy ( pFieldName, ID_KEY_FIELD_NAME, maxFieldNameLength ) ;
         pFieldName [ ossStrlen ( ID_KEY_FIELD_NAME ) ] = 0 ;
         fieldNameLen = ossStrlen ( ID_KEY_FIELD_NAME ) ;
      }
      else
      {
         fieldNameLen = genFieldName(pFieldName,maxFieldNameLength) ;
      }

      UINT32 hashVal = ossHash ( pFieldName, fieldNameLen ) ;
      it = fdset.find ( hashVal ) ;
      if ( it == fdset.end () )
      {
         fdset.insert ( hashVal ) ;
      }
      else
      {
         continue;
      }

      do
      {
         type = ossRand() % _OBJMAXTYPE ;
      } while ( 0 == curLevel && !idCreated &&
                ( type == _DOC || type == _ARRAY ) ) ;

      idCreated = TRUE ;

      switch ( type )
      {
      case _FLOAT:
      {
         builder.appendNumber(pFieldName,randDouble()) ;
         break;
      }
      case _STRING:
      {
         if ( BSONGEN_DFT_FIELD_LEN < maxFieldNameLength )
         {
            pStr = (CHAR*)SDB_OSS_MALLOC ( maxFieldNameLength + 1 ) ;
            if ( !pStr )
            {
               rc = SDB_OOM ;
               goto error ;
            }
         }

         genString(pStr,maxStringLength) ;

         builder.append(pFieldName,pStr) ;
         if ( pStr && pStr != &str[0] )
         {
            SDB_OSS_FREE ( pStr ) ;
            pStr = &str[0] ;
         }
         break ;
      }
      case _DOC:
      {
         if ( maxDepth < 1 )
            continue ;
         BSONObj obj ;
         rc = genObject ( maxFieldNum,
                          maxFieldNameLength,
                          maxStringLength,
                          maxDepth - 1,
                          obj,
                          forceOID,
                          curLevel + 1 ) ;
         GEN_CHECK_RC ( rc ) ;
         builder.append ( pFieldName, obj ) ;
         break ;
      }
      case _ARRAY:
      {

         if ( maxDepth < 1 )
            continue ;
         BSONArrayBuilder ba ;
         rc = genArray ( ba,
                         maxFieldNum,
                         maxFieldNameLength,
                         maxStringLength,
                         maxDepth - 1 ) ;
         GEN_CHECK_RC ( rc ) ;
         builder.append ( pFieldName, ba.arr() ) ;
         break ;
      }
      case _FALSE:
      {
         builder.appendBool ( pFieldName, FALSE ) ;
         break ;
      }
      case _TRUE:
      {
         builder.appendBool ( pFieldName, TRUE ) ;
         break ;
      }
      case _INT32:
         builder.appendNumber ( pFieldName,
                                ( (ossRand()%2)?(-1):(1) )*(INT32)ossRand() ) ;
         break ;
      case _TIMESTAMP:
      {
         UINT64 time = ossRand();
         time = (time << 32) + ossRand() ;
         builder.appendTimeT ( pFieldName, time ) ;
         break ;
      }
      case _INT64:
      {
         INT64 val = ossRand() ;
         val = (val << 32) + ossRand() ;
         val *= ( ossRand() % 2 )?(-1):(1) ;
         builder.append ( pFieldName, val ) ;
         break ;
      }
      case _NULLVALUE:
      {
         builder.appendNull(pFieldName);
         break;
      }
      default:
         rc = SDB_SYS ;
         goto error ;
      }

      --num ;
   }
   out =  builder.obj() ;

done :
   if ( pFieldName && pFieldName != &fieldName[0] )
   {
      SDB_OSS_FREE ( pFieldName ) ;
   }

   PD_TRACE_EXITRC ( SDB_BSONGEN_GENOBJ, rc ) ;
   return rc ;
error :
   goto done ;
}

/*
 *generate an Array.
 *@maxArray,the maximal length of the Array.
 *@maxFieldNameLength,maximal field name length,if this array need to generate
 *a BSONObj.
 *@maxDepth,maximal level count that array or object can embeded.
 */
PD_TRACE_DECLARE_FUNCTION ( SDB_BSONGEN_GENARR, "genArray" )
INT32 genArray ( BSONArrayBuilder &builder,
                 UINT32 maxArrayLength,
                 UINT32 maxFieldNameLength,
                 UINT32 maxStringLength,
                 UINT32 maxDepth )
{
   INT32 rc                                = SDB_OK ;
   PD_TRACE_ENTRY ( SDB_BSONGEN_GENARR ) ;
   UINT32 arrayLength                      = 0 ;
   CHAR str [ BSONGEN_DFT_STRING_LEN + 1 ] = {0};
   CHAR *pStr                              = &str[0] ;

   arrayLength                             = ossRand() % maxArrayLength ;

   while ( arrayLength > 0 )
   {
      UINT32 type = ossRand() % _OBJMAXTYPE ;
      switch ( type )
      {
      case _FLOAT:
         builder.append(randDouble()) ;
         break ;
      case _STRING:
      {
         if ( BSONGEN_DFT_FIELD_LEN < maxFieldNameLength )
         {
            pStr = (CHAR*)SDB_OSS_MALLOC ( maxFieldNameLength + 1 ) ;
            if ( !pStr )
            {
               rc = SDB_OOM ;
               goto error ;
            }
         }
         genString(pStr,maxStringLength ) ;

         builder.append(pStr) ;
         if ( pStr && pStr != &str[0] )
         {
            SDB_OSS_FREE ( pStr ) ;
            pStr = &str[0] ;
         }
         break ;
      }
      case _DOC:
      {
         if ( maxDepth < 1 )
            continue ;
         BSONObj obj ;
         rc = genObject ( maxArrayLength,
                          maxFieldNameLength,
                          maxStringLength,
                          maxDepth - 1, obj, FALSE, 1 ) ;
         GEN_CHECK_RC(rc) ;
         builder.append(obj) ;
         break ;
      }
      case _ARRAY:
      {
         if ( maxDepth < 1 )
            continue;
         BSONArrayBuilder  ba ;
         rc = genArray ( ba,
                         maxArrayLength,
                         maxFieldNameLength,
                         maxStringLength,
                         maxDepth - 1 ) ;
         GEN_CHECK_RC(rc) ;

         builder.append ( ba.arr() ) ;
         break ;
      }
      case _FALSE:
         builder.append ( FALSE ) ;
         break ;
      case _TRUE:
         builder.append ( TRUE ) ;
         break ;
      case _INT32:
         builder.append ( (( ossRand()%2 )?(-1):(1))*ossRand() ) ;
         break ;
      case _TIMESTAMP:
      {
         UINT64 time = ossRand();
         time = (time<<32) + ossRand() ;
         builder.append((bson::Date_t)time) ;
         break ;
      }
      case _INT64:
      {
         INT64 val = ossRand() ;
         val = (val<<32) + ossRand() ;
         val *= ((ossRand()%2)?(-1):(1)) ;
         builder.append(val) ;
         break ;
      }
      case _NULLVALUE:
      {
         builder.appendNull() ;
         break;
      }
      default:
         rc = SDB_SYS ;
         goto error ;
      }
      --arrayLength;
   }

done :
   PD_TRACE_EXITRC( SDB_BSONGEN_GENARR, rc ) ;
   return rc ;
error :
   goto done ;
}

UINT32 genString ( CHAR *str, UINT32 maxStringLength )
{
   UINT32 len = 0 ;

   len = ossRand() % maxStringLength ;
   for ( UINT32 i = 0; i < len; i++ )
   {
      if ( ossRand() % 2 == 1 )
         str[i] = 'a' + ossRand() % 26 ;
      else
         str[i] = 'A' + ossRand() % 26 ;
   }
   str[len] = '\0' ;
   return len ;
}

UINT32 genFieldName ( CHAR *str, UINT32 maxFieldNameLength )
{
   UINT32 len = 0 ;

   len = ( ossRand() % ( maxFieldNameLength - 1 ) )  + 1;

   for ( UINT32 i = 0; i < len; i++ )
   {
      if ( ossRand() % 2 )
         str[i] = 'a' + ossRand() % 26 ;
      else
         str[i] = 'A' + ossRand() % 26 ;
   }
   str[len] = '\0' ;
   return len ;
}









