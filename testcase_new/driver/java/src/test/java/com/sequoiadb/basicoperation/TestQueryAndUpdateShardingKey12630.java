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
import com.sequoiadb.base.DBQuery;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * FileName: TestQueryAndUpdateShardingKey12630.java test interface:
 * queryAndUpdate( BSONObject matcher,
 * 
 * BSONObject selector,
 * 
 * BSONObject orderBy,
 * 
 * BSONObject hint,
 * 
 * BSONObject update,
 * 
 * long skipRows,
 * 
 * long returnRows,
 * 
 * int flag,
 * 
 * boolean returnNew ) Flag=FLG_UPDATE_KEEP_SHARDINGKEY
 * 
 * @author wuyan
 * @Date 2018.8.30
 * @version 1.00
 */
public class TestQueryAndUpdateShardingKey12630 extends SdbTestBase {

    private String clName = "cl_12630";
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
        String[] records = { "{a:[1,2,'test',[3,4]],no:2,b:0,c:'test0'}",
                "{a:2,no:3,b:1,c:'test2'}" };
        insertDatas( records );
    }

    /**
     * test queryAndUpdate ShardingKey
     */
    @Test(enabled = false)
    public void queryAndUpdateShardingKey() {
        try {

            BSONObject selector = new BasicBSONObject();
            BSONObject sValue = new BasicBSONObject();
            BSONObject orderBy = new BasicBSONObject();
            BSONObject matcher = new BasicBSONObject();
            BSONObject hint = new BasicBSONObject();
            BSONObject update = new BasicBSONObject();
            BSONObject uValue = new BasicBSONObject();
            sValue.put( "$include", 0 );
            selector.put( "_id", sValue );
            uValue.put( "no", 11 );
            update.put( "$inc", uValue );
            matcher.put( "b", 1 );

            cl.createIndex( "testIndex", "{no:1,c:1}", false, false );
            hint.put( "", "testIndex" );
            int skipRows = 0;
            int returnRows = -1;
            int flag = DBQuery.FLG_QUERY_KEEP_SHARDINGKEY_IN_UPDATE;

            DBCursor dbCursor = cl.queryAndUpdate( matcher, selector, orderBy,
                    hint, update, skipRows, returnRows, flag, true );
            List< BSONObject > actualList = new ArrayList< BSONObject >();
            while ( dbCursor.hasNext() ) {
                BSONObject obj = dbCursor.getNext();
                actualList.add( obj );
            }
            dbCursor.close();

            // check update results
            String[] expRecords = { "{a:[1,2,'test',[3,4]],no:2,b:0,c:'test0'}",
                    "{a:2,no:14,b:1,c:'test2'}" };
            checkUpsertRecords( expRecords );

        } catch ( BaseException e ) {
            Assert.assertTrue( false,
                    "queryAndUpdate shardingKey fail " + e.getMessage() );
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
