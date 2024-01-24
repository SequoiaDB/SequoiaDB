package com.sequoiadb.basicoperation;

import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * FileName: TestUpsertShardingKey12629.java test interface: upsert (BSONObject
 * matcher, BSONObject modifier, BSONObject hint, int flag)
 * Flag=FLG_UPDATE_KEEP_SHARDINGKEY
 * 
 * @author wuyan
 * @Date 2018.8.30
 * @version 1.00
 */
public class TestUpsertShardingKey12629 extends SdbTestBase {

    private String clName = "cl_12629";
    private static Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private DBCollection cl;

    // 不支持更新分区键
    @BeforeClass(enabled = false)
    public void setUp() {
        try {
            sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        } catch ( BaseException e ) {
            Assert.assertTrue( false, "connect %s failed,"
                    + SdbTestBase.coordUrl + e.getMessage() );
        }

        if ( Commlib.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }

        createCL();
        String[] records = { "{a:[1,2,'test',[3,4]],no:0,b:0,c:'test0'}",
                "{a:2,no:[1,2,4],b:1,c:'test2'}" };
        insertDatas( records );
    }

    /**
     * test upsert ShardingKey: 1.insert new data when condtion is not matched
     * 2.when matching to the condition,update the corresponding data
     */
    @Test(enabled = false)
    public void upsertShardingKey() {
        try {
            // matching to the condition
            BSONObject modifier = new BasicBSONObject();
            BSONObject mValue = new BasicBSONObject();
            BSONObject mValue1 = new BasicBSONObject();
            BSONObject matcher = new BasicBSONObject();
            BSONObject hint = new BasicBSONObject();
            mValue.put( "no", 11 );
            mValue1.put( "a.3.1", 10 );
            modifier.put( "$inc", mValue );
            modifier.put( "$inc", mValue1 );
            matcher.put( "b", 0 );
            hint.put( "", "testIndex" );
            cl.createIndex( "testIndex", "{no:1,c:1}", false, false );
            cl.upsert( matcher, modifier, hint, null,
                    DBCollection.FLG_UPDATE_KEEP_SHARDINGKEY );

            // no matching to the condition,insert new data
            BSONObject matcher2 = new BasicBSONObject();
            BSONObject modifier2 = new BasicBSONObject();
            BSONObject mValue2 = new BasicBSONObject();
            BSONObject setOnInsert = new BasicBSONObject();
            mValue2.put( "no.a", 10 );
            modifier2.put( "$inc", mValue2 );
            matcher2.put( "b", 10 );
            setOnInsert.put( "a", "testa" );
            // setOnInsert.put("no", 3);
            cl.upsert( matcher2, modifier2, hint, setOnInsert,
                    DBCollection.FLG_UPDATE_KEEP_SHARDINGKEY );

            // check upsertShardingKey results
            String[] expRecords = {
                    "{a:[1,2,'test',[3,14]],no:0,b:0,c:'test0'}",
                    "{a:2,no:[1,2,4],b:1,c:'test2'}",
                    "{ a:'testa',b:10,no:{ a:10}}" };
            checkUpsertRecords( expRecords );
        } catch ( BaseException e ) {
            Assert.assertTrue( false, "update fail " + e.getMessage() );
        }
    }

    @AfterClass(enabled = false)
    public void tearDown() {
        try {
            if ( cs.isCollectionExist( clName ) ) {
                cs.dropCollection( clName );
            }
            sdb.close();
        } catch ( BaseException e ) {
            Assert.assertTrue( false, "clean up failed:" + e.getMessage() );
        }
    }

    private void createCL() {
        try {
            if ( !sdb.isCollectionSpaceExist( SdbTestBase.csName ) ) {
                sdb.createCollectionSpace( SdbTestBase.csName );
            }
        } catch ( BaseException e ) {
            // -33 CS exist,ignore exceptions
            Assert.assertEquals( -33, e.getErrorCode(), e.getMessage() );
        }
        String test = "{ShardingKey:{a:1,no:1},ShardingType:'hash'}";
        BSONObject options = ( BSONObject ) JSON.parse( test );
        try {
            cs = sdb.getCollectionSpace( SdbTestBase.csName );
            cl = cs.createCollection( clName, options );
        } catch ( BaseException e ) {
            Assert.assertTrue( false, "create cl fail " + e.getErrorType() + ":"
                    + e.getMessage() );
        }
    }

    public void insertDatas( String[] records ) {
        // insert records
        for ( int i = 0; i < records.length; i++ ) {
            try {
                cl.insert( records[ i ] );
            } catch ( BaseException e ) {
                Assert.assertTrue( false,
                        "insert Datas fail " + e.getMessage() );
            }
        }
    }

    private void checkUpsertRecords( String[] expRecords ) {
        try {
            List< BSONObject > expectedList = new ArrayList< BSONObject >();
            for ( int i = 0; i < expRecords.length; i++ ) {
                BSONObject expRecord = ( BSONObject ) JSON
                        .parse( expRecords[ i ] );
                ;
                expectedList.add( expRecord );
            }

            DBCursor cursor = cl.query( "", "{_id:{$include:0}}", "{b:1}", "" );
            List< BSONObject > actualList = new ArrayList< BSONObject >();
            while ( cursor.hasNext() ) {
                BSONObject object = cursor.getNext();
                actualList.add( object );
            }
            cursor.close();
            Assert.assertEquals( actualList.equals( expectedList ), true,
                    "check datas are unequal\n" + "actDatas: "
                            + actualList.toString() + "\nexpDatas: "
                            + expectedList.toString() );
        } catch ( BaseException e ) {
            Assert.assertTrue( false,
                    "check update result fail " + e.getMessage() );
        }
    }

}
