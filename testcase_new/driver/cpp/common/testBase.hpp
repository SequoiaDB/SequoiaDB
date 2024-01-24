/**************************************************************
 * @Description: test base class
 * @Modify     : Liang xuewang 
 *               2017-09-17
 ***************************************************************/
#ifndef TESTBASE_HPP_
#define TESTBASE_HPP_

#include <gtest/gtest.h>
#include <client.hpp>
#include "arguments.hpp"

using namespace sdbclient ;

class testBase : public testing::Test
{
protected:
   sdb db ;
   const char* commonCS ;
   sdbCollectionSpace cs ;
   virtual void SetUp() ;

   virtual void TearDown() ;

   BOOLEAN shouldClear() ;

} ;

#endif // TESTBASE_HPP_
