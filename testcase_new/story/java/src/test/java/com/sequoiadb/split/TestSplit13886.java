package com.sequoiadb.split;

import java.util.ArrayList;
import java.util.Date;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
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
import com.sequoiadb.testcommon.SdbThreadBase;

/**
 * FileName: TestSplit13886.java test content:split and insert record testlink
 * case:seqDB-13886
 * 
 * @author luweikang
 * @Date 2017.12.20
 * @version 1.00
 */

public class TestSplit13886 extends SdbTestBase {

    private String clName = "split13886";
    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private static DBCollection cl = null;
    String sourceRGName = "";
    String targetRGName = "";

    @BeforeClass
    public void setUp() {

        try {
            sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        } catch ( BaseException e ) {
            Assert.assertFalse( false,
                    "connect %s failed," + coordUrl + e.getMessage() );
        }

        if ( SplitUtils.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }

        if ( SplitUtils.OneGroupMode( sdb ) ) {
            throw new SkipException( "less two groups skip testcase" );
        }

        createCL( sdb );
    }

    @Test
    public void test() throws InterruptedException {
        insertRecord();
        Split split = new Split();
        InsertData insertData = new InsertData();
        split.start();
        insertData.start();

        if ( !split.isSuccess() ) {
            Assert.assertFalse( false, "split was failed!" );
        }
        if ( !insertData.isSuccess() ) {
            Assert.assertFalse( false, "insertData was failed!" );
        }
        checkSplitResult();
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( cs.isCollectionExist( clName ) ) {
                cs.dropCollection( clName );
            }
            sdb.disconnect();
        } catch ( BaseException e ) {
            Assert.assertTrue( false, "clean up failed:" + e.getMessage() );
        } finally {
        }
    }

    private class InsertData extends SdbThreadBase {

        @Override
        public void exec() throws Exception {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                BSONObject obj = null;
                String str = SplitUtils.getRandomString( 1024 );
                try {
                    for ( int i = 0; i < 50000; i++ ) {
                        obj = new BasicBSONObject();
                        obj.put( "a", 10000 + i );
                        obj.put( "b", str + ":newInsert" + i );
                        cl.insert( obj );
                    }
                } catch ( BaseException e ) {
                    Assert.assertFalse( false,
                            "insert record failed:" + e.getMessage() );
                }
            } catch ( BaseException e ) {
                throw e;
            } finally {
                if ( db != null ) {
                    db.disconnect();
                }
            }
        }
    }

    private class Split extends SdbThreadBase {

        @Override
        public void exec() throws Exception {
            Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            try {
                DBCollection cl = db.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( clName );
                sourceRGName = SplitUtils.getSrcGroupName( db,
                        SdbTestBase.csName, clName );
                targetRGName = SplitUtils.getSplitGroupName( sourceRGName );
                for ( int i = 0; i < 10; i++ ) {
                    cl.split( sourceRGName, targetRGName, 50 );
                    Thread.sleep( 100 );
                }
            } catch ( BaseException e ) {
                Assert.assertTrue( false, "split cl failed:" + e.getMessage()
                        + e.getStackTrace() );
            } finally {
                db.disconnect();
            }
        }
    }

    public void insertRecord() {
        try {
            String str = SplitUtils.getRandomString( 1024 );
            ArrayList< BSONObject > list = new ArrayList< BSONObject >();
            BSONObject obj = null;
            for ( int i = 0; i < 10000; i++ ) {
                obj = new BasicBSONObject();
                obj.put( "a", i );
                obj.put( "b", str + i );
                list.add( obj );
            }
            cl.bulkInsert( list, 0 );
        } catch ( Exception e ) {
            Assert.assertTrue( false,
                    "init data failed:" + e.getMessage() + e.getStackTrace() );
        }
    }

    public void createCL( Sequoiadb db ) {
        cs = db.getCollectionSpace( SdbTestBase.csName );
        try {
            if ( cs.isCollectionExist( clName ) ) {
                cs.dropCollection( clName );
            }
            BSONObject options = new BasicBSONObject();
            options = ( BSONObject ) JSON.parse(
                    "{ShardingKey:{a:1},ShardingType:'hash',Partition:4096}" );
            cl = cs.createCollection( clName, options );
        } catch ( BaseException e ) {
            Assert.assertTrue( false, "create cl failed:" + e.getMessage() );
        }
    }

    public void checkSplitResult() {
        DBCursor listCursor = cl.query();
        int count = 0;
        while ( listCursor.hasNext() ) {
            count++;
            listCursor.getNext();
        }
        listCursor.close();

        int allCount = 0;
        Sequoiadb dataDB = null;
        BasicBSONList splitGroupNames = new BasicBSONList();
        splitGroupNames.add( sourceRGName );
        splitGroupNames.add( targetRGName );
        for ( int i = 0; i < splitGroupNames.size(); i++ ) {
            try {
                String nodeName = sdb
                        .getReplicaGroup( ( String ) splitGroupNames.get( i ) )
                        .getMaster().getNodeName();
                dataDB = new Sequoiadb( nodeName, "", "" );
                DBCollection dataCL = dataDB
                        .getCollectionSpace( SdbTestBase.csName )
                        .getCollection( clName );
                DBCursor cursor = dataCL.query();
                int subCount = 0;
                while ( cursor.hasNext() ) {
                    subCount++;
                    cursor.getNext();
                }
                cursor.close();
                allCount += subCount;
            } catch ( BaseException e ) {
                Assert.assertTrue( false, "check split result fail "
                        + e.getErrorCode() + e.getMessage() );
            } finally {
                if ( dataDB != null ) {
                    dataDB.disconnect();
                }
            }
        }
        // sum of query results on each group is equal to the results of coord
        Assert.assertEquals( allCount, count,
                "list lobs error." + "allCount:" + allCount );
    }

}
