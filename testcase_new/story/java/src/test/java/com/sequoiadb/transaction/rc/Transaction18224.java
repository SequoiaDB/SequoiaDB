package com.sequoiadb.transaction.rc;

import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.transaction.TransUtils;

/**
 * @Description Transaction18224.java query.update事务回滚功能验证
 * @author luweikang
 * @date 2019年1月15日
 */
@Test(groups = "rc")
public class Transaction18224 extends SdbTestBase {

    private String clName = "transCL_18224";
    private Sequoiadb sdb = null;
    private Sequoiadb sdb1 = null;
    private Sequoiadb sdb2 = null;
    private DBCollection cl = null;
    private DBCollection cl1 = null;
    private DBCollection cl2 = null;
    private int startId = 0;
    private int endId = 100;
    private int startId2 = 100;
    private int endId2 = 200;
    private int incValue = 10000;
    private List< BSONObject > expDataList = null;

    @BeforeClass
    public void setUp() {
        sdb = TransUtils.getRandomSequoiadb( SdbTestBase.testGroup );
        sdb1 = TransUtils.getRandomSequoiadb( SdbTestBase.testGroup );
        sdb2 = TransUtils.getRandomSequoiadb( SdbTestBase.testGroup );
        sdb1.setSessionAttr( ( BSONObject ) JSON.parse(
                "{TransTimeout:" + TransUtils.transTimeoutSession + "}" ) );
        sdb2.setSessionAttr( ( BSONObject ) JSON.parse(
                "{TransTimeout:" + TransUtils.transTimeoutSession + "}" ) );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "skip standalone" );
        }
        List< String > groupNameList = CommLib.getDataGroupNames( sdb );
        if ( groupNameList.size() < 2 ) {
            throw new SkipException( "skip group size less than 2" );
        }

        BSONObject clOptions = new BasicBSONObject();
        clOptions.put( "ShardingKey", new BasicBSONObject( "a", 1 ) );
        clOptions.put( "ShardingType", "hash" );
        clOptions.put( "AutoSplit", true );
        cl = sdb.getCollectionSpace( csName ).createCollection( clName,
                clOptions );
        cl.createIndex( "a", "{a:1, b:1}", true, false );

        expDataList = TransUtils.insertDatas( cl, startId, endId, 1000 );
    }

    @Test
    public void test() {
        cl1 = sdb1.getCollectionSpace( csName ).getCollection( clName );
        cl2 = sdb2.getCollectionSpace( csName ).getCollection( clName );

        TransUtils.beginTransaction( sdb1 );
        TransUtils.beginTransaction( sdb2 );

        // trans1 update R1 to R2
        cl1.update( "{'a': 1000}",
                "{'$inc': {'_id': " + incValue + ", 'b': " + incValue + "}}",
                null );

        try {
            // trans2 insert R3
            expDataList = TransUtils.insertDatas( cl2, startId2, endId2, 2000 );

            // trans2 query.remove R1
            DBCursor cur = cl2.queryAndRemove( new BasicBSONObject( "a", 1000 ),
                    null, null, null, 0, -1, 0 );
            List< BSONObject > actList = TransUtils.getReadActList( cur );
            Assert.assertEquals( actList, expDataList );
            Assert.fail( "delete locked records shuold be error." );
        } catch ( BaseException e ) {
            Assert.assertEquals( e.getErrorCode(), -13, e.getMessage() );
        }

        // commit trans
        sdb1.commit();
        sdb2.commit();

        // no trans query
        expDataList.clear();
        expDataList = TransUtils.getUpdateDatas( startId + incValue,
                endId + incValue, 1000 );

        TransUtils.queryAndCheck( cl, "{ _id: 1}", "{'': null}", expDataList );
        TransUtils.queryAndCheck( cl, "{ _id: 1}", "{'': 'a'}", expDataList );
    }

    @AfterClass
    public void tearDown() {
        sdb2.commit();

        sdb.getCollectionSpace( csName ).dropCollection( clName );
        if ( sdb != null ) {
            sdb.close();
        }
        if ( sdb2 != null ) {
            sdb2.close();
        }
    }

}
