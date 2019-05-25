#ifndef JIRA_HPP__
#define JIRA_HPP__

#include <iostream>
#include <gtest/gtest.h>

using namespace std;

class jiraTestCase : public testing::Test
{
protected:
   static void SetUpTestCase()
   {
      cout << "in SetUpTestCase" << endl ;
   }
   static void TearDownTestCase()
   {
      cout << "in TearDownTestCase" << endl ;
   }
   virtual void SetUp()
   {
      cout << "in SetUp" << endl ;
   }
   virtual void TearDown()
   {
      cout << "in SetUp" << endl ;
   }
};

#endif // JIRA_HPP__


