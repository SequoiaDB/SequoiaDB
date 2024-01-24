package com.sequoiadb.transaction.rc;

import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
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
 * @Description Transaction18223.java query.update事务回滚功能验证
 * @author luweikang
 * @date 2019年1月15日
 */
@Test(groups = "rc")
public class Transaction18223 extends SdbTestBase {

    private String clName = "transCL_18223";
    private Sequoiadb sdb = null;
    private Sequoiadb sdb2 = null;
    private DBCollection cl = null;
    private DBCollection cl2 = null;
    private int startId = 0;
    private int endId = 1000;
    private int startId2 = 1000;
    private int endId2 = 2000;
    private int decValue = -1000;
    private List< BSONObject > expDataList = null;

    @BeforeClass
    public void setUp() {
        sdb = TransUtils.getRandomSequoiadb( SdbTestBase.testGroup );
        sdb2 = TransUtils.getRandomSequoiadb( SdbTestBase.testGroup );
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
        cl2 = sdb2.getCollectionSpace( csName ).getCollection( clName );

        TransUtils.beginTransaction( sdb2 );

        // trans1 insert R2
        TransUtils.insertDatas( cl2, startId2, endId2, 1000 );

        // query.update R2 to R3
        try {
            BSONObject cond = new BasicBSONObject();
            cond.put( "$gte", startId2 );
            cond.put( "$lt", endId2 );
            BSONObject matcher = new BasicBSONObject( "b", cond );
            BSONObject modifer = new BasicBSONObject( "$inc",
                    new BasicBSONObject( "b", decValue ) );
            DBCursor cur = cl2.queryAndUpdate( matcher, null, null, null,
                    modifer, 0, -1, 0, false );
            List< BSONObject > actList = TransUtils.getReadActList( cur );
            Assert.assertEquals( actList, expDataList );
            Assert.fail( "update records to existing records" );
        } catch ( BaseException e ) {
            Assert.assertEquals( e.getErrorCode(), -38, e.getMessage() );
        }

        // commit trans
        sdb2.commit();

        // no trans query
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
