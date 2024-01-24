package com.sequoiadb.split;

import java.util.ArrayList;

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

/**
 * @FileName:SEQDB-8583 范围切分时，指定分区范围不正确:1、在cl下指定范围条件进行数据切分 2、分别验证如下两种场景：
 *                      a、执行split操作，任务已下发开始执行切分（可通过listTasks查看任务已下发，
 *                      直连目标节点查看数据已开始迁移），同时删除cl b、执行split操作，当切分任务还没开始执行时，删除cl
 *                      3、查看数据切分是否成功，删除cl是否成功
 * @author huangqiaohui
 * @version 1.00
 *
 */

public class Split8583a extends SdbTestBase {
    private String clName = "testcaseCL8583a";
    private String srcGroupName;
    private String destGroupName;
    private Sequoiadb commSdb = null;

    @SuppressWarnings("deprecation")
    @BeforeClass(enabled = true)
    public void setUp() {

        try {
            commSdb = new Sequoiadb( coordUrl, "", "" );

            // 跳过 standAlone 和数据组不足的环境
            if ( CommLib.isStandAlone( commSdb ) ) {
                throw new SkipException( "skip StandAlone" );
            }
            if ( CommLib.getDataGroupNames( commSdb ).size() < 2 ) {
                throw new SkipException(
                        "current environment less than tow groups " );
            }

            CollectionSpace commCS = commSdb.getCollectionSpace( csName );
            commCS.createCollection( clName, ( BSONObject ) JSON.parse(
                    "{ShardingKey:{\"a\":1},ShardingType:\"range\"}" ) );
            ArrayList< String > tmp = SplitUtils.getGroupName( commSdb, csName,
                    clName );// 获取目标组名和源组名
            srcGroupName = tmp.get( 0 );
            destGroupName = tmp.get( 1 );
            prepareData( commSdb ); // 准备数据
        } catch ( BaseException e ) {
            if ( commSdb != null ) {
                commSdb.disconnect();
            }
            Assert.fail( "TestCase8583a setUp error, error description:"
                    + e.getMessage() + "\r\n"
                    + SplitUtils.getKeyStack( e, this ) );
        }
    }

    @SuppressWarnings("deprecation")
    public void prepareData( Sequoiadb db ) {
        try {
            DBCollection cl = db.getCollectionSpace( csName )
                    .getCollection( clName );
            ArrayList< BSONObject > arr = new ArrayList<>();
            for ( int i = 0; i < 1000; i++ ) {
                arr.add( ( BSONObject ) JSON.parse( "{a:" + i + "}" ) );
            }
            cl.bulkInsert( arr, SplitUtils.FLG_INSERT_CONTONDUP );
        } catch ( BaseException e ) {
            throw e;
        }

    }

    // 切分删除CL
    @SuppressWarnings({ "resource", "deprecation" })
    @Test
    public void dropCL() {
        Sequoiadb sdb = null;
        DBCursor dbc = null;
        Split split = new Split();
        split.start();
        try {
            sdb = new Sequoiadb( coordUrl, "", "" );
            // 等待切分任务建立
            while ( true ) {
                dbc = sdb.listTasks(
                        ( BSONObject ) JSON.parse(
                                "{Name:\"" + csName + "." + clName + "\"}" ),
                        null, null, null );
                if ( dbc.hasNext() ) {
                    break;
                }
            }
            // 删除CL
            sdb.getCollectionSpace( csName ).dropCollection( clName );

            // 检查CL是否被删除
            if ( sdb.getCollectionSpace( csName )
                    .isCollectionExist( clName ) ) {
                Assert.fail( "CL delete fail" );
            }

            // 检查切分任务是否存在
            dbc = sdb.listTasks(
                    ( BSONObject ) JSON.parse(
                            "{Name:\"" + csName + "." + clName + "\"}" ),
                    null, null, null );
            if ( dbc.hasNext() ) {
                Assert.fail( "split Task dose not cancel" );
            }

        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() + "\r\n"
                    + SplitUtils.getKeyStack( e, this ) );
        } finally {
            if ( dbc != null ) {
                dbc.close();
            }
            if ( sdb != null ) {
                sdb.disconnect();
            }
            split.join();
        }
    }

    @SuppressWarnings("deprecation")
    @AfterClass(enabled = true)
    public void tearDown() {
        try {
            CollectionSpace commCS = commSdb.getCollectionSpace( csName );
            if ( commCS.isCollectionExist( clName ) ) {
                commCS.dropCollection( clName );
                Assert.fail( "cl should not exist" );
            }
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() + "\r\n"
                    + SplitUtils.getKeyStack( e, this ) );
        } finally {
            if ( commSdb != null ) {
                commSdb.disconnect();
            }
        }
    }

    class Split extends SdbThreadBase {

        @SuppressWarnings({ "resource", "deprecation" })
        @Override
        public void exec() throws Exception {
            Sequoiadb sdb = null;
            try {
                sdb = new Sequoiadb( coordUrl, "", "" );
                DBCollection cl = sdb.getCollectionSpace( csName )
                        .getCollection( clName );
                if ( cl == null ) {
                    return;
                }
                cl.splitAsync( srcGroupName, destGroupName,
                        ( BSONObject ) JSON.parse( "{a:0}" ),
                        ( BSONObject ) JSON.parse( "{a:100}" ) );
            } catch ( BaseException e ) {
                throw e;
            } finally {
                if ( sdb != null )
                    sdb.disconnect();
            }
        }
    }

}
