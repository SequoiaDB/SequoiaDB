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
 * @Description seqDB-17136:回滚的记录与其他事务的记录重复
 * @author luweikang
 * @date 2019年1月15日
 */
public class Transaction17136 extends SdbTestBase {
    private String clName = "cl17136";
    private String idxName = "idx17136";
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
        BSONObject data1 = ( BSONObject ) JSON.parse( "{_id:'id17136_1'}" );
        cl.insert( data1 );

        try {
            TransUtils.beginTransaction( sdb );
            cl.delete( "{_id:'id17136_1'}" );

            try {
                TransUtils.beginTransaction( sdb2 );
                cl2.insert( data1 );
                Assert.fail( "Need throw error -38." );
            } catch ( BaseException e ) {
                Assert.assertEquals( e.getErrorCode(), -38 );
            } finally {
                sdb2.rollback();
            }

            expDataList.clear();
            TransUtils.queryAndCheck( cl, "{_id:1}", "{'':null}", expDataList );
            TransUtils.queryAndCheck( cl, "{_id:1}", "{'':'$id'}",
                    expDataList );

            sdb.rollback();

            expDataList.clear();
            expDataList.add( data1 );
            TransUtils.queryAndCheck( cl, "{_id:1}", "{'':null}", expDataList );
            TransUtils.queryAndCheck( cl, "{_id:1}", "{'':'$id'}",
                    expDataList );

        } finally {
            sdb.rollback();
            cl.delete( "" );
        }

    }

    @Test(priority = 2)
    public void testCommonUniqueIdx1() {
        cl.createIndex( idxName, "{a:1}", true, true );
        BSONObject data1 = ( BSONObject ) JSON.parse( "{_id:1,a:'id17136_1'}" );
        cl.insert( data1 );

        try {
            TransUtils.beginTransaction( sdb );
            cl.delete( "{a:'id17136_1'}" );

            try {
                TransUtils.beginTransaction( sdb2 );
                BSONObject data2 = ( BSONObject ) JSON
                        .parse( "{_id:2,a:'id17136_1'}" );
                cl2.insert( data2 );
                Assert.fail( "Need throw error -38." );
            } catch ( BaseException e ) {
                Assert.assertEquals( e.getErrorCode(), -38 );
            } finally {
                sdb2.rollback();
            }

            expDataList.clear();
            TransUtils.queryAndCheck( cl, "{a:1}", "{'':null}", expDataList );
            TransUtils.queryAndCheck( cl, "{a:1}", "{'':'a'}", expDataList );

            sdb.rollback();

            expDataList.clear();
            expDataList.add( data1 );
            TransUtils.queryAndCheck( cl, "{a:1}", "{'':null}", expDataList );
            TransUtils.queryAndCheck( cl, "{a:1}", "{'':'a'}", expDataList );

        } finally {
            sdb.rollback();
            cl.delete( "" );
        }

    }

    @AfterClass
    public void tearDown() {
        sdb.getCollectionSpace( csName ).dropCollection( clName );
        sdb.close();
    }

}
