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

   Source File Name = sqlBsonBuilder.cpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#include "sqlBsonBuilder.hpp"
#include "pd.hpp"
#include "sqlUtil.hpp"
#include "../util/fromjson.hpp"
#include "pdTrace.hpp"
#include "sqlTrace.hpp"
#include "boost/algorithm/string.hpp"
#include <sstream>

namespace engine
{
   #define SQL_CHILDREN_CHECK\
           {\
              if ( 2 != itr->children.size() )\
              {\
                 SDB_ASSERT( FALSE, "impossible" ) ; \
                 rc = SDB_INVALIDARG ;\
                 goto error ;\
              }\
            }

   INT32 _sqlBsonBuilder::buildFullName( const SQL_CONTAINER &c,
                                         string &fullName )
   {
      INT32 rc = SDB_OK ;
      if ( c.empty() )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      fullName = boost::trim_copy(string( c.begin()->value.begin(),
                         c.begin()->value.end() )) ;
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sqlBsonBuilder::buildSelector( const SQL_CONTAINER &c,
                                         BSONObj &obj )
   {
      INT32 rc = SDB_OK ;
      BSONObjBuilder builder ;

      if ( c.empty() )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      rc =  _buildSelector( c.begin(), builder ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }
      obj = builder.obj() ;
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sqlBsonBuilder::buildOrder( const SQL_CONTAINER &c,
                                      BSONObj &obj )
   {
      INT32 rc = SDB_OK ;
      BSONObjBuilder builder ;
      if ( c.empty() )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      if ( c.begin()->children.empty() )
      {
         builder.append( boost::trim_copy(
                           string( c.begin()->value.begin(),
                                   c.begin()->value.end()))
                           , 1 ) ;
      }
      {
      SQL_CON_ITR end = c.begin()->children.end() ;
      SQL_CON_ITR itr = c.begin()->children.begin();
      while ( TRUE )
      {
         if ( itr == end )
         {
            break ;
         }

         if ( itr == end - 1 )
         {
            builder.append( boost::trim_copy(
                           string( itr->value.begin(), itr->value.end()))
                           , 1 ) ;
            break ;
         }
         else
         {
            if ( SQL_SYMBOL_DESC ==  boost::to_lower_copy(boost::trim_copy(
                           string( (itr+1)->value.begin(),
                                   (itr+1)->value.end() ))))
            {
               builder.append( boost::to_lower_copy(trim_copy(
                           string( itr->value.begin(), itr->value.end()))),
                              -1 );
               itr += 2 ;
            }
            else if ( SQL_SYMBOL_ASC ==  boost::trim_copy(
                           string( (itr+1)->value.begin(),
                                   (itr+1)->value.end() )))
            {
               builder.append( boost::trim_copy(
                           string( itr->value.begin(), itr->value.end())),
                              1 );
               itr += 2 ;
            }
            else
            {
               builder.append( boost::trim_copy(
                           string( itr->value.begin(), itr->value.end())),
                              1 );
               itr++ ;
            }
         }
      }

      obj = builder.obj() ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sqlBsonBuilder::buildCondition( const SQL_CONTAINER &c,
                                          BSONObj &obj )
   {
      INT32 rc = SDB_OK ;
      string jEle ;
      stringstream json ;
      if ( c.empty() )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      rc = _buildCondition( c.begin(), jEle ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      json << "{" << jEle << "}" ;

      rc = fromjson( json.str().c_str(), obj ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sqlBsonBuilder::buildSet( const SQL_CONTAINER &c,
                                    BSONObj &obj )
   {
      INT32 rc = SDB_OK ;
      stringstream ss ;
      if ( c.empty() )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      sqlDumpAst(c) ;
      ss << "{$set:{" ;
      rc = _buildSet( c.begin(), ss ) ;
      ss.seekp((INT32)ss.tellp()-1 ) ;
      ss << "}}" ;

      rc = fromjson( ss.str().c_str(), obj ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sqlBsonBuilder::buildInsertObj( const SQL_CONTAINER &fields,
                                          const SQL_CONTAINER &values,
                                          BSONObj &obj )
   {
      INT32 rc = SDB_OK ;
      stringstream ss ;
      if ( fields.empty() || values.empty() )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( fields.begin()->children.size()
           != values.begin()->children.size() )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      ss << "{" ;
      if ( fields.begin()->children.empty() )
      {
         SQL_CON_ITR fitr = fields.begin() ;
         SQL_CON_ITR vitr = values.begin() ;
         ss << boost::trim_copy( string( fitr->value.begin(),
                                        fitr->value.end()) )
            << ":"
            << boost::trim_copy( string( vitr->value.begin(),
                                         vitr->value.end()) ) ;
      }
      else
      {
      SQL_CON_ITR fitr = fields.begin()->children.begin() ;
      SQL_CON_ITR vitr = values.begin()->children.begin() ;
      for ( ; fitr != fields.begin()->children.end(); fitr++, vitr++ )
      {
         ss << boost::trim_copy( string( fitr->value.begin(),
                                         fitr->value.end()) )
            << ":"
            << boost::trim_copy( string( vitr->value.begin(),
                                         vitr->value.end()) ) ;
         ss << "," ;
      }
      ss.seekp((INT32)ss.tellp()-1 ) ;
      }
      ss << "}" ;
      rc = fromjson( ss.str().c_str(),obj ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sqlBsonBuilder::_buildSelector( const SQL_CON_ITR &itr,
                                          BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;
      if ( itr->value.begin() == itr->value.end() )
      {
         if ( itr->children.empty() )
         {
            goto done ;
         }
         for ( SQL_CON_ITR citr = itr->children.begin();
               citr != itr->children.end();
               citr++ )
         {
            _buildSelector( citr, builder ) ;
         }
      }
      else
      {
         builder.append(boost::trim_copy( string( itr->value.begin(),
                                         itr->value.end() ) ),
                        "") ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sqlBsonBuilder::_buildSet( const SQL_CON_ITR &itr,
                                    stringstream &ss )
   {
      INT32 rc = SDB_OK ;
      if ( SQL_SYMBOL_EG == string( itr->value.begin(),
                                    itr->value.end() ) )
      {
         SQL_CHILDREN_CHECK
         ss << string( itr->children.begin()->value.begin(),
                       itr->children.begin()->value.end() )
            << ":"
            << string( (itr->children.begin()+1)->value.begin(),
                       (itr->children.begin()+1)->value.end() )
            << "," ;
      }
      else
      {
         for ( SQL_CON_ITR citr = itr->children.begin();
               citr != itr->children.end();
               citr++ )
         {
            _buildSet( citr, ss ) ;
         }
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sqlBsonBuilder::_buildCondition( const SQL_CON_ITR &itr,
                                          string &s )
   {
      INT32 rc = SDB_OK ;
      stringstream ss ;
      string value = string( itr->value.begin(), itr->value.end() ) ;
      if ( itr->children.empty() )
      {
         ss << boost::trim_copy(value) ;
      }
      else if ( 0 == value.compare( SQL_SYMBOL_LT ) )
      {
         SQL_CHILDREN_CHECK
         {
         string lchild, rchild ;
         SQL_CON_ITR child = itr->children.begin() ;
         rc = _buildCondition( child,
                              lchild ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }
         rc = _buildCondition( ++child,
                              rchild ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }
         ss << lchild << ":{" << "$lt:" << rchild << "}" ;
         }
      }
      else if ( 0 == value.compare( SQL_SYMBOL_GT ) )
      {
         SQL_CHILDREN_CHECK
         {
         string lchild, rchild ;
         SQL_CON_ITR child = itr->children.begin() ;
         rc = _buildCondition( child,
                              lchild ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }
         rc = _buildCondition( ++child,
                              rchild ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }
         ss << lchild << ":{" << "$gt:" << rchild << "}" ;
         }
      }
      else if ( 0 == value.compare( SQL_SYMBOL_EG ) )
      {
         SQL_CHILDREN_CHECK
         {
         string lchild, rchild ;
         SQL_CON_ITR child = itr->children.begin() ;
         rc = _buildCondition( child,
                              lchild ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }
         rc = _buildCondition( ++child,
                              rchild ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }
         ss << lchild << ":" << rchild ;
         }
      }
      else if ( 0 == value.compare( SQL_SYMBOL_NE ) )
      {
         SQL_CHILDREN_CHECK
         {
         string lchild, rchild ;
         SQL_CON_ITR child = itr->children.begin() ;
         rc = _buildCondition( child,
                              lchild ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }
         rc = _buildCondition( ++child,
                              rchild ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }
         ss << lchild << ":{" << "$ne:" << rchild << "}" ;
         }
      }
      else if ( 0 == value.compare( SQL_SYMBOL_GTE ) )
      {
         SQL_CHILDREN_CHECK
         {
         string lchild, rchild ;
         SQL_CON_ITR child = itr->children.begin() ;
         rc = _buildCondition( child,
                              lchild ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }
         rc = _buildCondition( ++child,
                              rchild ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }
         ss << lchild << ":{" << "$gte:" << rchild << "}" ;
         }
      }
      else if ( 0 == value.compare( SQL_SYMBOL_LTE ) )
      {
         SQL_CHILDREN_CHECK
         {
         string lchild, rchild ;
         SQL_CON_ITR child = itr->children.begin() ;
         rc = _buildCondition( child,
                              lchild ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }
         rc = _buildCondition( ++child,
                              rchild ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }
         ss << lchild << ":{" << "$lte:" << rchild << "}" ;
         }
      }
      else if ( 0 == value.compare( SQL_SYMBOL_AND ) )
      {
         SQL_CHILDREN_CHECK
         {
         string lchild, rchild ;
         SQL_CON_ITR child = itr->children.begin() ;
         rc = _buildCondition( child,
                              lchild ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }
         rc = _buildCondition( ++child,
                              rchild ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }
         ss << "$and:{" << lchild << "," << rchild << "}" ;
         }
      }
      else if ( 0 == value.compare( SQL_SYMBOL_OR ) )
      {
         SQL_CHILDREN_CHECK
         {
         string lchild, rchild ;
         SQL_CON_ITR child = itr->children.begin() ;
         rc = _buildCondition( child,
                              lchild ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }
         rc = _buildCondition( ++child,
                              rchild ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }
         ss << "$or:{" << lchild << "," << rchild << "}" ;
         }
      }
      else
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      s = ss.str() ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sqlBsonBuilder::buildCreateIxm( const SQL_CONTAINER &c1,
                                          const SQL_CONTAINER &c2,
                                          BSONObj &obj,
                                          string &fullName )
   {
      INT32 rc = SDB_OK ;
      string indexName ;
      BOOLEAN isUnique = FALSE ;
      SQL_CON_ITR itr ;
      BSONObj fields ;
      BSONObjBuilder builder ;
      if ( c1.empty() || c2.empty() )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( c1.begin()->children.size() == 3 )
      {
         itr = c1.begin()->children.begin() + 1;
         isUnique = TRUE ;
      }
      else if ( c1.begin()->children.size() == 2 )
      {
         itr = c1.begin()->children.begin() ;
         isUnique = FALSE ;
      }
      else
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      indexName = boost::trim_copy( string(itr->value.begin(),
                                              itr->value.end()) ) ;
      itr++ ;
      fullName = boost::trim_copy( string(itr->value.begin(),
                                              itr->value.end()) ) ;

      rc = buildOrder( c2, fields ) ;
      if ( SDB_OK != rc )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      builder.append( "name", indexName ) ;
      builder.append( "key", fields ) ;
      builder.appendBool( "unique", isUnique ) ;
      obj = builder.obj() ;
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sqlBsonBuilder::buildDropIxm( const SQL_CONTAINER &c,
                                        BSONObj &obj,
                                        string &fullName )
   {
      INT32 rc = SDB_OK ;
      if ( c.empty() )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( c.begin()->children.size() != 2 )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      {
      SQL_CON_ITR itr = c.begin()->children.begin() ;
      obj = BSON( "" << boost::trim_copy(
                        string( itr->value.begin(), itr->value.end()) ) ) ;
      itr++ ;
      fullName = boost::trim_copy( string( itr->value.begin(),
                                           itr->value.end()) ) ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

}
