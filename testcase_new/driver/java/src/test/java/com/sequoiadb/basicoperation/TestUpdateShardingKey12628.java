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
 * FileName: TestUpdateShardingKey12628.java test interface: update (BSONObject
 * matcher, BSONObject modifier, BSONObject hint, int flag) update (String
 * matcher, String modifier, String hint, int flag)
 * DBQuery.setFlag()=FLG_UPDATE_KEEP_SHARDINGKEY;update( DBQuery query )
 * 
 * @author wuyan
 * @Date 2018.8.30
 * @version 1.00
 */
public class TestUpdateShardingKey12628 extends SdbTestBase {

    private String clName = "cl_12628";
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
        String[] records = { "{_id:1,'no':0,b:0,c:'test0'}",
                "{_id:2,no:{'$numberLong':'300000'},b:1,c:'test1'}",
                "{_id:3,no:[1,2,3],b:2,c:'test2'}" };
        insertDatas( records );
    }

    /**
     * test: 1.update ShardingKey by update (String matcher, String modifier,
     * String hint,int flag) 2.update ShardingKey by update(BSONObject matcher,
     * BSONObject modifier, BSONObject hint, int flag) 3.update ShardingKey by
     * update( DBQuery query )
     */
    @Test(enabled = false)
    public void updateShardingKey() {
        try {
            // update ShardingKey by update (String matcher, String modifier,
            // String hint,int flag)
            BSONObject modifier = new BasicBSONObject();
            BSONObject mValue = new BasicBSONObject();
            BSONObject mValue1 = new BasicBSONObject();
            BSONObject matcher = new BasicBSONObject();
            BSONObject hint = new BasicBSONObject();
            mValue.put( "no", 11 );
            mValue1.put( "c", "testupdatebson" );
            modifier.put( "$inc", mValue );
            modifier.put( "$set", mValue1 );
            matcher.put( "b", 0 );
            hint.put( "", "testIndex" );
            cl.createIndex( "testIndex", "{no:1,c:1}", false, false );
            cl.update( matcher, modifier, hint,
                    DBCollection.FLG_UPDATE_KEEP_SHARDINGKEY );

            // update ShardingKey by update(BSONObject matcher, BSONObject
            // modifier, BSONObject hint, int flag)
            String matcher2 = "{ b : 1 }";
            String modifier2 = "{ '$inc' : { no : 2 }, '$set': { c : 'testupdateJson'}}";
            String hint2 = "{ '' : 'testIndex' }";
            int updateFlag = DBCollection.FLG_UPDATE_KEEP_SHARDINGKEY;
            cl.update( matcher2, modifier2, hint2, updateFlag );

            // update ShardingKey by update( DBQuery query )
            DBQuery updateQuery = new DBQuery();
            BSONObject modifier3 = new BasicBSONObject();
            BSONObject mValue3 = new BasicBSONObject();
            BSONObject matcher3 = new BasicBSONObject();
            mValue3.put( "no.2", 11 );
            modifier3.put( "$inc", mValue3 );
            matcher3.put( "b", 2 );

            updateQuery.setFlag( DBCollection.FLG_UPDATE_KEEP_SHARDINGKEY );
            updateQuery.setMatcher( matcher3 );
            updateQuery.setModifier( modifier3 );
            updateQuery.setHint( hint );
            cl.update( updateQuery );

            // check updateShardingKey results
            String[] expRecords = { "{_id:1,no:11,b:0,c:'testupdatebson'}",
                    "{_id:2,no:{'$numberLong':'300002'},b:1,c:'testupdateJson'}",
                    "{_id:3,no:[1,2,14],b:2,c:'test2'}" };
            checkUpdateRecords( expRecords );
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
        String test = "{ShardingKey:{no:1},ShardingType:'hash'}";
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

    private void checkUpdateRecords( String[] expRecords ) {
        try {
            List< BSONObject > expectedList = new ArrayList< BSONObject >();
            for ( int i = 0; i < expRecords.length; i++ ) {
                BSONObject expRecord = ( BSONObject ) JSON
                        .parse( expRecords[ i ] );
                ;
                expectedList.add( expRecord );
            }

            BSONObject tmp = new BasicBSONObject();
            DBCursor cursor = cl.query( tmp, null, null, null );
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
