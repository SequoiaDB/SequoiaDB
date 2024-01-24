package com.sequoiadb.lzw;

import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.crud.compress.snappy.SnappyUilts;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @FileName:seqDB-6647:已存在压缩的记录，删除CL后创建同名CL 1、删除已存在压缩数据的CL 2、检查返回结果
 *                                           3、再次创建同名CL并插入数据，检查返回结果
 * @Author linsuqiang
 * @Date 2016-12-27
 * @Version 1.00
 */
public class TestLzw6647 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String clName = "cl_6647";
    private String dataGroupName = null;

    @BeforeClass
    public void setUp() {
        try {
            sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        }
        if ( SnappyUilts.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }
        DBCollection cl = createCL();
        insertData( cl, 1000 );
    }

    @SuppressWarnings("deprecation")
    @AfterClass
    public void tearDown() {
        try {
            CollectionSpace cs = sdb.getCollectionSpace( csName );
            if ( cs.isCollectionExist( clName ) ) {
                cs.dropCollection( clName );
            }
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        } finally {
            if ( sdb != null ) {
                sdb.disconnect();
            }
        }
    }

    @SuppressWarnings("deprecation")
    @Test
    public void test() {
        Sequoiadb db = null;
        try {
            db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            CollectionSpace cs = sdb.getCollectionSpace( csName );
            cs.dropCollection( clName );
            checkDropped();
            DBCollection cl = createCL();
            List< BSONObject > insertedData = insertData( cl, 1000 );
            checkData( insertedData );
        } catch ( BaseException e ) {
            e.printStackTrace();
            Assert.fail( e.getMessage() );
        } finally {
            if ( db != null ) {
                db.disconnect();
            }
        }
    }

    private DBCollection createCL() {
        DBCollection cl = null;
        BSONObject option = new BasicBSONObject();
        try {
            dataGroupName = getDataGroups( sdb ).get( 0 );
            option.put( "Group", dataGroupName );
            option.put( "Compressed", true );
            option.put( "CompressionType", "lzw" );
            CollectionSpace cs = sdb.getCollectionSpace( csName );
            cl = cs.createCollection( clName, option );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        }
        return cl;
    }

    private ArrayList< String > getDataGroups( Sequoiadb sdb ) {
        ArrayList< String > groupList = null;
        try {
            groupList = sdb.getReplicaGroupNames();
            groupList.remove( "SYSCatalogGroup" );
            groupList.remove( "SYSCoord" );
            groupList.remove( "SYSSpare" );
        } catch ( BaseException e ) {
            Assert.assertTrue( false, "getDataGroups fail " + e.getMessage() );
        }
        return groupList;
    }

    @SuppressWarnings("deprecation")
    private List< BSONObject > insertData( DBCollection cl, int recSum ) {
        List< BSONObject > recs = new ArrayList<>();
        for ( int i = 0; i < recSum; i++ ) {
            BSONObject rec = new BasicBSONObject();
            rec.put( "_id", i );
            rec.put( "b", "abcabcabcabcabcabcabcabcabc123123123" );
            recs.add( rec );
        }
        cl.bulkInsert( recs, 0 );
        return recs;
    }

    @SuppressWarnings({ "deprecation", "resource" })
    private void checkDropped() {
        // check on catalog node
        String clNameOpt = "{Name: '" + csName + "." + clName + "'}";
        DBCursor cataSnapshot = sdb.getSnapshot( 8, clNameOpt, null, null );
        if ( cataSnapshot.hasNext() ) {
            Assert.fail( "fail to drop CL. CL info is found in catalog node" );
        }
        cataSnapshot.close();

        int tryTimes = 10;
        boolean isDropped = false;
        String url = sdb.getReplicaGroup( dataGroupName ).getMaster()
                .getNodeName();
        Sequoiadb dataDB = null;
        try {
            dataDB = new Sequoiadb( url, "", "" );
            for ( int i = 0; i < tryTimes; i++ ) {
                // check on data node
                DBCursor dataSnapshot = dataDB.getSnapshot( 4, clNameOpt, null,
                        null );
                if ( !dataSnapshot.hasNext() ) {
                    isDropped = true;
                    break;
                }
                dataSnapshot.close();

                // try again after 1 second
                try {
                    Thread.sleep( 1000 );
                } catch ( InterruptedException e ) {
                    e.printStackTrace();
                }
            }
        } finally {
            if ( dataDB != null ) {
                dataDB.disconnect();
            }
        }
        if ( !isDropped ) {
            Assert.fail( "fail to drop CL. CL info is found in data node" );
        }
    }

    private void checkData( List< BSONObject > expRecs ) {
        DBCollection cl = sdb.getCollectionSpace( csName )
                .getCollection( clName );
        DBCursor cursor = cl.query( null, null, "{_id:1}", null );
        List< BSONObject > actRecs = new ArrayList<>();
        while ( cursor.hasNext() ) {
            actRecs.add( cursor.getNext() );
        }
        cursor.close();
        Assert.assertEquals( actRecs, expRecs, "data is wrong" );
    }
}