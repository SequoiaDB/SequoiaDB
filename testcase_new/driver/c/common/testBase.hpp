/**************************************************************
 * @Description: test base class
 * @Modify     : Liang xuewang 
 *               2017-09-17
 ***************************************************************/
#ifndef TESTBASE_HPP_
#define TESTBASE_HPP_

#include <gtest/gtest.h>
#include <client.h>
#include "arguments.hpp"

class testBase : public testing::Test
{
protected:
   sdbConnectionHandle db ;

   virtual void SetUp() ;

   virtual void TearDown() ;

   BOOLEAN shouldClear() ;

private:
   BOOLEAN forceClear ;
} ;

#endif // TESTBASE_HPP_
