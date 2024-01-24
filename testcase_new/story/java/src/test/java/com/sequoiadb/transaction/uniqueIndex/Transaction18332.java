package com.sequoiadb.transaction.uniqueIndex;

import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.transaction.TransUtils;

/**
 * @Description seqDB-18332:更新记录与其他事务多次更新的记录重复
 * @author luweikang
 * @date 2019年1月15日
 */
public class Transaction18332 extends SdbTestBase {
    private String clName = "cl18332";
    private String idxName = "idx18332";
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
        cl.createIndex( idxName, "{a:1}", true, true );
        BSONObject data1 = ( BSONObject ) JSON.parse( "{_id:1,a:1}" );
        BSONObject data2 = ( BSONObject ) JSON.parse( "{_id:2,a:2}" );
        cl.insert( data1 );
        cl.insert( data2 );
        try {
            TransUtils.beginTransaction( sdb );
            for ( int i = 0; i < 3; i++ ) {
                cl.update( "{a:2}", "{$inc:{_id:1}}",
                        "{'':'" + idxName + "'}" );
            }

            // 其他事务更新记录
            TransUtils.beginTransaction( sdb2 );
            cl2.update( "{a:1}", "{$set:{_id:3}}", "{'':'" + idxName + "'}" );

            sdb2.commit();
            sdb.rollback();

            expDataList.clear();
            BSONObject data3 = ( BSONObject ) JSON.parse( "{_id:3,a:1}" );
            expDataList.add( data2 );
            expDataList.add( data3 );
            TransUtils.queryAndCheck( cl, "{_id:1}", "{'':null}", expDataList );
            TransUtils.queryAndCheck( cl, "{_id:1}", "{'':'$id'}",
                    expDataList );

        } finally {
            sdb.rollback();
            sdb2.commit();
            cl.delete( "" );
            cl.dropIndex( idxName );
        }

    }

    @Test(priority = 2)
    public void testIdIndex2() {
        cl.createIndex( idxName, "{a:1}", true, true );
        BSONObject data1 = ( BSONObject ) JSON.parse( "{_id:1,a:1}" );
        BSONObject data2 = ( BSONObject ) JSON.parse( "{_id:2,a:2}" );
        cl.insert( data1 );
        cl.insert( data2 );

        try {
            TransUtils.beginTransaction( sdb );
            for ( int i = 0; i < 3; i++ ) {
                cl.update( "{a:2}", "{$inc:{_id:1}}",
                        "{'':'" + idxName + "'}" );
            }

            // 本事务更新记录
            cl.update( "{a:1}", "{$set:{_id:3}}", "{'':'" + idxName + "'}" );

            sdb.rollback();

            expDataList.clear();
            expDataList.add( data1 );
            expDataList.add( data2 );
            TransUtils.queryAndCheck( cl, "{_id:1}", "{'':null}", expDataList );
            TransUtils.queryAndCheck( cl, "{_id:1}", "{'':'$id'}",
                    expDataList );

        } finally {
            sdb.rollback();
            cl.delete( "" );
            cl.dropIndex( idxName );
        }

    }

    @Test(priority = 3)
    public void testIdIndex3() {
        cl.createIndex( idxName, "{a:1}", true, true );
        BSONObject data1 = ( BSONObject ) JSON.parse( "{_id:1,a:1}" );
        BSONObject data2 = ( BSONObject ) JSON.parse( "{_id:2,a:2}" );
        cl.insert( data1 );
        cl.insert( data2 );

        try {
            TransUtils.beginTransaction( sdb );
            for ( int i = 0; i < 3; i++ ) {
                cl.update( "{a:2}", "{$inc:{_id:1}}",
                        "{'':'" + idxName + "'}" );
            }

            // 非事务更新记录
            cl2.update( "{a:1}", "{$set:{_id:3}}", "{'':'" + idxName + "'}" );

            sdb.rollback();

            expDataList.clear();
            BSONObject data3 = ( BSONObject ) JSON.parse( "{_id:3,a:1}" );
            expDataList.add( data2 );
            expDataList.add( data3 );
            TransUtils.queryAndCheck( cl, "{_id:1}", "{'':null}", expDataList );
            TransUtils.queryAndCheck( cl, "{_id:1}", "{'':'$id'}",
                    expDataList );

        } finally {
            sdb.rollback();
            cl.delete( "" );
            cl.dropIndex( idxName );
        }

    }

    @Test(priority = 4)
    public void testCommonUniqueIdx1() {
        cl.createIndex( idxName, "{a:1}", true, true );
        BSONObject data1 = ( BSONObject ) JSON.parse( "{_id:1,a:1}" );
        BSONObject data2 = ( BSONObject ) JSON.parse( "{_id:2,a:2}" );
        cl.insert( data1 );
        cl.insert( data2 );

        try {
            TransUtils.beginTransaction( sdb );
            for ( int i = 0; i < 3; i++ ) {
                cl.update( "{_id:2}", "{$inc:{a:1}}", "{'':'$id'}" );
            }

            // 其他事务更新记录
            TransUtils.beginTransaction( sdb2 );
            cl2.update( "{_id:1}", "{$set:{a:3}}", "{'':'$id'}" );

            sdb2.commit();
            sdb.rollback();

            expDataList.clear();
            BSONObject data3 = ( BSONObject ) JSON.parse( "{_id:1,a:3}" );
            expDataList.add( data2 );
            expDataList.add( data3 );
            TransUtils.queryAndCheck( cl, "{a:1}", "{'':null}", expDataList );
            TransUtils.queryAndCheck( cl, "{a:1}", "{'':'" + idxName + "'}",
                    expDataList );

        } finally {
            sdb.rollback();
            sdb2.commit();
            cl.delete( "" );
            cl.dropIndex( idxName );
        }

    }

    @Test(priority = 5)
    public void testCommonUniqueIdx2() {
        cl.createIndex( idxName, "{a:1}", true, true );
        BSONObject data1 = ( BSONObject ) JSON.parse( "{_id:1,a:1}" );
        BSONObject data2 = ( BSONObject ) JSON.parse( "{_id:2,a:2}" );
        cl.insert( data1 );
        cl.insert( data2 );

        try {
            TransUtils.beginTransaction( sdb );
            for ( int i = 0; i < 3; i++ ) {
                cl.update( "{_id:2}", "{$inc:{a:1}}", "{'':'$id'}" );
            }

            // 本事务更新记录
            cl.update( "{_id:1}", "{$set:{a:3}}", "{'':'$id'}" );

            sdb.rollback();

            expDataList.clear();
            expDataList.add( data1 );
            expDataList.add( data2 );
            TransUtils.queryAndCheck( cl, "{a:1}", "{'':null}", expDataList );
            TransUtils.queryAndCheck( cl, "{a:1}", "{'':'" + idxName + "'}",
                    expDataList );

        } finally {
            sdb.rollback();
            cl.delete( "" );
            cl.dropIndex( idxName );
        }

    }

    @Test(priority = 6)
    public void testCommonUniqueIdx4() {
        cl.createIndex( idxName, "{a:1}", true, true );
        BSONObject data1 = ( BSONObject ) JSON.parse( "{_id:1,a:1}" );
        BSONObject data2 = ( BSONObject ) JSON.parse( "{_id:2,a:2}" );
        cl.insert( data1 );
        cl.insert( data2 );

        try {
            TransUtils.beginTransaction( sdb );
            for ( int i = 0; i < 3; i++ ) {
                cl.update( "{_id:2}", "{$inc:{a:1}}", "{'':'$id'}" );
            }

            // 非事务更新记录
            cl2.update( "{_id:1}", "{$set:{a:3}}", "{'':'$id'}" );

            sdb.rollback();

            expDataList.clear();
            BSONObject data3 = ( BSONObject ) JSON.parse( "{_id:1,a:3}" );
            expDataList.add( data2 );
            expDataList.add( data3 );
            TransUtils.queryAndCheck( cl, "{a:1}", "{'':null}", expDataList );
            TransUtils.queryAndCheck( cl, "{a:1}", "{'':'" + idxName + "'}",
                    expDataList );

        } finally {
            sdb.rollback();
            cl.delete( "" );
            cl.dropIndex( idxName );
        }

    }

    @AfterClass
    public void tearDown() {
        sdb.getCollectionSpace( csName ).dropCollection( clName );
        sdb.close();
    }

}
