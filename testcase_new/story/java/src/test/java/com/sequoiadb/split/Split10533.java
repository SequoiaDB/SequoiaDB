package com.sequoiadb.split;

import java.util.ArrayList;
import java.util.List;
import java.util.Random;

import org.bson.BSONObject;
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
import com.sequoiadb.fulltext.utils.FullTextDBUtils;
import com.sequoiadb.fulltext.utils.FullTextUtils;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.threadexecutor.ResultStore;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;

/**
 * @FileName:SEQDB-10533 切分过程中执行truncate :1、向cl中插入数据记录 2、执行split，设置切分条件
 *                       3、切分过程中执行truncate操作，分别验证如下几个场景： a、迁移数据过程中执行truncate
 *                       b、清除数据过程中执行truncate 4、查看切分和truncate操作结果
 * @author huangqiaohui
 * @version 1.00
 *
 */

public class Split10533 extends SdbTestBase {
    private DBCollection cl = null;
    private String clName = "testcaseCL10533";
    private String srcGroupName;
    private String destGroupName;
    private Sequoiadb commSdb = null;
    private List< BSONObject > insertedData = new ArrayList< BSONObject >();
    private int recsNum = 1000;

    @BeforeClass()
    public void setUp() {

        try {
            commSdb = new Sequoiadb( coordUrl, "", "" );

            // 跳过 standAlone 和数据组不足的环境
            CommLib commlib = new CommLib();
            if ( commlib.isStandAlone( commSdb ) ) {
                throw new SkipException( "skip StandAlone" );
            }
            List< String > groupsName = commlib.getDataGroupNames( commSdb );
            if ( groupsName.size() < 2 ) {
                throw new SkipException(
                        "current environment less than tow groups " );
            }
            srcGroupName = groupsName.get( 0 );
            destGroupName = groupsName.get( 1 );

            CollectionSpace customCS = commSdb.getCollectionSpace( csName );
            cl = customCS.createCollection( clName, ( BSONObject ) JSON.parse(
                    "{ShardingKey:{'sk':1},Partition:4096,ShardingType:'hash',Group:'"
                            + srcGroupName + "'}" ) );
            insertData( cl );// 写入待切分的记录（500）
        } catch ( BaseException e ) {
            if ( commSdb != null ) {
                commSdb.disconnect();
            }
            Assert.fail( this.getClass().getName()
                    + " setUp error, error description:" + e.getMessage()
                    + "\r\n" + SplitUtils.getKeyStack( e, this ) );
        }
    }

    public void insertData( DBCollection cl ) {
        for ( int i = 0; i < 1000; i++ ) {
            BSONObject obj = ( BSONObject ) JSON.parse( "{sk:" + i + "}" );
            cl.insert( obj );
            insertedData.add( obj );
        }
    }

    @Test
    public void testTruncateCL() throws Exception {
        ThreadExecutor es = new ThreadExecutor( FullTextUtils.THREAD_TIMEOUT );
        ThreadTruncate threadTruncate = new ThreadTruncate();
        ThreadSplit threadSplit = new ThreadSplit();
        es.addWorker( threadTruncate );
        es.addWorker( threadSplit );
        es.run();

        // 检验结果
        if ( threadTruncate.getRetCode() == 0 ) {
            Assert.assertEquals( cl.getCount(), 0 );
        } else if ( threadTruncate.getRetCode() != -190
                && threadTruncate.getRetCode() != -147 ) {
            Assert.fail( "truncate fail, e: " + threadTruncate.getRetCode() );
        } else {
            Assert.assertEquals( cl.getCount(), recsNum );
        }

        int actRgNum = FullTextDBUtils.getCLGroups( cl ).size();
        if ( threadSplit.getRetCode() == 0 ) {
            Assert.assertEquals( actRgNum, 2 );
            checkCatalog( commSdb );
        } else if ( threadSplit.getRetCode() != -321
                && threadSplit.getRetCode() != -243
                && threadSplit.getRetCode() != -147
                && threadSplit.getRetCode() != -190 ) {
            Assert.fail( "split fail, e: " + threadSplit.getRetCode() );
        } else {
            Assert.assertEquals( actRgNum, 1 );
        }
    }

    @AfterClass()
    public void tearDown() {
        try {
            CollectionSpace cs = commSdb.getCollectionSpace( csName );
            cs.dropCollection( clName );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() + "\r\n"
                    + SplitUtils.getKeyStack( e, this ) );
        } finally {
            if ( commSdb != null ) {
                commSdb.disconnect();
            }
        }
    }

    private void checkCatalog( Sequoiadb sdb ) {
        DBCursor dbc = null;
        try {
            dbc = sdb.getSnapshot( Sequoiadb.SDB_SNAP_CATALOG,
                    "{Name:\"" + csName + "." + clName + "\"}", null, null );
            BasicBSONList list = null;
            if ( dbc.hasNext() ) {
                list = ( BasicBSONList ) dbc.getNext().get( "CataInfo" );
            }

            BSONObject destExpectLowBound = ( BSONObject ) JSON
                    .parse( "{\"\":2048}" );
            BSONObject destExpectUpBound = ( BSONObject ) JSON
                    .parse( "{\"\":4096}" );

            BSONObject srcExpectLowBound = ( BSONObject ) JSON
                    .parse( "{\"\":0}" );
            BSONObject srcExpectUpBound = ( BSONObject ) JSON
                    .parse( "{\"\":2048}" );

            boolean srcCheckFlag = false;
            boolean destCheckFlag = false;
            for ( int i = 0; i < list.size(); i++ ) {
                String groupName = ( String ) ( ( BSONObject ) list.get( i ) )
                        .get( "GroupName" );
                // 目标组编目信息检查
                if ( groupName.equals( destGroupName ) ) {
                    BSONObject actualLowBound = ( BSONObject ) ( ( BSONObject ) list
                            .get( i ) ).get( "LowBound" );
                    BSONObject actualUpBound = ( BSONObject ) ( ( BSONObject ) list
                            .get( i ) ).get( "UpBound" );
                    if ( actualLowBound.equals( destExpectLowBound )
                            && actualUpBound.equals( destExpectUpBound ) ) {
                        destCheckFlag = true;
                    } else {
                        Assert.fail( "check catalog fail" );
                    }
                }
                // 源组编目信息检查
                if ( groupName.equals( srcGroupName ) ) {
                    BSONObject actualLowBound = ( BSONObject ) ( ( BSONObject ) list
                            .get( i ) ).get( "LowBound" );
                    BSONObject actualUpBound = ( BSONObject ) ( ( BSONObject ) list
                            .get( i ) ).get( "UpBound" );
                    if ( actualLowBound.equals( srcExpectLowBound )
                            && actualUpBound.equals( srcExpectUpBound ) ) {
                        srcCheckFlag = true;
                    } else {
                        Assert.fail( "check catalog fail" );
                    }
                }
            }
            // 检查源和目标组编目信息的校验是否均通过
            Assert.assertTrue( srcCheckFlag && destCheckFlag, "srcCheckFalg:"
                    + srcCheckFlag + " destCheckFlag:" + destCheckFlag );
        } finally {
            if ( dbc != null ) {
                dbc.close();
            }
        }
    }

    class ThreadTruncate extends ResultStore {
        @ExecuteOrder(step = 1)
        public void truncateCL() throws InterruptedException {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( coordUrl, "", "" );
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                Random random = new Random();
                Thread.sleep( random.nextInt( 5 ) * 1000 );
                cl.truncate();
            } catch ( BaseException e ) {
                saveResult( e.getErrorCode(), e );
            } finally {
                if ( db != null ) {
                    db.disconnect();
                }
            }
        }
    }

    class ThreadSplit extends ResultStore {
        @ExecuteOrder(step = 1)
        public void split() {
            Sequoiadb sdb = null;
            try {
                sdb = new Sequoiadb( coordUrl, "", "" );
                CollectionSpace cs = sdb.getCollectionSpace( csName );
                DBCollection cl = cs.getCollection( clName );
                cl.split( srcGroupName, destGroupName, 50 );
            } catch ( BaseException e ) {
                saveResult( e.getErrorCode(), e );
            } finally {
                if ( sdb != null ) {
                    sdb.disconnect();
                }
            }
        }

    }
}
