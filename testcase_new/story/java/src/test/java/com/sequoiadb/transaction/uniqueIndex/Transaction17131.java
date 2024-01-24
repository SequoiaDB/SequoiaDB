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
 * @Description seqDB-17131:更新已提交记录与其他事务插入的记录重复
 * @author luweikang
 * @date 2019年1月15日
 */
public class Transaction17131 extends SdbTestBase {
    private String clName = "cl17131";
    private String idxName = "idx17131";
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
        BSONObject data1 = ( BSONObject ) JSON.parse( "{_id:'id17131_1'}" );
        cl.insert( data1 );

        try {
            TransUtils.beginTransaction( sdb );
            BSONObject data2 = ( BSONObject ) JSON.parse( "{_id:'id17131_2'}" );
            cl.insert( data2 );
            TransUtils.beginTransaction( sdb2 );
            try {
                cl2.update( "{_id:'id17131_1'}", "{$set:{_id:'id17131_2'}}",
                        "" );
                Assert.fail( "Need throw error -38." );
            } catch ( BaseException e ) {
                Assert.assertEquals( e.getErrorCode(), -38, e.getMessage() );
            }

            expDataList.clear();
            expDataList.add( data1 );
            expDataList.add( data2 );
            TransUtils.queryAndCheck( cl2, "{_id:1}", "{'':null}",
                    expDataList );
            TransUtils.queryAndCheck( cl2, "{_id:1}", "{'':'$id'}",
                    expDataList );

            sdb.rollback();

            expDataList.clear();
            expDataList.add( data1 );
            TransUtils.queryAndCheck( cl2, "{'':null}", expDataList );
            TransUtils.queryAndCheck( cl2, "{'':'$id'}", expDataList );
        } finally {
            sdb2.rollback();
            sdb.rollback();
            cl.delete( "" );
        }
    }

    @Test(priority = 2)
    public void testIdIndex2() {
        BSONObject data1 = ( BSONObject ) JSON.parse( "{_id:'id17131_1'}" );
        cl.insert( data1 );

        try {
            TransUtils.beginTransaction( sdb );
            BSONObject data2 = ( BSONObject ) JSON.parse( "{_id:'id17131_2'}" );
            cl.insert( data2 );
            TransUtils.beginTransaction( sdb2 );
            try {
                cl2.update( "{_id:'id17131_1'}", "{$set:{_id:'id17131_2'}}",
                        "" );
                Assert.fail( "Need throw error -38." );
            } catch ( BaseException e ) {
                Assert.assertEquals( e.getErrorCode(), -38, e.getMessage() );
            }

            sdb.commit();

            expDataList.clear();
            expDataList.add( data1 );
            expDataList.add( data2 );
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
        BSONObject data1 = ( BSONObject ) JSON.parse( "{_id:1,a:'id17131_1'}" );
        cl.insert( data1 );

        try {
            TransUtils.beginTransaction( sdb );
            BSONObject data2 = ( BSONObject ) JSON
                    .parse( "{_id:2,a:'id17131_2'}" );
            cl.insert( data2 );
            TransUtils.beginTransaction( sdb2 );
            try {
                cl2.update( "{a:'id17131_1'}", "{$set:{a:'id17131_2'}}", "" );
                Assert.fail( "Need throw error -38." );
            } catch ( BaseException e ) {
                Assert.assertEquals( e.getErrorCode(), -38, e.getMessage() );
            }

            expDataList.clear();
            expDataList.add( data1 );
            expDataList.add( data2 );
            TransUtils.queryAndCheck( cl2, "{a:1}", "{'':null}", expDataList );
            TransUtils.queryAndCheck( cl2, "{a:1}", "{'':'" + idxName + "'}",
                    expDataList );

            sdb.rollback();

            expDataList.clear();
            expDataList.add( data1 );
            TransUtils.queryAndCheck( cl2, "{'':null}", expDataList );
            TransUtils.queryAndCheck( cl2, "{'':'" + idxName + "'}",
                    expDataList );
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
        BSONObject data1 = ( BSONObject ) JSON.parse( "{_id:1,a:'id17131_1'}" );
        cl.insert( data1 );

        try {
            TransUtils.beginTransaction( sdb );
            BSONObject data2 = ( BSONObject ) JSON
                    .parse( "{_id:2,a:'id17131_2'}" );
            cl.insert( data2 );
            TransUtils.beginTransaction( sdb2 );
            try {
                cl2.update( "{a:'id17131_1'}", "{$set:{a:'id17131_2'}}", "" );
                Assert.fail( "Need throw error -38." );
            } catch ( BaseException e ) {
                Assert.assertEquals( e.getErrorCode(), -38, e.getMessage() );
            }

            sdb.commit();

            expDataList.clear();
            expDataList.add( data1 );
            expDataList.add( data2 );
            TransUtils.queryAndCheck( cl2, "{a:1}", "{'':null}", expDataList );
            TransUtils.queryAndCheck( cl2, "{a:1}", "{'':'" + idxName + "'}",
                    expDataList );
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
