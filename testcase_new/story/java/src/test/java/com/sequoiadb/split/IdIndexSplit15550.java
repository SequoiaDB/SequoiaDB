package com.sequoiadb.split;

import java.util.ArrayList;
import java.util.List;
import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;
import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

/**
 * @FileName: seqDB-15550:创建id索引与切分并发 1、向cl中插入数据记录 2、执行split，设置切分条件
 *            3、切分过程中创建id索引 4、查看切分和创建id索引结果
 * @author zhaoxiaoni
 * @Date 2018/8/17
 * @version 3.00
 *
 */
public class IdIndexSplit15550 extends SdbTestBase {

    private Sequoiadb sdb = null;
    private List< String > groupNames = null;
    private String clName = "cl_15550";
    private String desGroup;
    private String srcGroup;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( coordUrl, "", "" );
        // 跳过standAlone和数据组不足的情况
        CommLib commLib = new CommLib();
        if ( commLib.isStandAlone( sdb ) ) {
            throw new SkipException( "Skip StandAlone" );
        }
        groupNames = commLib.getDataGroupNames( sdb );
        if ( groupNames.size() < 2 ) {
            throw new SkipException( "Only one group!" );
        }

        srcGroup = groupNames.get( 0 );
        desGroup = groupNames.get( 1 );

        CollectionSpace cs = sdb.getCollectionSpace( csName );
        DBCollection cl = cs.createCollection( clName,
                ( BSONObject ) JSON.parse(
                        "{ShardingKey:{sk:1},ShardingType:'range',AutoIndexId:false,Group:'"
                                + srcGroup + "'}" ) );

        insertData( cl );
    }

    @Test
    public void SplitAndCreate() {
        Sequoiadb db = null;
        Split split = null;
        Create create = null;
        DBCollection cl = null;
        try {
            db = new Sequoiadb( coordUrl, "", "" );
            cl = db.getCollectionSpace( csName ).getCollection( clName );

            create = new Create();
            create.start();

            split = new Split();
            split.start();

            Assert.assertEquals( create.isSuccess(), true );
            Assert.assertEquals( split.isSuccess(), true );

            // 校验索引
            cl.getIndexInfo( "$id" );
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
                // 校验源组和目标组的数据
                checkData( 4500, "{sk:{$gte:500,$lt:5000}}", desGroup );
                checkData( 500, "{sk:{$gte:0,$lt:500}}", srcGroup );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() == -279 || e.getErrorCode() == -147 ) {
                    checkData( 5000, "{sk:{$gte:0,$lt:5000}}", srcGroup );
                } else {
                    throw e;
                }
            } finally {
                if ( db != null ) {
                    db.close();
                }
            }
        }
    }

    private class Create extends SdbThreadBase {

        @Override
        public void exec() throws Exception {
            // TODO Auto-generated method stub
            Sequoiadb db = null;
            DBCollection cl = null;
            try {
                db = new Sequoiadb( coordUrl, "", "" );
                cl = db.getCollectionSpace( csName ).getCollection( clName );
                cl.createIdIndex(
                        ( BSONObject ) JSON.parse( "{SortBufferSize:128}" ) );
            } catch ( BaseException e ) {
                throw e;
            } finally {
                if ( db != null ) {
                    db.close();
                }
            }
        }
    }

    private void checkData( int expectedCount, String matcher, String group ) {
        Sequoiadb dataDb = null;
        DBCollection cl = null;
        long count;
        try {
            dataDb = sdb.getReplicaGroup( group ).getMaster().connect();
            cl = dataDb.getCollectionSpace( csName ).getCollection( clName );
            count = cl.getCount( matcher );
            Assert.assertEquals( count, expectedCount );
        } catch ( BaseException e ) {
            throw e;
        } finally {
            if ( dataDb != null ) {
                dataDb.close();
            }
        }
    }
}
