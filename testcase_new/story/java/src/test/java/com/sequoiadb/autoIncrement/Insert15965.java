package com.sequoiadb.autoIncrement;

/**
 * @FileName:seqDB-15965：自增字段同时时shardKey，不指定自增字段插入与切分并发
 * 预置条件:集合已存在，ShardingKey同时是自增字段
 * 测试步骤：不指定自增字段插入的同时，执行切分
 * 预期结果：记录插入成功，切分执行成功，自增字段值正确，记录落在正确的数据组上
 * @Author zhaoyu
 * @Date 2018-11-01
 * @Version 1.00
 */
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
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

public class Insert15965 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private CollectionSpace scs = null;
    private DBCollection scl = null;
    private String clName = "cl_15965";
    private String indexName = "idIndex";
    private int initDataNum = 50000;
    private int threadNum = 5;
    private long threadInsertNum = 10000;
    private int splitStartValue = 50000;
    private List< String > coordNodes = null;
    private List< String > groupNames = null;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "skip StandAlone" );
        }
        coordNodes = CommLib.getNodeAddress( sdb, "SYSCoord" );
        if ( coordNodes.size() < 2 ) {
            throw new SkipException( "skip one coordNode" );
        }
        groupNames = CommLib.getDataGroupNames( sdb );
        if ( groupNames.size() < 2 ) {
            throw new SkipException( "skip one data group" );
        }
        scs = sdb.getCollectionSpace( csName );
        scl = scs.createCollection( clName, ( BSONObject ) JSON
                .parse( "{AutoIncrement:{Field:'id', AcquireSize:1}, "
                        + "ShardingKey:{'id':1}, ShardingType:'range', EnsureShardingIndex:false,"
                        + "Group:'" + groupNames.get( 0 ) + "'}" ) );
        scl.createIndex( indexName, "{id:1}", true, true );
        insertData( scl, initDataNum );
    }

    @AfterClass
    public void tearDown() {
        scs.dropCollection( clName );
        sdb.close();
    }

    @Test
    public void test() {
        InsertThread insertThread = new InsertThread( coordNodes.get( 0 ) );
        SplitThread splitThread = new SplitThread( coordNodes.get( 1 ) );
        insertThread.start( threadNum );
        splitThread.start();
        if ( !( insertThread.isSuccess() && splitThread.isSuccess() ) ) {
            Assert.fail(
                    insertThread.getErrorMsg() + splitThread.getErrorMsg() );
        }

        checkResult( scl, initDataNum + threadInsertNum * threadNum );

    }

    private class InsertThread extends SdbThreadBase {
        private String coordNode;

        public InsertThread( String coordNode ) {
            super();
            this.coordNode = coordNode;
        }

        @Override
        public void exec() throws BaseException {
            try ( Sequoiadb db = new Sequoiadb( coordNode, "", "" )) {
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                for ( int i = 0; i < threadInsertNum; i++ ) {
                    BSONObject obj = ( BSONObject ) JSON
                            .parse( "{a:" + i + "}" );
                    cl.insert( obj );
                }
            }

        }
    }

    private class SplitThread extends SdbThreadBase {
        private String coordNode;

        public SplitThread( String coordNode ) {
            super();
            this.coordNode = coordNode;
        }

        @Override
        public void exec() throws BaseException {
            try ( Sequoiadb db = new Sequoiadb( coordNode, "", "" )) {
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                cl.split( groupNames.get( 0 ), groupNames.get( 1 ),
                        ( BSONObject ) JSON
                                .parse( "{id:" + splitStartValue + "}" ),
                        null );
            }

        }
    }

    public void checkResult( DBCollection cl, long expectNum ) {
        // 校验记录数
        long count = ( long ) cl.getCount();
        Assert.assertEquals( count, expectNum );

        // 按照切分条件查询各数据组上的记录之和等于记录总数
        Sequoiadb srcDataMaster = sdb.getReplicaGroup( groupNames.get( 0 ) )
                .getMaster().connect();
        long srcDataNum = ( long ) srcDataMaster.getCollectionSpace( csName )
                .getCollection( clName )
                .getCount( "{id:{$lt:" + splitStartValue + "}}" );
        Sequoiadb desDataMaster = sdb.getReplicaGroup( groupNames.get( 1 ) )
                .getMaster().connect();
        long desDataNum = ( long ) desDataMaster.getCollectionSpace( csName )
                .getCollection( clName )
                .getCount( "{id:{$gte:" + splitStartValue + "}}" );

        Assert.assertEquals( srcDataNum + desDataNum, expectNum );
    }

    public void insertData( DBCollection cl, int insertNum ) {
        ArrayList< BSONObject > arrList = new ArrayList< BSONObject >();
        for ( int i = 0; i < insertNum; i++ ) {
            BSONObject obj = ( BSONObject ) JSON.parse( "{a:" + i + "}" );
            arrList.add( obj );
        }
        cl.insert( arrList );
    }

}
