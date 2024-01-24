#include "utilList.hpp"
#include <boost/intrusive/list.hpp>
#include "gtest/gtest.h"
using namespace engine;

class utilPartitionListTest : public ::testing::Test
{
public:
   utilPartitionListTest()
   {
       // initialization code here
   }

   void SetUp( ) { 
       // code here will execute just before the test ensues 
   }

   void TearDown( ) { 

       // code here will be called just after the test completes
       // ok to through exceptions from here if need be
   }
} ;

typedef boost::intrusive::list_base_hook< > BaseHook ;

class testClass : public BaseHook
{
public:
   int i ;

   testClass( int num ) : i( num ) {}
} ;

UINT32 randomNum()
{
   static UINT32 i = 0;

   return i++ ;
}

typedef boost::intrusive::list< testClass, boost::intrusive::base_hook<BaseHook> > SUBLIST ;
typedef _utilPartitionList< testClass, SUBLIST, randomNum > partitionList ;

// Test empty list
TEST (utilPartitionListTest, empty)
{
   partitionList list ;

   partitionList::iterator it = list.begin() ;

   ASSERT_EQ( it, list.end() ) ;

   testClass* testObj = new testClass(10) ;
   list.add( testObj ) ;

   ASSERT_EQ( list.size(), 1 ) ;

   list.erase( list.begin() ) ;

   ASSERT_EQ( list.begin(), list.end() ) ;
}

// Test register object
TEST (utilPartitionListTest, insert)
{
   partitionList list ;

   testClass* testObj = new testClass(10) ;
   list.add( testObj ) ;

   ASSERT_EQ( list.size(), 1 ) ;

   UINT32 result = list.begin()->i ;

   ASSERT_EQ( result, 10 ) ;

   list.erase( list.begin() ) ;

   ASSERT_EQ( list.size(), 0 ) ;

   delete testObj ; 
}

// Test register object
TEST (utilPartitionListTest, multiInsert)
{
   partitionList list ;

   testClass* testObj[1000] ;

   for ( UINT32 i = 0; i < 1000; i++ )
   {
      testObj[i] = new testClass(i) ;
      list.add( testObj[i] ) ;
   }

   ASSERT_EQ( list.size(), 1000 ) ;

   UINT32 counter = 0 ;

   partitionList::iterator it = list.begin() ;
   while ( it != list.end() )
   {
      it = list.erase( it ) ;
      counter++ ;
   }

   ASSERT_EQ( counter, 1000 ) ;

   for ( UINT32 i = 0; i < 1000; i++ )
   {
      delete testObj[i] ;
   }
}
/*
// Test iterator. Two active iterators cannot exist in the same block
// as this will lead to hang
TEST (utilPartitionListTest, iteratorHang)
{
   partitionList list ;

   testClass* testObj[1000] ;

   for ( UINT32 i = 0; i < 1000; i++ )
   {
      testObj[i] = new testClass(i) ;
      list.add( testObj[i] ) ;
   }

   ASSERT_EQ( list.size(), 1000 ) ;

   UINT32 counter = 0 ;

   partitionList::iterator it = list.begin() ;

   // this will hang
   partitionList::iterator it2 = list.begin() ;
}
*/
// Test iterator. Invalidating one iterator should allow the other
// iterator to proceed.
TEST (utilPartitionListTest, iteratorNotHang)
{
   partitionList list ;

   testClass* testObj[1000] ;

   for ( UINT32 i = 0; i < 1000; i++ )
   {
      testObj[i] = new testClass(i) ;
      list.add( testObj[i] ) ;
   }

   partitionList::iterator it = list.begin() ;

   it.invalidate() ;
   // this will not hang
   partitionList::iterator it2 = list.begin() ;
}

TEST (utilPartitionListTest, insertDelete )
{
   partitionList list ;

   testClass* testObj[2000] ;

   for ( UINT32 i = 0; i < 1000; i++ )
   {
      testObj[i] = new testClass(i) ;
      list.add( testObj[i] ) ;
   }
   UINT32 counter = 1000 ;

   // erase and insert 100 elements
   for ( int j = 0 ; j < 10; j++ )
   {
      partitionList::iterator it = list.begin() ;
      for ( UINT32 i = 0; i < 100; i++ )
      {
         it = list.erase( it ) ;
      }

      it.invalidate() ;

      for ( UINT32 i = counter; i < counter+100; i++ )
      {
         testObj[i] = new testClass(i) ;
         list.add( testObj[i] ) ;
      }
      counter += 100 ;
   }

   ASSERT_EQ( list.size(), 1000 ) ;

   partitionList::iterator it = list.begin() ;

   while ( it != list.end() )
   {
      it = list.erase( it ) ;
   }

   for ( UINT32 i = 0; i < 2000; i++ )
   {
      delete testObj[i] ;
   }
}

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

