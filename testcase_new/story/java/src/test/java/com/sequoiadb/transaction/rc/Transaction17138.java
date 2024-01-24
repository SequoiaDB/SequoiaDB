package com.sequoiadb.transaction.rc;

import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.transaction.TransUtils;

/**
 * @Description Transaction17138.java upsert操作时，事务提交，与读并发
 * @author luweikang
 * @date 2019年1月15日
 */
@Test(groups = "rc")
public class Transaction17138 extends SdbTestBase {

    private String clName = "transCL_17138";
    private Sequoiadb sdb = null;
    private Sequoiadb sdb2 = null;
    private DBCollection cl = null;
    private DBCollection cl2 = null;
    private int recordNum = 100;
    private DBCursor recordCur = null;
    private List< BSONObject > dataList = null;
    private List< BSONObject > expDataList = null;

    @BeforeClass
    public void setUp() {
        sdb = TransUtils.getRandomSequoiadb( SdbTestBase.testGroup );
        sdb2 = TransUtils.getRandomSequoiadb( SdbTestBase.testGroup );
        cl = sdb.getCollectionSpace( csName ).createCollection( clName );
        cl.createIndex( "a", "{a:1}", false, false );
        expDataList = new ArrayList< BSONObject >();

        dataList = prepareData( recordNum );
        cl.insert( dataList );
    }

    @Test
    public void test() {
        cl2 = sdb2.getCollectionSpace( csName ).getCollection( clName );

        TransUtils.beginTransaction( sdb );
        TransUtils.beginTransaction( sdb2 );

        // 2 trans1 upsert record R1 to R2
        BSONObject modifier = null;
        BSONObject data = null;
        for ( int i = 0; i < recordNum * 2; i++ ) {
            modifier = new BasicBSONObject();
            data = new BasicBSONObject();
            data.put( "_id", "upsert17138_" + i );
            data.put( "a", i );
            data.put( "b", "test_update_" + i );
            data.put( "c", 13700000000L );
            data.put( "d", "customer transaction type data application." );
            modifier.put( "$set", data );
            cl.upsert( new BasicBSONObject( "a", i ), modifier, null );
            expDataList.add( data );
        }

        // 3 trans2 select record R1
        TransUtils.queryAndCheck( cl2, "{'': null}", dataList );
        TransUtils.queryAndCheck( cl2, "{'': 'a'}", dataList );

        // 4 commit trans1
        sdb.commit();

        // 5 trans2 select record R1 and R2
        TransUtils.queryAndCheck( cl2, "{a:1}", "{'': null}", expDataList );
        TransUtils.queryAndCheck( cl2, "{a:1}", "{'': 'a'}", expDataList );

        sdb2.commit();
    }

    @AfterClass
    public void tearDown() {
        sdb.commit();
        sdb2.commit();

        sdb.getCollectionSpace( csName ).dropCollection( clName );
        if ( recordCur != null ) {
            recordCur.close();
        }
        if ( sdb != null ) {
            sdb.close();
        }
        if ( sdb2 != null ) {
            sdb2.close();
        }
    }

    private List< BSONObject > prepareData( int recordNum ) {
        List< BSONObject > dataList = new ArrayList< BSONObject >();
        BSONObject data = null;
        for ( int i = 0; i < recordNum; i++ ) {
            data = new BasicBSONObject();
            data.put( "a", i * 2 );
            data.put( "b", "testTrans_17138" );
            data.put( "c", 13700000000L );
            data.put( "d", "customer transaction type data application." );
            dataList.add( data );
        }
        return dataList;
    }

}
