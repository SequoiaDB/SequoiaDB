/*******************************************************************************

   Copyright (C) 2023-present SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   Source File Name = utilUniqueID.cpp

   Descriptive Name =

   When/how to use: Process CS/CL Unique ID

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who     Description
   ====== ========== ======= ==============================================
          09/08/2018 Ting YU Initial Draft

   Last Changed =

*******************************************************************************/
#include "utilUniqueID.hpp"
#include "msgDef.h"

using namespace bson ;

namespace engine
{

   // output:
   // [
   //    < "bar1", 2667174690817 >, < "bar2", 2667174690818 >
   // ]
   std::string utilClNameId2Str( std::vector< PAIR_CLNAME_ID > clInfoList )
   {
      std::vector< PAIR_CLNAME_ID >::iterator it ;
      std::ostringstream ss ;

      try
      {
         ss << "[ " ;
         for ( it = clInfoList.begin() ; it != clInfoList.end() ; it++ )
         {
            if ( it != clInfoList.begin() )
            {
               ss << ", " ;
            }
            ss << "< "
               << "\"" << it->first << "\""
               << ", "
               << it->second
               << " >" ;
         }
         ss << " ]" ;
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
      }

      return ss.str() ;
   }

   // input: clInfoObj
   // [
   //    { "Name": "bar1", "UniqueID": 2667174690817 } ,
   //    { "Name": "bar2", "UniqueID": 2667174690818 }
   // ]
   //
   // output: map<string, utilCLUniqueID>
   // [
   //    < "bar1", 2667174690817 > ,
   //    < "bar2", 2667174690818 >
   // ]
   MAP_CLNAME_ID utilBson2ClNameId( const BSONObj& clInfoObj )
   {
      MAP_CLNAME_ID clMap ;

      try
      {
         BSONObjIterator it( clInfoObj ) ;
         while ( it.more() )
         {
            BSONElement subEle = it.next() ;
            if ( Object == subEle.type() )
            {
               BSONObj clObj = subEle.embeddedObject() ;
               BSONElement nameE = clObj.getField( FIELD_NAME_NAME ) ;
               BSONElement idE = clObj.getField( FIELD_NAME_UNIQUEID ) ;
               if ( String != nameE.type() || !idE.isNumber())
               {
                  continue ;
               }
               clMap[ nameE.String() ] = (utilCLUniqueID)idE.numberLong() ;
            }
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
      }

      return clMap ;
   }

   // input: clInfoObj
   // [
   //    { "Name": "bar1", "UniqueID": 2667174690817 } ,
   //    { "Name": "bar2", "UniqueID": 2667174690818 }
   // ]
   //
   // outpu: BSONObj
   // [
   //    { "Name": "bar1", "UniqueID": 0 } ,
   //    { "Name": "bar2", "UniqueID": 0 }
   // ]
   BSONObj utilSetUniqueID( const BSONObj& clInfoObj, utilCLUniqueID setValue )
   {
      BSONArrayBuilder arrBuilder ;

      try
      {
         BSONObjIterator it( clInfoObj ) ;
         while ( it.more() )
         {
            BSONElement subEle = it.next() ;
            if ( Object == subEle.type() )
            {
               BSONObj clObj = subEle.embeddedObject() ;
               BSONElement nameE = clObj.getField( FIELD_NAME_NAME ) ;
               BSONElement idE = clObj.getField( FIELD_NAME_UNIQUEID ) ;
               if ( String != nameE.type() || !idE.isNumber() )
               {
                  continue ;
               }
               arrBuilder << BSON( FIELD_NAME_NAME << nameE.String() <<
                                   FIELD_NAME_UNIQUEID << (INT64)setValue ) ;
            }
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
      }

      return arrBuilder.arr() ;
   }

   // input: idxInfoVec
   // [ { Collection: "foo.bar", IndexDef: {xxx} },
   //   { Collection: "foo.ba1", IndexDef: {xxx} }
   // ]
   // output: vector<char*, vector<BSONObj>>
   // [
   //    < "foo.bar", <def1, def2, ...>,
   //    < "foo.ba1", <def1, def2, ...> >
   // ]
   INT32 utilBson2IdxNameId( const ossPoolVector<BSONObj>& idxInfoVec,
                             MAP_CLNAME_IDX& clMap )
   {
      INT32 rc = SDB_OK ;

      try
      {
         for ( ossPoolVector<BSONObj>::const_iterator cit = idxInfoVec.begin() ;
               cit != idxInfoVec.end() ; ++cit )
         {
            const BSONObj& idxObj = *cit ;
            const CHAR* collection = NULL ;
            const CHAR* indexName = NULL ;

            BSONElement ele = idxObj.getField( FIELD_NAME_COLLECTION ) ;
            if ( String != ele.type() )
            {
               continue ;
            }
            collection = ele.valuestr() ;

            ele = idxObj.getField( IXM_FIELD_NAME_INDEX_DEF ) ;
            if ( Object != ele.type() )
            {
               continue ;
            }

            BSONObj def = ele.Obj() ;

            ele = def.getField( IXM_FIELD_NAME_NAME ) ;
            if ( String != ele.type() )
            {
               continue ;
            }
            indexName = ele.valuestr() ;

            MAP_CLNAME_IDX::iterator it = clMap.find( collection ) ;
            if ( it != clMap.end() )
            {
               it->second[ indexName ] = def ;
            }
            else
            {
               MAP_IDXNAME_DEF idxMap ;
               idxMap[ indexName ] = def ;
               clMap[ collection ] = idxMap ;
            }
         }
      }
      catch( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_RC_CHECK( rc, PDERROR, "Occur exception: %s", e.what() ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 utilGetCSBounds( const CHAR *fieldName,
                          utilCSUniqueID csUniqueID,
                          BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;

      try
      {
         // unique ID of collection space is the high 32 bits of unique ID
         // of collection
         // so the bound is between values from high 32 bits with specified
         // unique ID of collection space to high 32 bits with next unique
         // ID of collection space
         // ( csUniqueID << 32 ) <= clUniqueID < ( csUniqueID + 1 ) << 32
         INT64 lowBound = (INT64)( utilBuildCLUniqueID( csUniqueID, 0 ) ) ;
         INT64 upBound = (INT64)( utilBuildCLUniqueID( csUniqueID + 1, 0 ) ) ;

         BSONObjBuilder subBuilder( builder.subobjStart( fieldName ) ) ;
         subBuilder.append( "$gte", lowBound ) ;
         subBuilder.append( "$lt", upBound ) ;
         subBuilder.doneFast() ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build bound for collection space, "
                 "occur exception %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      return rc ;

   error:
      goto done ;
   }

   INT32 utilGetCSBounds( const CHAR *fieldName,
                          utilCSUniqueID csUniqueID,
                          BSONObj &matcher )
   {
      INT32 rc = SDB_OK ;

      try
      {
         BSONObjBuilder builder ;

         // call interface with builder
         rc = utilGetCSBounds( fieldName, csUniqueID, builder ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build bound for collection "
                      "space, rc: %d", rc ) ;

         matcher = builder.obj() ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build bound for collection space, "
                 "occur exception %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      return rc ;

   error:
      goto done ;
   }

}
