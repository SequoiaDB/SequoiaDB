package com.sequoiadb.transaction.uniqueIndex;

import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.transaction.TransUtils;

/**
 * @Description seqDB-17132:更新已提交记录与其他事务更新的记录重复
 * @author luweikang
 * @date 2019年1月15日
 */
public class Transaction17132 extends SdbTestBase {
    private String clName = "cl17132";
    private String idxName = "idx17132";
    private Sequoiadb sdb = null;
    private Sequoiadb sdb2 = null;
    private DBCollection cl = null;
    private DBCollection cl2 = null;
    private List< BSONObject > expDataList = new ArrayList< BSONObject >();

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        cl = sdb.getCollectionSpace( csName ).createCollection( clName );

        sdb2 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        cl2 = sdb2.getCollectionSpace( csName ).getCollection( clName );
    }

    @Test(priority = 1)
    public void testIdIndex1() {
        BSONObject data1 = ( BSONObject ) JSON.parse( "{_id:'id17132_1'}" );
        BSONObject data2 = ( BSONObject ) JSON.parse( "{_id:'id17132_2'}" );
        cl.insert( data1 );
        cl.insert( data2 );

        try {
            TransUtils.beginTransaction( sdb );
            cl.update( "{_id:'id17132_1'}", "{$set:{_id:'id17132_3'}}", "" );
            TransUtils.beginTransaction( sdb2 );
            try {
                cl2.update( "{_id:'id17132_2'}", "{$set:{_id:'id17132_3'}}",
                        "" );
                Assert.fail( "Need throw error -38." );
            } catch ( BaseException e ) {
                Assert.assertEquals( e.getErrorCode(), -38, e.getMessage() );
            }

            expDataList.clear();
            BSONObject data3 = ( BSONObject ) JSON.parse( "{_id:'id17132_3'}" );
            expDataList.add( data2 );
            expDataList.add( data3 );
            TransUtils.queryAndCheck( cl2, "{_id:1}", "{'':null}",
                    expDataList );
            TransUtils.queryAndCheck( cl2, "{_id:1}", "{'':'$id'}",
                    expDataList );

            sdb.rollback();

            expDataList.clear();
            expDataList.add( data1 );
            expDataList.add( data2 );
            TransUtils.queryAndCheck( cl2, "{_id:1}", "{'':null}",
                    expDataList );
            TransUtils.queryAndCheck( cl2, "{_id:1}", "{'':'$id'}",
                    expDataList );
        } finally {
            sdb2.rollback();
            sdb.rollback();
            cl.delete( "" );
        }
    }

    @Test(priority = 2)
    public void testIdIndex2() {
        BSONObject data1 = ( BSONObject ) JSON.parse( "{_id:'id17132_1'}" );
        BSONObject data2 = ( BSONObject ) JSON.parse( "{_id:'id17132_2'}" );
        cl.insert( data1 );
        cl.insert( data2 );

        try {
            TransUtils.beginTransaction( sdb );
            cl.update( "{_id:'id17132_1'}", "{$set:{_id:'id17132_3'}}", "" );
            TransUtils.beginTransaction( sdb2 );
            try {
                cl2.update( "{_id:'id17132_2'}", "{$set:{_id:'id17132_3'}}",
                        "" );
                Assert.fail( "Need throw error -38." );
            } catch ( BaseException e ) {
                Assert.assertEquals( e.getErrorCode(), -38, e.getMessage() );
            }

            sdb.commit();

            expDataList.clear();
            expDataList.add( data2 );
            BSONObject data3 = ( BSONObject ) JSON.parse( "{_id:'id17132_3'}" );
            expDataList.add( data3 );
            TransUtils.queryAndCheck( cl2, "{_id:1}", "{'':null}",
                    expDataList );
            TransUtils.queryAndCheck( cl2, "{_id:1}", "{'':'$id'}",
                    expDataList );
        } finally {
            sdb2.commit();
            sdb.commit();
            cl.delete( "" );
        }
    }

    @Test(priority = 3)
    public void testCommonUniqueIdx1() {
        cl.createIndex( idxName, "{a:1}", true, true );
        BSONObject data1 = ( BSONObject ) JSON.parse( "{_id:1,a:'id17132_1'}" );
        BSONObject data2 = ( BSONObject ) JSON.parse( "{_id:2,a:'id17132_2'}" );
        cl.insert( data1 );
        cl.insert( data2 );

        try {
            TransUtils.beginTransaction( sdb );
            cl.update( "{a:'id17132_1'}", "{$set:{a:'id17132_3'}}", "" );
            TransUtils.beginTransaction( sdb2 );
            try {
                cl2.update( "{a:'id17132_2'}", "{$set:{a:'id17132_3'}}", "" );
                Assert.fail( "Need throw error -38." );
            } catch ( BaseException e ) {
                Assert.assertEquals( e.getErrorCode(), -38, e.getMessage() );
            }

            expDataList.clear();
            BSONObject data3 = ( BSONObject ) JSON
                    .parse( "{_id:1,a:'id17132_3'}" );
            expDataList.add( data2 );
            expDataList.add( data3 );
            TransUtils.queryAndCheck( cl2, "{a:1}", "{'':null}", expDataList );
            TransUtils.queryAndCheck( cl2, "{a:1}", "{'':'a'}", expDataList );

            sdb.rollback();

            expDataList.clear();
            expDataList.add( data1 );
            expDataList.add( data2 );
            TransUtils.queryAndCheck( cl2, "{a:1}", "{'':null}", expDataList );
            TransUtils.queryAndCheck( cl2, "{a:1}", "{'':'a'}", expDataList );
        } finally {
            sdb2.rollback();
            sdb.rollback();
            cl.delete( "" );
            cl.dropIndex( idxName );
        }
    }

    @Test(priority = 4)
    public void testCommonUniqueIdx2() {
        cl.createIndex( idxName, "{a:1}", true, true );
        BSONObject data1 = ( BSONObject ) JSON.parse( "{_id:1,a:'id17132_1'}" );
        BSONObject data2 = ( BSONObject ) JSON.parse( "{_id:2,a:'id17132_2'}" );
        cl.insert( data1 );
        cl.insert( data2 );

        try {
            TransUtils.beginTransaction( sdb );
            cl.update( "{a:'id17132_1'}", "{$set:{a:'id17132_3'}}", "" );
            TransUtils.beginTransaction( sdb2 );
            try {
                cl2.update( "{a:'id17132_2'}", "{$set:{a:'id17132_3'}}", "" );
                Assert.fail( "Need throw error -38." );
            } catch ( BaseException e ) {
                Assert.assertEquals( e.getErrorCode(), -38, e.getMessage() );
            }

            sdb.commit();

            expDataList.clear();
            expDataList.add( data2 );
            BSONObject data3 = ( BSONObject ) JSON
                    .parse( "{_id:1,a:'id17132_3'}" );
            expDataList.add( data3 );
            TransUtils.queryAndCheck( cl2, "{a:1}", "{'':null}", expDataList );
            TransUtils.queryAndCheck( cl2, "{a:1}", "{'':'a'}", expDataList );
        } finally {
            sdb2.commit();
            sdb.commit();
            cl.delete( "" );
            cl.dropIndex( idxName );
        }
    }

    @AfterClass
    public void tearDown() {
        sdb.getCollectionSpace( csName ).dropCollection( clName );
        sdb.close();
        sdb2.close();
    }

}
