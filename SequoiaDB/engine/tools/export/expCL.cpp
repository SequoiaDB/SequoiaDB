/*******************************************************************************

   Copyright (C) 2011-2018 SequoiaDB Ltd.

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

   Source File Name = expCL.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who          Description
   ====== =========== ============ =============================================
          29/07/2016  Lin Yuebang  Initial Draft

   Last Changed =

*******************************************************************************/
#include "expCL.hpp"
#include "expOptions.hpp"
#include "pd.hpp"
#include "expUtil.hpp"
#include "jstobs.h"
#include <iostream>

namespace exprt
{
   #define EXP_SPACE_CHAR  ' '
   #define EXP_TAB_CHAR    '\t'
   #define EXP_DOT_CHAR    '.'
   #define EXP_COMMA_CHAR  ','

   #define EXP_COMMA_STR   ","

   #define EXP_RECORD_KEYNAME "_id"

   #define EXP_SELECT_WITHOUT_ID "{ _id : { $include : 0 } }"
   
   INT32 expCL::parseCLFields( const string &rawStr ) 
   {
      INT32 rc = SDB_OK ;
      string::size_type dotPos = string::npos ;
      string::size_type sepPos = string::npos ;
   
      dotPos = rawStr.find_first_of(EXP_DOT_CHAR) ;
      sepPos = rawStr.find_first_of(EXPCL_FIELDS_SEP_CHAR) ;
   
      if ( string::npos == dotPos )
      {
         PD_LOG( PDERROR, "Invalid format of collection, does not contain '%c'",
                 EXP_DOT_CHAR) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( string::npos == sepPos )
      {
      }
      else if ( sepPos <= dotPos + 1 )
      {
         PD_LOG( PDERROR, "Invalid format of collection, '" 
                          EXPCL_FIELDS_SEP_STR "' sits before '%c'",
                 EXP_DOT_CHAR) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      else
      {
         fields = string( rawStr, sepPos + 1 ) ;
         trimBoth(fields) ;
      }

      csName = string( rawStr, 0, dotPos ) ;
      clName = string( rawStr, dotPos+1, sepPos - (dotPos+1) ) ;
      trimBoth(csName) ;
      trimBoth(clName) ;

   done :
      return rc ;
   error :
      goto done ;
   }

   bool operator<( const expCL &CL1, const expCL &CL2 )
   {
      return ( CL1.csName < CL2.csName ) || 
             ( CL1.csName == CL2.csName && CL1.clName < CL2.clName ) ;
   }

   static INT32 getCLFields( sdbConnectionHandle hConn, expCL &cl, 
                             CHAR del, const string &selectStr = "" )
   {
      INT32 rc = SDB_OK ;
      sdbCursorHandle hCusor = SDB_INVALID_HANDLE ;
      sdbCollectionHandle hCL = SDB_INVALID_HANDLE ;
      string clFullName = cl.fullName() ;
      bson record ;
      bson select ;
      bson_iterator it ;
      bson_init ( &select ) ;
      bson_init ( &record ) ;

      if ( selectStr.empty() )
      {
         bson_empty( &select ) ;
      }
      else
      {
         if( !json2bson( selectStr.c_str(), NULL, CJSON_RIGOROUS_PARSE,
                         FALSE, TRUE, 0, &select ) )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Invalid format of select : %s", 
                   selectStr.c_str() ) ;
            goto error ;
         }
      }

      rc = sdbGetCollection( hConn, clFullName.c_str(), &hCL ) ;
      if ( SDB_DMS_NOTEXIST == rc )
      {
         PD_LOG( PDERROR, "Collection %s does not exist", clFullName.c_str() ) ;
         goto error ;
      }
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to get collection %s, rc = %d", 
                 clFullName.c_str(), rc ) ;
         goto error ;
      }
      
      rc = sdbQuery ( hCL, NULL, &select, NULL, NULL, 0, 1, &hCusor ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to query the first record of %s, rc = %d",
                 clFullName.c_str(), rc ) ;
         goto error ;
      }

      // get the first record
      rc = sdbNext( hCusor, &record ) ;
      if ( SDB_DMS_EOC == rc )
      {
         PD_LOG( PDINFO, "Collection %s is empty", clFullName.c_str() ) ;
         rc = SDB_OK ;
         goto done ;
      }
      else if ( SDB_OK != rc)
      {
         PD_LOG( PDERROR, "Failed to get the first record of %s, rc = %d",
                 clFullName.c_str(), rc ) ;
         goto error ;
      }

      // get each filed, except the '_id'
      bson_iterator_init ( &it, &record ) ;
      while ( BSON_EOO != bson_iterator_next ( &it ) )
      {
         const CHAR *key = bson_iterator_key ( &it ) ;
         cl.fields += key ;
         cl.fields += del ;
      }
      // erase last delimiter
      if ( !cl.fields.empty() )
      {
         cl.fields.resize( cl.fields.size() - 1 ) ;
      }
      
   done :
      bson_destroy( &select ) ;
      bson_destroy( &record ) ;
      if ( SDB_INVALID_HANDLE != hCL )
      {
         sdbReleaseCollection(hCL) ;
      }
      if ( SDB_INVALID_HANDLE != hCusor )
      {
         sdbCloseCursor(hCusor) ;
         sdbReleaseCursor(hCusor) ;
      }
      return rc ;
   error :
      goto done ;
   }

   // parse --cscl/--excludecscl into corresponding set
   static INT32 getCSCLSet( const string &includeCSCLs, 
                            const string &excludeCSCLs,
                            set<string> &includeCS, 
                            set<expCL> &includeCollection,
                            set<string> &excludeCS, 
                            set<expCL> &excludeCollection )
   {
      INT32 rc = SDB_OK ;
      vector<string> includeList, excludeList ;

      if ( !includeCSCLs.empty() )
      {
         cutStr( includeCSCLs, includeList, EXP_COMMA_STR ) ;
      }
      if ( !excludeCSCLs.empty() )
      {
         cutStr( excludeCSCLs, excludeList, EXP_COMMA_STR ) ;
      }

      for ( vector<string>::iterator it = excludeList.begin();
            excludeList.end() != it; ++it )
      {
         // is name of cs
         if ( string::npos == it->find_first_of(EXP_DOT_CHAR) ) 
         {
            excludeCS.insert(*it) ;
         }
         // is full name of cl
         else
         {
            expCL cl ;
            rc = cl.parseCLFields(*it) ;
            if ( SDB_OK != rc )
            {
               cerr << "invalid format of \"" << *it << "\"" << endl ;
               goto error ;
            }
            excludeCollection.insert(cl) ;
         }
      } // end for

      for ( vector<string>::iterator it = includeList.begin();
            includeList.end() != it; ++it )
      {
         // is name of cs
         if ( string::npos == it->find_first_of(EXP_DOT_CHAR) ) 
         {
            if ( excludeCS.end() == excludeCS.find(*it) )
            {
               includeCS.insert(*it) ;
            }
         }
         // is full name of cl
         else
         {
            expCL cl ;
            rc = cl.parseCLFields(*it) ;
            if ( SDB_OK != rc )
            {
               cerr << "invalid format of \"" << *it << "\"" << endl ;
               goto error ;
            }

            if ( excludeCS.end() == excludeCS.find(cl.csName) &&
                 excludeCollection.end() == excludeCollection.find(cl) && 
                 includeCS.end() == includeCS.find(cl.csName) )
            {
               includeCollection.insert(cl) ;
            }
         }
      } // end for
      
   done :
      return rc ;
   error :
      goto done ;
   }

   static INT32 checkCS( sdbConnectionHandle hConn, const string &cs )
   {
      INT32 rc = SDB_OK ;
      sdbCSHandle hCS = SDB_INVALID_HANDLE ;
      rc = sdbGetCollectionSpace ( hConn, cs.c_str(), &hCS ) ;
      if ( SDB_DMS_CS_NOTEXIST == rc )
      {
         cerr << "collection space " << cs << " does not exist" << endl ;
         PD_LOG ( PDERROR, "Collection space %s does not exist", cs.c_str() ) ;
         goto error ;
      }
      else if( SDB_OK != rc )
      {
         PD_LOG ( PDERROR, "Failed to get collection space %s, rc = %d",
                  cs.c_str(), rc ) ;
         goto error ;
      }
   done :
      if ( SDB_INVALID_HANDLE != hCS )
      {
         sdbReleaseCS(hCS) ;
      }
      return rc ;
   error :
      goto done ;
   }

   static INT32 checkCL( sdbConnectionHandle hConn, const expCL &cl )
   {
      INT32 rc = SDB_OK ;
      sdbCollectionHandle hCL = SDB_INVALID_HANDLE ;
      string fullName = cl.fullName() ;
      rc = sdbGetCollection( hConn, fullName.c_str(), &hCL ) ;
      if ( SDB_DMS_NOTEXIST == rc )
      {
         cerr << "collection " << fullName << " does not exist" << endl ;
         PD_LOG( PDERROR, "Collection %s does not exist", fullName.c_str() ) ;
         goto error ;
      }
      else if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to get collection %s", fullName.c_str() ) ;
         goto error ;
      }
   done :
      if ( SDB_INVALID_HANDLE != hCL )
      {
         sdbReleaseCollection(hCL) ;
      }
      return rc ;
   error :
      goto done ;
   }

   // check whether each cs or cl in the set exists
   static INT32 checkCSCLSet( sdbConnectionHandle hConn,
                              const set<string> &includeCS, 
                              const set<expCL> &includeCollection,
                              const set<string> &excludeCS, 
                              const set<expCL> &excludeCollection )
   {
      INT32 rc = SDB_OK ;

      // include-cs-set
      for ( set<string>::const_iterator it = includeCS.begin(); 
            includeCS.end() != it; ++it)
      {
         rc = checkCS( hConn,*it ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to check cs in include-cs set" ) ;
            goto error ;
         }
      }

      // include-cl-set
      for ( set<expCL>::const_iterator it = includeCollection.begin(); 
            includeCollection.end() != it; ++it)
      {
         rc = checkCL( hConn,*it ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to check cl in include-cl set" ) ;
            goto error ;
         }
      }
      
   done :
      return rc ;
   error :
      goto done ;
   }  

   // generate the contents for _collections from cs/cl set
   INT32 expCLSet::_generateCLList( sdbConnectionHandle hConn,
                                    const set<string> &includeCS, 
                                    const set<expCL> &includeCollection,
                                    const set<string> &excludeCS, 
                                    const set<expCL> &excludeCollection )
   {
      INT32 rc = SDB_OK ;
      sdbCursorHandle     hCLList = SDB_INVALID_HANDLE ;
      const CHAR          *clFullName = NULL ;
      bson_type           bsonType = BSON_STRING ;
      vector<expCL>       &clList = _collections ;
      bson_iterator  it ;
      bson           clObj ;

      bson_init(&clObj) ;

      rc = sdbListCollections( hConn, &hCLList ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG ( PDERROR, "Failed to list collections, rc = %d", rc ) ;
         goto error ;
      }
      
      while (TRUE)
      {
         expCL cl ;
         
         rc = sdbNext( hCLList, &clObj ) ; 
         if (SDB_DMS_EOC == rc) 
         { 
            rc = SDB_OK ;
            break ; 
         } 
         else if ( SDB_OK != rc) 
         { 
            PD_LOG ( PDERROR, "Failed to get next collection, rc = %d", rc ) ; 
            goto error ; 
         } 
         
         bsonType = bson_find( &it, &clObj, "Name" ) ; 
         if ( BSON_STRING != bsonType ) 
         { 
            rc = SDB_SYS ; 
            PD_LOG ( PDERROR, "Incorrected type of collection's name ,"
                              "type = %d", 
                     bsonType) ;
            goto error ; 
         } 
         clFullName = bson_iterator_string( &it ) ;

         rc = cl.parseCLFields(clFullName) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to parse the fullname \"%s\" for expCL",
                    clFullName ) ;
            goto error ;
         }

         // cl is excluded
         if ( ( excludeCS.end() != excludeCS.find(cl.csName) ) ||
              ( excludeCollection.end() != excludeCollection.find(cl) ) )
         {
            continue ;
         }

         // cl is included
         if ( ( _includeAll ) ||
              ( includeCS.end() != includeCS.find(cl.csName) ) ||
              ( includeCollection.end() != includeCollection.find(cl) ) )
         {
            clList.push_back( expCL() ) ;
            clList.back().swap(cl) ;
         }
      }
   done :
      bson_destroy(&clObj) ;
      if ( SDB_INVALID_HANDLE != hCLList )
      {
         sdbCloseCursor(hCLList) ;
         sdbReleaseCursor(hCLList) ;
      }
      return rc ;
   error :
      goto done ;
   }

   // parse the option '--fields' format as <clFullName>:<Fields-list> or
   // just as <Fields-list> for single collection
   // the results will be stored in set<expCL>
   INT32 expCLSet::_parseRawFileds( set<expCL> &clFieldsSet )
   {
      INT32 rc = SDB_OK ;
      const vector<string> &rawCLFields = _options.fieldsList() ;
      // the fields should be format as <clFullName>:<field-list>
      // since compat for old version, one fields may be format as <field-list>
      // so completes the prefix '<clFullName>:' here
      if ( 1 == rawCLFields.size() && 1 == rawCLFields.size() &&
           string::npos == rawCLFields.back().find(EXPCL_FIELDS_SEP_STR) )
      {
         expCL cl ;
         string clFields = _options.cscl() ;
         clFields += EXPCL_FIELDS_SEP_STR ;
         clFields += rawCLFields.back() ;
         rc = cl.parseCLFields(clFields) ;
         if ( SDB_OK != rc )
         {
            cerr << "invalid format to specify the fileds : "
                 << rawCLFields.back() << endl ;
            PD_LOG( PDERROR, "invalid format to specify the fileds : %s",
                    rawCLFields.back().c_str() ) ;
            goto error ;
         }
         clFieldsSet.insert(cl) ;
      }
      else 
      {
         for ( vector<string>::const_iterator it = rawCLFields.begin();
               rawCLFields.end() != it; ++it )
         {
            expCL cl ;
            rc = cl.parseCLFields(*it) ;
            if ( SDB_OK != rc )
            {
               cerr << "invalid format to specify the fileds : "
                    << *it << endl ;
               PD_LOG( PDERROR, "invalid format to specify the fileds : %s",
                       it->c_str() ) ;
               goto error ;
            }
            clFieldsSet.insert(cl) ;
         } // end for
      } // end else
   done :
      return rc ;
   error :
      goto done ; 
   }

   INT32 expCLSet::_completeCLListFields( sdbConnectionHandle hConn )
   {
      INT32 rc = SDB_OK ;
      set<expCL> clFieldsSet ;

      rc = _parseRawFileds(clFieldsSet) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to parse fields" ) ;
         goto error ;
      }

      // complete the <field-list> for each collection
      for ( vector<expCL>::iterator it = _collections.begin();
            _collections.end() != it; ++it )
      {
         set<expCL>::const_iterator found = clFieldsSet.find(*it) ;
         if ( clFieldsSet.end() != found )
         {
            it->fields = found->fields ;
         }

         if ( !it->fields.empty() ) { continue ; }

         if ( _options.hasGenConf() )
         {
            const CHAR* pSelect = EXP_SELECT_WITHOUT_ID ;
            if ( _options.withId() ) 
            { 
               pSelect = "" ; 
            }
            rc = getCLFields( hConn, *it, EXP_COMMA_CHAR, pSelect ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "Failed to get fields for collection" ) ;
               goto error ;
            }
         }
         else if ( FORMAT_CSV == _options.type() )
         {
            const CHAR* pSelect = EXP_SELECT_WITHOUT_ID ;
            if ( _options.withId() ) 
            { 
               pSelect = "" ; 
            }
            if ( !_options.select().empty() ) 
            { 
               pSelect = _options.select().c_str() ;
            }
            rc = getCLFields( hConn, *it, EXP_COMMA_CHAR, pSelect ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "Failed to get fields for collection" ) ;
               goto error ;
            }

            // for csv, when the collection is not empty, 
            // fields must be specified
            if ( FORMAT_CSV == _options.type() && 
                 !_options.force() &&
                 !it->fields.empty() &&
                 _options.select().empty() )
            {
               cerr << "for csv, fields for each collection must be specified"
                    << endl ;
               PD_LOG( PDERROR, "For csv, fields for each collection "
                                "must be specified, but collection %s not",
                       it->fullName().c_str() ) ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }
         }
         else if( FORMAT_JSON == _options.type() )
         {
            it->select = EXP_SELECT_WITHOUT_ID ;
            if ( _options.withId() )
            {
               it->select = "" ; 
            }
            if ( !_options.select().empty() )
            {
               it->select = _options.select().c_str() ;
            }
         }

      } // end for
      
   done :
      return rc ;
   error :
      goto done ; 
   }

   // do some checking and specialize for single collection 
   INT32 expCLSet::_parsePost()
   {
      INT32 rc = SDB_OK ;
      if ( 1 == _collections.size() )
      {
         if ( !_options.select().empty() )   
         { 
            _collections.back().select = _options.select() ;
         }
         if ( !_options.filter().empty() )   
         { 
            _collections.back().filter = _options.filter() ;
         }
         if ( !_options.sort().empty() )   
         { 
            _collections.back().sort = _options.sort() ;
         }
         _collections.back().skip = _options.skip() ;
         _collections.back().limit = _options.limit() ;
      }
      else if ( _collections.size() > 1 &&
                ( !_options.select().empty() ||
                  !_options.filter().empty() ||
                  !_options.sort().empty()  )   
               )
      {
         cerr << "ambiguity of select/filter/sort in more than one collections"
              << endl ;
         PD_LOG( PDERROR, "ambiguity of select/filter/sort "
                          "in more than one collections" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      else if ( _collections.size() > 1 && !_options.file().empty() )
      {
         cerr << "ambiguity of output file in more than one collections"
              << endl ;
         PD_LOG( PDERROR, "ambiguity of output file "
                          "in more than one collections" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
   done :
      return rc ;
   error :
      goto done ; 
   }

   INT32 expCLSet::parse( sdbConnectionHandle hConn )
   {
      INT32 rc = SDB_OK ;
      set<string>         includeCS ;
      set<string>         excludeCS ;
      set<expCL>          includeCollection ;
      set<expCL>          excludeCollection ;

      if ( _options.cscl().empty() )
      {
         _includeAll = TRUE ;
      }

      rc = getCSCLSet( _options.cscl(), _options.excludeCscl(),
                       includeCS, includeCollection,
                       excludeCS, excludeCollection ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to get cs/cl set, rc = %d", rc ) ;
         goto error ;
      }

      rc = checkCSCLSet( hConn,
                         includeCS, includeCollection,
                         excludeCS, excludeCollection ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Invalid cs/cl specified, rc = %d", rc ) ;
         goto error ;
      }

      if ( includeCS.empty() && !includeCollection.empty() )
      {
         _collections = vector<expCL>( includeCollection.begin(), 
                                       includeCollection.end() ) ;
      }
      else
      {
         rc = _generateCLList( hConn, includeCS, includeCollection,
                               excludeCS, excludeCollection ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG ( PDERROR, "Failed to generate collection list, rc = %d",rc);
            goto error ;
         }
      }

      rc = _completeCLListFields(hConn) ;
      if ( SDB_OK != rc )
      {
         PD_LOG ( PDERROR, "Failed to complete fields for collection list,"
                  " rc = %d", rc ) ;
         goto error ;
      }

      rc = _parsePost() ;
      if ( SDB_OK != rc )
      {
         PD_LOG ( PDERROR, "Failed in parsePost") ;
         goto error ;
      }
      
   done :
      return rc ;
   error :
      goto done ;
   }
}
