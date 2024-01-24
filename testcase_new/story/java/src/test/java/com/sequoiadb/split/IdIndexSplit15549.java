package com.sequoiadb.split;

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
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

/**
 * @FileName: seqDB-15549:切分时，修改AutoIndexId属性 1、向cl中插入数据记录 2、执行split，设置切分条件
 *            3、切分过程中修改AutoIndexId属性 4、查看切分和删除id索引结果
 * @author zhaoxiaoni
 * @Date 2018/8/17
 * @version 3.00
 *
 */

public class IdIndexSplit15549 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String clName = "cl_15549";
    private List< String > groupNames;
    private String srcGroup;
    private String desGroup;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( coordUrl, "", "" );

        CommLib commlib = new CommLib();
        if ( commlib.isStandAlone( sdb ) ) {
            throw new SkipException( "StandAlone environment!" );
        }

        groupNames = commlib.getDataGroupNames( sdb );
        if ( groupNames.size() < 2 ) {
            throw new SkipException(
                    "Current environment less than tow groups! " );
        }
        srcGroup = groupNames.get( 0 );
        desGroup = groupNames.get( 1 );

        CollectionSpace cs = sdb.getCollectionSpace( csName );
        DBCollection cl = cs.createCollection( clName,
                ( BSONObject ) JSON.parse(
                        "{ShardingKey:{sk:1},ShardingType:'range',Group:'"
                                + srcGroup + "'}" ) );

        insertData( cl );
    }

    @Test
    public void splitAndAlter() {
        Sequoiadb db = null;
        DBCollection cl = null;
        DBCursor cursor = null;
        try {
            db = new Sequoiadb( coordUrl, "", "" );
            cl = db.getCollectionSpace( csName ).getCollection( clName );

            Split splitThread = new Split();
            splitThread.start();

            int timeOut = 60;
            int doTimes = 0;
            while ( doTimes < timeOut ) {
                BasicBSONObject matcher = new BasicBSONObject();
                matcher.put( "Name", csName + "." + clName );
                matcher.put( "Status", new BasicBSONObject( "$ne", 9 ) );
                cursor = db.listTasks( matcher, null, null, null );
                if ( cursor.hasNext() ) {
                    try {
                        cl.alterCollection( ( BSONObject ) JSON
                                .parse( "{AutoIndexId:false}" ) );
                        Assert.fail( "Alter collection should not succeed!" );
                    } catch ( BaseException e ) {
                        if ( e.getErrorCode() != -334 ) {
                            throw e;
                        } else {
                            break;
                        }
                    }
                } else {
                    doTimes++;
                    try {
                        Thread.sleep( 1000 );
                    } catch ( InterruptedException e1 ) {
                        // TODO Auto-generated catch block
                        e1.printStackTrace();
                    }
                    continue;
                }
            }

            if ( !splitThread.isSuccess() ) {
                splitThread.getExceptions().get( 0 ).printStackTrace();
                Assert.fail( splitThread.getErrorMsg() );
            }

            checkData( 4500, "{sk:{$gte:500,$lt:5000}}", desGroup );
            checkData( 500, "{sk:{$gte:0,$lt:500}}", srcGroup );
        } catch ( BaseException e ) {
            throw e;
        } finally {
            if ( db != null ) {
                db.close();
            }
        }
    }

    @AfterClass
    public void tearDown() {
        CollectionSpace cs = null;
        try {
            cs = sdb.getCollectionSpace( csName );
            cs.dropCollection( clName );
        } catch ( BaseException e ) {
            throw e;
        } finally {
            if ( sdb != null ) {
                sdb.close();
                ;
            }
        }
    }

    private void insertData( DBCollection cl ) {
        // TODO Auto-generated method stub
        List< BSONObject > list = new ArrayList< BSONObject >();
        for ( int i = 0; i < 5000; i++ ) {
            BSONObject obj = ( BSONObject ) JSON.parse( "{sk:" + i + "}" );
            list.add( obj );
        }
        cl.insert( list );
    }

    private class Split extends SdbThreadBase {

        @Override
        public void exec() throws Exception {
            // TODO Auto-generated method stub
            Sequoiadb db = null;
            DBCollection cl = null;
            try {
                db = new Sequoiadb( coordUrl, "", "" );
                cl = db.getCollectionSpace( csName ).getCollection( clName );
                cl.split( srcGroup, desGroup, 90 );
            } catch ( BaseException e ) {
                throw e;
            } finally {
                if ( db != null ) {
                    db.close();
                }
            }
        }
    }

    private void checkData( int expectedCount, String macher, String group ) {
        Sequoiadb dateDb = null;
        DBCollection cl = null;
        long count;
        try {
            dateDb = sdb.getReplicaGroup( group ).getMaster().connect();
            cl = dateDb.getCollectionSpace( csName ).getCollection( clName );
            count = cl.getCount( macher );
            Assert.assertEquals( count, expectedCount );
        } catch ( BaseException e ) {
            throw e;
        } finally {
            if ( dateDb != null ) {
                dateDb.close();
            }
        }
    }
}