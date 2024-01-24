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
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.transaction.TransUtils;

/**
 * @Description Transaction18221.java query.update事务回滚功能验证
 * @author luweikang
 * @date 2019年1月15日
 */
@Test(groups = "rc")
public class Transaction18221 extends SdbTestBase {

    private String clName = "transCL_18221";
    private Sequoiadb sdb = null;
    private Sequoiadb sdb2 = null;
    private DBCollection cl = null;
    private DBCollection cl2 = null;
    private int startId = 0;
    private int endId = 100;
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
        cl.createIndex( "a", "{a:1, b:1}", false, false );

        expDataList = TransUtils.insertRandomDatas( cl, startId, endId );

    }

    @Test
    public void test() {
        cl2 = sdb2.getCollectionSpace( csName ).getCollection( clName );

        TransUtils.beginTransaction( sdb2 );

        // query.update R1 to R2
        BSONObject update = new BasicBSONObject( "$set",
                new BasicBSONObject( "b", 20000 ) );
        BSONObject orderBy = new BasicBSONObject( "a", 1 );
        DBCursor tbCur = cl2.queryAndUpdate( null, null, orderBy, null, update,
                0, -1, 0, false );
        try {
            List< BSONObject > actList = TransUtils.getReadActList( tbCur );
            Assert.assertEquals( actList, expDataList );
        } finally {
            if ( tbCur != null ) {
                tbCur.close();
            }
        }

        // commit trans
        sdb2.rollback();

        // no trans query
        TransUtils.queryAndCheck( cl, "{a:1}", "{'': null}", expDataList );
        TransUtils.queryAndCheck( cl, "{a:1}", "{'': 'a'}", expDataList );
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
