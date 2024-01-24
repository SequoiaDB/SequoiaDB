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
 * @Description seqDB-17129:更新已提交记录与本事务中更新的记录唯一索引重复
 * @author luweikang
 * @date 2019年1月15日
 */
public class Transaction17129 extends SdbTestBase {
    private String clName = "cl17129";
    private String idxName = "idx17129";
    private Sequoiadb sdb = null;
    private DBCollection cl = null;
    private List< BSONObject > expDataList = new ArrayList< BSONObject >();

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        cl = sdb.getCollectionSpace( csName ).createCollection( clName );
    }

    @Test(priority = 1)
    public void testIdIndex1() {
        BSONObject data1 = ( BSONObject ) JSON.parse( "{_id:'id17129_1'}" );
        BSONObject data2 = ( BSONObject ) JSON.parse( "{_id:'id17129_2'}" );
        cl.insert( data1 );
        cl.insert( data2 );

        try {
            TransUtils.beginTransaction( sdb );

            try {
                cl.update( "{_id:'id17129_1'}", "{$set:{_id:'id17129_3'}}",
                        "" );
                cl.update( "{_id:'id17129_2'}", "{$set:{_id:'id17129_3'}}",
                        "" );
                Assert.fail( "Need throw error -38." );
            } catch ( BaseException e ) {
                Assert.assertEquals( e.getErrorCode(), -38, e.getMessage() );
            }

            expDataList.clear();
            expDataList.add( data1 );
            expDataList.add( data2 );
            TransUtils.queryAndCheck( cl, "{_id:1}", "{'':null}", expDataList );
            TransUtils.queryAndCheck( cl, "{_id:1}", "{'':'_id'}",
                    expDataList );

        } finally {
            sdb.rollback();
            cl.delete( "" );
        }
    }

    @Test(priority = 2)
    public void testCommonUniqueIdx1() {
        cl.createIndex( idxName, "{a:1}", true, true );
        BSONObject data1 = ( BSONObject ) JSON.parse( "{_id:1,a:'id17129_1'}" );
        BSONObject data2 = ( BSONObject ) JSON.parse( "{_id:2,a:'id17129_2'}" );
        cl.insert( data1 );
        cl.insert( data2 );

        try {
            TransUtils.beginTransaction( sdb );

            try {
                cl.update( "{a:'id17129_1'}", "{$set:{a:'id17129_3'}}", "" );
                cl.update( "{a:'id17129_2'}", "{$set:{a:'id17129_3'}}", "" );
                Assert.fail( "Need throw error -38." );
            } catch ( BaseException e ) {
                Assert.assertEquals( e.getErrorCode(), -38, e.getMessage() );
            }

            expDataList.clear();
            expDataList.add( data1 );
            expDataList.add( data2 );
            TransUtils.queryAndCheck( cl, "{_id:1}", "{'':null}", expDataList );
            TransUtils.queryAndCheck( cl, "{_id:1}", "{'':'$id'}",
                    expDataList );
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
