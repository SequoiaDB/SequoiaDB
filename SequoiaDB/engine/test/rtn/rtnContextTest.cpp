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
*******************************************************************************/

#include "ossTypes.hpp"
#include <gtest/gtest.h>
#include <iostream>

#include "rtnCB.hpp"

using namespace std;
using namespace engine;
using namespace bson ;

static BOOLEAN s_testTRUE() { return TRUE ; }

static void s_testContext( const rtnContextPtr &context, bool isPtrValid )
{
   // test context with bool() operators
   if ( context )
   {
      ASSERT_TRUE( NULL != context.get() ) ;
      ASSERT_TRUE( isPtrValid ) ;
   }
   else
   {
      ASSERT_TRUE( NULL == context.get() ) ;
      ASSERT_FALSE( isPtrValid ) ;
   }

   if ( !context )
   {
      ASSERT_TRUE( NULL == context.get() ) ;
   }
   else
   {
      ASSERT_TRUE( NULL != context.get() ) ;
   }

   if ( s_testTRUE() && context )
   {
      ASSERT_TRUE( NULL != context.get() ) ;
   }
   else
   {
      ASSERT_TRUE( NULL == context.get() ) ;
   }

   if ( s_testTRUE() && !context )
   {
      ASSERT_TRUE( NULL == context.get() ) ;
   }
   else
   {
      ASSERT_TRUE( NULL != context.get() ) ;
   }
}

TEST(rtnContextTest, interface)
{
   pmdEDUMgr mgr ;
   pmdEDUCB eduCB( &mgr, EDU_TYPE_AGENT ) ;
   SDB_RTNCB rtnCB ;

   INT64 contextID = -1 ;
   rtnContextPtr context ;

   s_testContext( context, false ) ;

   ASSERT_TRUE( SDB_OK == rtnCB.contextNew( RTN_CONTEXT_DUMP,
                                            context,
                                            contextID,
                                            &eduCB ) ) ;
   ASSERT_TRUE( 2 == context.refCount() ) ;
   s_testContext( context, true ) ;

   context.release() ;
   s_testContext( context, false ) ;

   ASSERT_TRUE( SDB_OK == rtnCB.contextFind( contextID, context, &eduCB ) ) ;
   ASSERT_TRUE( 2 == context.refCount() ) ;
   s_testContext( context, true ) ;

   rtnCB.contextDelete( contextID, &eduCB ) ;
   ASSERT_TRUE( 1 == context.refCount() ) ;
   s_testContext( context, true ) ;

   context.release() ;
   s_testContext( context, false ) ;
}

TEST(rtnContextTest, count)
{
   pmdEDUMgr mgr ;
   pmdEDUCB eduCB( &mgr, EDU_TYPE_AGENT ) ;
   SDB_RTNCB rtnCB ;

   INT64 contextIDs[ 10 ] ;
   for ( UINT32 i = 0 ; i < 10 ; ++ i )
   {
      contextIDs[ i ] = -1 ;
   }

   for ( UINT32 i = 0 ; i < 10 ; ++ i )
   {
      INT64 contextID = -1 ;
      rtnContextPtr context ;
      ASSERT_TRUE( SDB_OK == rtnCB.contextNew( RTN_CONTEXT_DUMP,
                                               context,
                                               contextID,
                                               &eduCB ) ) ;
      contextIDs[ i ] = contextID ;
   }

   ASSERT_TRUE( 10 == rtnCB.contextNum() ) ;

   for ( UINT32 i = 0 ; i < 10 ; ++ i )
   {
      INT64 contextID = contextIDs[ i ] ;
      rtnCB.contextDelete( contextID, &eduCB ) ;
      contextIDs[ i ] = -1 ;
   }

   ASSERT_TRUE( 0 == rtnCB.contextNum() ) ;
}
