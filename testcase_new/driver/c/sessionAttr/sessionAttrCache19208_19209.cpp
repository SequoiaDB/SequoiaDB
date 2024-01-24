/**************************************************************
 * @Description: 测试getSessionAttr/setSessionAttr 
 * seqDB-19208:  获取会话缓存中的事务配置
 * seqDB-19209:  清空会话缓存中的事务配置
 * @Modify    :  liuxiaoxuan 
 *               2019-08-30
 **************************************************************/
#include <client.h>
#include <gtest/gtest.h>
#include <stdlib.h>
#include <stdio.h>
#include "testBase.hpp"
#include "arguments.hpp"

class sessionAttrCache19208_19209 : public testBase
{
protected:
    INT32 transisolation ;
    INT32 transactiontimeout ;

    void SetUp()
    {
        testBase::SetUp() ;
    }

    void TearDown()
    {
        INT32 rc = SDB_OK ;
        bson config ;
        bson_init( &config ) ;
        bson_append_int( &config, "transisolation", transisolation ) ;
        bson_append_int( &config, "transactiontimeout", transactiontimeout ) ;
        bson_finish( &config ) ;
        bson option ;
        bson_init( &option ) ;
        bson_append_bool( &option, "Global", true ) ;
        bson_finish( &option ) ;
        rc = sdbUpdateConfig( db, &config, &option ) ;
        ASSERT_EQ( SDB_OK, rc ) ;
        bson_destroy( &config ) ; 
        bson_destroy( &option) ; 
        testBase::TearDown() ;
    }
} ;

TEST_F( sessionAttrCache19208_19209, getSessionAttr19208 )
{
    INT32 rc = SDB_OK ;
    bson attrObj ;
    bson_init( &attrObj ) ;
    rc = sdbGetSessionAttr( db, &attrObj ) ;
    ASSERT_EQ( SDB_OK, rc ) ;
    
    // get default session attr 
    bson_iterator it ;
    bson_find( &it, &attrObj, "TransIsolation" ) ; 
    transisolation = bson_iterator_int( &it ) ;
    ASSERT_EQ( 0, transisolation ) ;
    bson_find( &it, &attrObj, "TransTimeout" ) ;
    transactiontimeout =  bson_iterator_int( &it ) ;
    ASSERT_EQ( 60, transactiontimeout ) ;
    bson_destroy( &attrObj ) ;
    
    // set session attr
    bson option ;
    bson_init( &option ) ;
    bson_append_int( &option, "TransIsolation", 2 ) ;
    bson_finish( &option ) ;
    rc = sdbSetSessionAttr( db, &option ) ;
    ASSERT_EQ( SDB_OK, rc ) ; 
    bson_destroy( &option ) ;

    // get session attr
    bson_init( &attrObj ) ;
    rc = sdbGetSessionAttr( db, &attrObj ) ;
    ASSERT_EQ( SDB_OK, rc ) ;
 
    bson_find( &it, &attrObj, "TransIsolation" ) ;
    ASSERT_EQ( 2, bson_iterator_int( &it ) ) ;
    bson_find( &it, &attrObj, "TransTimeout" ) ;
    ASSERT_EQ( 60, bson_iterator_int( &it ) ) ;
    bson_destroy( &attrObj ) ;

    // update config
    bson config ;
    bson_init( &config ) ;
    bson_append_int( &config, "transisolation", 0 ) ;
    bson_append_int( &config, "transactiontimeout", 120 ) ;
    bson_finish( &config ) ;
    bson_init( &option ) ;
    bson_append_bool( &option, "Global", true ) ;
    bson_finish( &option ) ;
    rc = sdbUpdateConfig( db, &config, &option ) ;
    ASSERT_EQ( SDB_OK, rc ) ;
    bson_destroy( &config ) ;
    bson_destroy( &option) ;
    
    // get session attr, cache:true
    bson_init( &attrObj ) ;
    rc = sdbGetSessionAttr( db, &attrObj ) ;
    ASSERT_EQ( SDB_OK, rc ) ;
 
    bson_find( &it, &attrObj, "TransIsolation" ) ;
    ASSERT_EQ( 2, bson_iterator_int( &it ) ) ;
    bson_find( &it, &attrObj, "TransTimeout" ) ;
    ASSERT_EQ( 60, bson_iterator_int( &it ) ) ;
    bson_destroy( &attrObj ) ;

    // get session attr, cache:false
    bson_init( &attrObj ) ;
    rc = sdbGetSessionAttrEx( db, false, &attrObj ) ;
    ASSERT_EQ( SDB_OK, rc ) ;
 
    bson_find( &it, &attrObj, "TransIsolation" ) ;
    ASSERT_EQ( 2, bson_iterator_int( &it ) ) ;
    bson_find( &it, &attrObj, "TransTimeout" ) ;
    ASSERT_EQ( 120, bson_iterator_int( &it ) ) ;
    bson_destroy( &attrObj ) ;
}

TEST_F( sessionAttrCache19208_19209, clearSessionAttr19209 )
{
    INT32 rc = SDB_OK ;

    // set session attr
    bson option ;
    bson_init( &option ) ;
    bson_append_int( &option, "TransIsolation", 2 ) ;
    bson_finish( &option ) ;
    rc = sdbSetSessionAttr( db, &option ) ;
    ASSERT_EQ( SDB_OK, rc ) ;

    // get session attr
    bson attrObj ;
    bson_init( &attrObj ) ;
    rc = sdbGetSessionAttr( db, &attrObj ) ;
    ASSERT_EQ( SDB_OK, rc ) ;
 
    bson_iterator it ;
    bson_find( &it, &attrObj, "TransIsolation" ) ;
    ASSERT_EQ( 2, bson_iterator_int( &it ) ) ;
    bson_find( &it, &attrObj, "TransTimeout" ) ;
    ASSERT_EQ( 60, bson_iterator_int( &it ) ) ;
    bson_destroy( &attrObj ) ;
    bson_destroy( &option ) ;
  
    // update config
    bson config ;
    bson_init( &config ) ;
    bson_append_int( &config, "transisolation", 0 ) ;
    bson_append_int( &config, "transactiontimeout", 120 ) ;
    bson_finish( &config ) ;
    bson_init( &option ) ;
    bson_append_bool( &option, "Global", true ) ;
    bson_finish( &option ) ;
    rc = sdbUpdateConfig( db, &config, &option ) ;
    ASSERT_EQ( SDB_OK, rc ) ;
    bson_destroy( &config ) ;
    bson_destroy( &option) ;

    // get session attr
    bson_init( &attrObj ) ;
    rc = sdbGetSessionAttr( db, &attrObj ) ;
    ASSERT_EQ( SDB_OK, rc ) ;
    bson_print( &attrObj ) ;

    bson_find( &it, &attrObj, "TransIsolation" ) ;
    ASSERT_EQ( 2, bson_iterator_int( &it ) ) ;
    bson_find( &it, &attrObj, "TransTimeout" ) ;
    ASSERT_EQ( 60, bson_iterator_int( &it ) ) ;
    bson_destroy( &attrObj ) ;

    // clear cache
    bson empty ;
    bson_init( &empty ) ;
    bson_finish( &empty ) ;
    rc = sdbSetSessionAttr( db, &empty ) ;
    ASSERT_EQ( SDB_OK, rc ) ;
    bson_destroy( &option ) ;
    bson_destroy( &empty ) ;

    // get session attr, cache:true
    bson_init( &attrObj ) ;
    rc = sdbGetSessionAttr( db, &attrObj ) ;
    ASSERT_EQ( SDB_OK, rc ) ;

    bson_find( &it, &attrObj, "TransIsolation" ) ;
    ASSERT_EQ( 2, bson_iterator_int( &it ) ) ;
    bson_find( &it, &attrObj, "TransTimeout" ) ;
    ASSERT_EQ( 120, bson_iterator_int( &it ) ) ;
    bson_destroy( &attrObj ) ;

    // get session attr, cache:false
    bson_init( &attrObj ) ;
    rc = sdbGetSessionAttrEx( db, false, &attrObj ) ;
    ASSERT_EQ( SDB_OK, rc ) ;

    bson_find( &it, &attrObj, "TransIsolation" ) ;
    ASSERT_EQ( 2, bson_iterator_int( &it ) ) ;
    bson_find( &it, &attrObj, "TransTimeout" ) ;
    ASSERT_EQ( 120, bson_iterator_int( &it ) ) ;
    bson_destroy( &attrObj ) ;   
}
