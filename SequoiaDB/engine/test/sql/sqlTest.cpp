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

*******************************************************************************/

#include "ossTypes.hpp"
#include <gtest/gtest.h>
#include "sqlUtil.hpp"
#include "sqlParser.hpp"
#include "sqlBsonBuilder.hpp"
#include "sqlWhere.hpp"
#include "sqlSelect.hpp"
#include "sqlFrom.hpp"
#include "sqlOrder.hpp"
#include "sqlInsert.hpp"
#include "sqlInsertFields.hpp"
#include "sqlSet.hpp"
#include "sqlValue.hpp"
#include "../util/fromjson.hpp"

#include <stdio.h>
#include <string>
#include <iostream>

#include <boost/thread.hpp>
#include <boost/lexical_cast.hpp>

using namespace engine;
using namespace std;

/*
TEST(sqlTest, where)
{
   const CHAR *sql = "where a ^ \"34\" and (field1=\"xxx\" and field2=62 and fss <10.5) and field4>=\"-4.1\" or ( field3 < -8771 )" ;
   SQL_WHERE_GRAMMAR where ;
   tree_parse_info<> info =  ast_parse(sql, where, space_p ) ;
   cout << sql << endl ;
   cout << "full:" << info.full << "stop:" << info.stop << endl ;
   sqlDumpAst( info.trees ) ;

   BSONObj obj ;
   sqlBsonBuilder builder ;
   builder.buildCondition( info.trees, obj ) ;
   cout << obj.toString() << endl ;

}
*/

/*
TEST(sqlTest, select)
{
   const CHAR *sql = "select a from table" ;
   BSONObj obj ;
   sqlBsonBuilder builder ;
   SQL_SELECT_GRAMMAR select ;
   tree_parse_info<> info =  ast_parse(sql, select, space_p ) ;
   sqlDumpAst( info.trees ) ;
   builder.buildSelector( info.trees, obj ) ;
   info =  ast_parse("select a,b, c from table", select, space_p ) ;
    sqlDumpAst( info.trees ) ;
   builder.buildSelector( info.trees, obj ) ;
}
*/

/*
TEST(sqlTest, from)
{
   const CHAR *sql = " from a.b" ;
   string name ;
   sqlBsonBuilder builder ;
   SQL_FROM_GRAMMAR from ;
   tree_parse_info<> info =  ast_parse(sql, from, space_p ) ;
   cout << "hit:" << info.match << "stop:" << info.stop << endl ;
   sqlDumpAst( info.trees ) ;
}
*/

/*
TEST(sqlTest, order)
{
   const CHAR *sql = "order by a desc, b, c.d asc" ;
   SQL_ORDER_GRAMMAR order ;
   tree_parse_info<> info =  ast_parse(sql, order, space_p ) ;
   cout << "hit" << info.match << "stop" << info.stop << endl ;
   sqlDumpAst( info.trees ) ;
   info =  ast_parse("order by a desc", order, space_p ) ;
   cout << "hit" << info.match << "stop" << info.stop << endl ;
   sqlDumpAst( info.trees ) ;
}
*/


TEST(sqlTest, insert)
{
   const CHAR *sql = "INSERT into a.b ( c, d, e, f ) values( 6.1, \"8.1\", \"aaa\", \"bbb\")" ;
   SQL_INSERT_GRAMMAR insert ;
   SQL_IFIELDS_GRAMMAR fields ;
   SQL_VALUE_GRAMMAR value ;
   tree_parse_info<> info =  ast_parse(sql, insert, space_p ) ;
   cout << "hit" << info.match << "stop" << info.stop << endl ;
   info =  ast_parse(info.stop, fields, space_p ) ;
   cout << "hit" << info.match << "stop" << info.stop << endl ;
   sqlDumpAst( info.trees ) ;
   info =  ast_parse(info.stop, value, space_p ) ;
    cout << "hit" << info.match << "stop" << info.stop << endl ;
   sqlDumpAst( info.trees ) ;
   info =  ast_parse("(c)", fields, space_p ) ;
   sqlDumpAst( info.trees ) ;
}


TEST(sqlTest, fullSelect)
{
   {
   const CHAR *sql = "SELECT a,b,c.d from a.b where a <= \"4\" and (field1=\"xxx\" and field2=6.2 and fss <0) and field4>=\"aaa\" or ( field3 < 1 ) order by a desc, b, c.d asc" ;
   sqlParser parser ;
   SQL_REQ req ;
   INT32 rc = parser.parse( sql, req ) ;
   cout << req.type << endl ;
   cout << req.fullName << endl ;
   cout << req.condition.toString() << endl ;
   cout << req.selector.toString() << endl ;
   cout << req.order.toString() << endl ;

   ASSERT_TRUE( SDB_OK == rc ) ;
   ASSERT_TRUE( SQL_TYPE_SELECT == req.type ) ;
   ASSERT_TRUE( "a.b" == req.fullName ) ;
   BSONObj condition ;
   ASSERT_TRUE( SDB_OK == fromjson("{ $or: { $and: { $and: { a: { $lte: \"4\" }, $and: { $and: { field1: \"xxx\", field2: 6.2 }, fss: { $lt: 0 } } }, field4: { $gte:\"aaa\" } }, field3: { $lt: 1 } } }"
                                   ,condition) ) ;
   ASSERT_TRUE( condition == req.condition ) ;
   BSONObj selector ;
   ASSERT_TRUE( SDB_OK == fromjson("{ a: \"\", b: \"\", c.d: \"\" }", selector ) );
   ASSERT_TRUE( selector == req.selector ) ;
   BSONObj order ;
   ASSERT_TRUE( SDB_OK == fromjson("{ a: -1, b: 1, c.d: 1 }", order)) ;
   ASSERT_TRUE( order == req.order ) ;
   }
   {
   const CHAR *sql = "SELECT a from a.b where a <= \"4\" and (field1=\"xxx\" and field2=6.2 and fss <0) and field4>=\"aaa\" or ( field3 < 1 ) order by a desc" ;
   sqlParser parser ;
   SQL_REQ req ;
   INT32 rc = parser.parse( sql, req ) ;
   cout << req.type << endl ;
   cout << req.fullName << endl ;
   cout << req.condition.toString() << endl ;
   cout << req.selector.toString() << endl ;
   cout << req.order.toString() << endl ;

   ASSERT_TRUE( SDB_OK == rc ) ;
   ASSERT_TRUE( SQL_TYPE_SELECT == req.type ) ;
   ASSERT_TRUE( "a.b" == req.fullName ) ;
   BSONObj condition ;
   ASSERT_TRUE( SDB_OK == fromjson("{ $or: { $and: { $and: { a: { $lte: \"4\" }, $and: { $and: { field1: \"xxx\", field2: 6.2 }, fss: { $lt: 0 } } }, field4: { $gte:\"aaa\" } }, field3: { $lt: 1 } } }"
                                   ,condition) ) ;
   ASSERT_TRUE( condition == req.condition ) ;
   BSONObj selector ;
   ASSERT_TRUE( SDB_OK == fromjson("{ a: \"\"}", selector ) );
   ASSERT_TRUE( selector == req.selector ) ;
   BSONObj order ;
   ASSERT_TRUE( SDB_OK == fromjson("{ a: -1 }", order)) ;
   ASSERT_TRUE( order == req.order ) ;
   }
   {
   const CHAR *sql = "SELECT * from a.b where a <= \"4\" and (field1=\"xxx\" and field2=6.2 and fss <0) and field4>=\"aaa\" or ( field3 < 1 ) order by a desc, b, c.d asc" ;
   sqlParser parser ;
   SQL_REQ req ;
   INT32 rc = parser.parse( sql, req ) ;
   cout << req.type << endl ;
   cout << req.fullName << endl ;
   cout << req.condition.toString() << endl ;
   cout << req.selector.toString() << endl ;
   cout << req.order.toString() << endl ;

   ASSERT_TRUE( SDB_OK == rc ) ;
   ASSERT_TRUE( SQL_TYPE_SELECT == req.type ) ;
   ASSERT_TRUE( "a.b" == req.fullName ) ;
   BSONObj condition ;
   ASSERT_TRUE( SDB_OK == fromjson("{ $or: { $and: { $and: { a: { $lte: \"4\" }, $and: { $and: { field1: \"xxx\", field2: 6.2 }, fss: { $lt: 0 } } }, field4: { $gte:\"aaa\" } }, field3: { $lt: 1 } } }"
                                   ,condition) ) ;
   ASSERT_TRUE( condition == req.condition ) ;
   BSONObj selector ;
   ASSERT_TRUE( SDB_OK == fromjson("{}", selector ) );
   ASSERT_TRUE( selector == req.selector ) ;
   BSONObj order ;
   ASSERT_TRUE( SDB_OK == fromjson("{ a: -1, b: 1, c.d: 1 }", order)) ;
   ASSERT_TRUE( order == req.order ) ;
   }
}

TEST(sqlTest, fullInsert)
{
   {
   const CHAR *sql = "INSERT into a.b ( c, d, e, f ) values( 6.1, \"8.1\", \"aaa\", \"bbb\")" ;
   sqlParser parser ;
   SQL_REQ req ;
   INT32 rc = parser.parse( sql, req ) ;
   cout << req.type << endl ;
   cout << req.fullName << endl ;
   cout << req.condition.toString() << endl ;
   cout << req.selector.toString() << endl ;
   cout << req.order.toString() << endl ;
   ASSERT_TRUE( SDB_OK == rc ) ;
   ASSERT_TRUE( SQL_TYPE_INSERT == req.type ) ;
   ASSERT_TRUE( "a.b" == req.fullName ) ;
   BSONObj obj ;
   ASSERT_TRUE( SDB_OK == fromjson("{c:6.1, d:\"8.1\", e:\"aaa\", f:\"bbb\"}", obj) ) ;
   ASSERT_TRUE( obj == req.selector ) ;
   }
   {
   const CHAR *sql = "INSERT into a.b ( c ) values( 6.1)" ;
   sqlParser parser ;
   SQL_REQ req ;
   INT32 rc = parser.parse( sql, req ) ;
   cout << req.type << endl ;
   cout << req.fullName << endl ;
   cout << req.condition.toString() << endl ;
   cout << req.selector.toString() << endl ;
   cout << req.order.toString() << endl ;
   ASSERT_TRUE( SDB_OK == rc ) ;
   ASSERT_TRUE( SQL_TYPE_INSERT == req.type ) ;
   ASSERT_TRUE( "a.b" == req.fullName ) ;
   BSONObj obj ;
   ASSERT_TRUE( SDB_OK == fromjson("{c:6.1}", obj) ) ;
   ASSERT_TRUE( obj == req.selector ) ;
   }
}


TEST(sqlTest, update)
{
   const CHAR *sql = "SET c = 1";
   SQL_SET_GRAMMAR set ;
   tree_parse_info<> info =  ast_parse(sql, set, space_p ) ;
   cout << "hit" << info.match << "stop" << info.stop << endl ;
   sqlDumpAst( info.trees ) ;
   info =  ast_parse("set c =1, d =2", set, space_p ) ;
   cout << "hit" << info.match << "stop" << info.stop << endl ;
   sqlDumpAst( info.trees ) ;
}


TEST(sqlTest, fullUpdate)
{
   {
   const CHAR *sql = "UPDATE a.b SET c = 1 WHERE e = \"b\" and f = 1" ;
   sqlParser parser ;
   SQL_REQ req ;
   INT32 rc = parser.parse( sql, req ) ;
   cout << req.type << endl ;
   cout << req.fullName << endl ;
   cout << req.condition.toString() << endl ;
   cout << req.selector.toString() << endl ;
   cout << req.order.toString() << endl ;
   ASSERT_TRUE( SDB_OK == rc ) ;
   ASSERT_TRUE( SQL_TYPE_UPDATE == req.type ) ;
   ASSERT_TRUE( "a.b" == req.fullName ) ;
   BSONObj obj ;
   ASSERT_TRUE ( SDB_OK == fromjson("{$set:{c:1}}", obj) ) ;
   BSONObj condition ;
   ASSERT_TRUE ( SDB_OK == fromjson("{$and:{e:\"b\", f:1}}", condition) ) ;
   ASSERT_TRUE( req.condition == condition ) ;
   ASSERT_TRUE( req.selector == obj ) ;
   }

   {
   const CHAR *sql = "UPDATE a.b SET c = 1, d=\"xxx\" WHERE e = \"b\" and f = 1" ;
   sqlParser parser ;
   SQL_REQ req ;
   INT32 rc = parser.parse( sql, req ) ;
   cout << req.type << endl ;
   cout << req.fullName << endl ;
   cout << req.condition.toString() << endl ;
   cout << req.selector.toString() << endl ;
   cout << req.order.toString() << endl ;
   ASSERT_TRUE( SDB_OK == rc ) ;
   ASSERT_TRUE( SQL_TYPE_UPDATE == req.type ) ;
   ASSERT_TRUE( "a.b" == req.fullName ) ;
   BSONObj obj ;
   ASSERT_TRUE ( SDB_OK == fromjson("{$set:{c:1,d:\"xxx\"}}", obj) ) ;
   BSONObj condition ;
   ASSERT_TRUE ( SDB_OK == fromjson("{$and:{e:\"b\", f:1}}", condition) ) ;
   ASSERT_TRUE( req.condition == condition ) ;
   ASSERT_TRUE( req.selector == obj ) ;
   }
}

TEST( sqlTest, fullDelete )
{
   {
   const CHAR *sql = "DELETE FROM a.b where e = \"b\" and f = 1" ;
   sqlParser parser ;
   SQL_REQ req ;
   INT32 rc = parser.parse( sql, req ) ;
   cout << req.type << endl ;
   cout << req.fullName << endl ;
   cout << req.condition.toString() << endl ;
   cout << req.selector.toString() << endl ;
   cout << req.order.toString() << endl ;
   ASSERT_TRUE( SDB_OK == rc ) ;
   ASSERT_TRUE( SQL_TYPE_DELETE == req.type ) ;
   ASSERT_TRUE( "a.b" == req.fullName ) ;
   BSONObj condition ;
   ASSERT_TRUE ( SDB_OK == fromjson("{$and:{e:\"b\", f:1}}", condition) ) ;
   ASSERT_TRUE( req.condition == condition ) ;
   }

   {
   const CHAR *sql = "DELETE FROM a.b" ;
   sqlParser parser ;
   SQL_REQ req ;
   INT32 rc = parser.parse( sql, req ) ;
   cout << req.type << endl ;
   cout << req.fullName << endl ;
   cout << req.condition.toString() << endl ;
   cout << req.selector.toString() << endl ;
   cout << req.order.toString() << endl ;
   ASSERT_TRUE( SDB_OK == rc ) ;
   ASSERT_TRUE( SQL_TYPE_DELETE == req.type ) ;
   ASSERT_TRUE( "a.b" == req.fullName ) ;
   BSONObj condition ;
   ASSERT_TRUE ( SDB_OK == fromjson("{}", condition) ) ;
   ASSERT_TRUE( req.condition == condition ) ;
   }
}

TEST( sqlTest, crtIndex )
{
   {
   const CHAR *sql = "CREATE UNIQUE INDEX aaa ON a.b ( field1 desc, field2, field3 asc)" ;
   sqlParser parser ;
   SQL_REQ req ;
   INT32 rc = parser.parse( sql, req ) ;
   cout << req.type << endl ;
   cout << req.fullName << endl ;
   cout << req.condition.toString() << endl ;
   cout << req.selector.toString() << endl ;
   cout << req.order.toString() << endl ;
   ASSERT_TRUE( SDB_OK == rc ) ;
   ASSERT_TRUE( SQL_TYPE_CREATE_INDEX == req.type ) ;
   ASSERT_TRUE( "a.b" == req.fullName ) ;
   BSONObj obj ;
   ASSERT_TRUE ( SDB_OK == fromjson("{name:\"aaa\",key:{field1:-1, field2:1, field3:1}, unique:true}", obj) ) ;
   ASSERT_TRUE( req.selector == obj ) ;
   }

   {
   const CHAR *sql = "CREATE UNIQUE INDEX aaa ON a.b ( field1 desc)" ;
   sqlParser parser ;
   SQL_REQ req ;
   INT32 rc = parser.parse( sql, req ) ;
   cout << req.type << endl ;
   cout << req.fullName << endl ;
   cout << req.condition.toString() << endl ;
   cout << req.selector.toString() << endl ;
   cout << req.order.toString() << endl ;
   ASSERT_TRUE( SDB_OK == rc ) ;
   ASSERT_TRUE( SQL_TYPE_CREATE_INDEX == req.type ) ;
   ASSERT_TRUE( "a.b" == req.fullName ) ;
   BSONObj obj ;
   ASSERT_TRUE ( SDB_OK == fromjson("{name:\"aaa\",key:{field1:-1}, unique:true}", obj) ) ;
   ASSERT_TRUE( req.selector == obj ) ;
   }
}


