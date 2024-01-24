package com.sequoiadb.split;

import java.util.ArrayList;

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
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

/**
 * @FileName:SEQDB-514 数据切分过程中执行findandremove组合skip/limit:1.在cl下指定分区键进行数据切分
 *                     2、切分过程中执行findandremove组合skip/limit 3、查看数据切分结果
 *                     4、再次插入数据，查看写数据情况
 * @author huangqiaohui
 * @version 1.00
 *
 */

public class Split514 extends SdbTestBase {
    private String clName = "testcaseCL514";
    private String srcGroupName;
    private String destGroupName;
    private Sequoiadb commSdb = null;
    private int removeNum = 0;

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
                    "{ShardingKey:{\"a\":1},ReplSize:0,ShardingType:\"range\"}" ) );
            ArrayList< String > tmp = SplitUtils.getGroupName( commSdb, csName,
                    clName );
            srcGroupName = tmp.get( 0 );
            destGroupName = tmp.get( 1 );
            prepareData( commSdb ); // 准备切分的数据
        } catch ( BaseException e ) {
            if ( commSdb != null ) {
                commSdb.disconnect();
            }
            Assert.fail( "TestCase514 setUp error, error description:"
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

    // 切分同时查询删除，校验结果
    @SuppressWarnings("deprecation")
    @Test(enabled = true)
    public void testFindAndRemove() {
        Sequoiadb sdb = null;
        Split split = new Split();
        FindAndRemove findAndRemove = new FindAndRemove();
        try {
            split.start();
            sdb = new Sequoiadb( coordUrl, "", "" );
            findAndRemove.start();
            if ( !split.isSuccess() ) {
                Assert.fail( split.getErrorMsg() );
            }
            if ( !findAndRemove.isSuccess() ) {
                Assert.fail( findAndRemove.getErrorMsg() );
            }
            // 检查编目
            checkCatalog( sdb );
            // 校验目标组数据，重新插入，再次校验
            checkData( sdb );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() + "\r\n"
                    + SplitUtils.getKeyStack( e, this ) );
        } finally {
            if ( sdb != null ) {
                sdb.disconnect();
            }
        }

    }

    // 检查编目信息的切分范围是否正确
    private void checkCatalog( Sequoiadb sdb ) {
        DBCursor dbc = null;
        try {
            dbc = sdb.getSnapshot( Sequoiadb.SDB_SNAP_CATALOG,
                    "{Name:\"" + csName + "." + clName + "\"}", null, null );
            BasicBSONList list = null;
            if ( dbc.hasNext() ) {
                list = ( BasicBSONList ) dbc.getNext().get( "CataInfo" );
            } else {
                Assert.fail( clName + " collection catalog not found" );
            }
            BSONObject expectLowBound = ( BSONObject ) JSON
                    .parse( "{\"a\":0}" );
            BSONObject expectUpBound = ( BSONObject ) JSON
                    .parse( "{\"a\":100}" );
            for ( int i = 0; i < list.size(); i++ ) {
                String groupName = ( String ) ( ( BSONObject ) list.get( i ) )
                        .get( "GroupName" );
                if ( groupName.equals( destGroupName ) ) {
                    BSONObject actualLowBound = ( BSONObject ) ( ( BSONObject ) list
                            .get( i ) ).get( "LowBound" );
                    BSONObject actualUpBound = ( BSONObject ) ( ( BSONObject ) list
                            .get( i ) ).get( "UpBound" );
                    if ( actualLowBound.equals( expectLowBound )
                            && actualUpBound.equals( expectUpBound ) ) {
                        break;
                    } else {
                        Assert.fail( "check catalog fail" );
                    }
                }
            }

        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() + "\r\n"
                    + SplitUtils.getKeyStack( e, this ) );
        } finally {
            if ( dbc != null ) {
                dbc.close();
            }
        }

    }

    @SuppressWarnings("deprecation")
    private void checkData( Sequoiadb sdb ) {
        Sequoiadb destDataNode = null;
        Sequoiadb srcdataNode = null;
        try {
            // 获取目标组主节点链接
            destDataNode = sdb.getReplicaGroup( destGroupName ).getMaster()
                    .connect();
            // 获取源组主节点链接
            srcdataNode = sdb.getReplicaGroup( srcGroupName ).getMaster()
                    .connect();

            long destDataCount = destDataNode.getCollectionSpace( csName )
                    .getCollection( clName ).getCount( "{a:{$gte:0,$lt:100}}" );
            Assert.assertEquals( destDataCount, 100 - removeNum );// 目标组应当含有上述范围数据
            Assert.assertEquals( destDataNode.getCollectionSpace( csName )
                    .getCollection( clName ).getCount(), 60 );// 目标组应当仅含有上述范围数据

            insertAndCheck( sdb, destDataNode, srcdataNode ); // 重新插入数据，检查落入情况
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() + "\r\n"
                    + SplitUtils.getKeyStack( e, this ) );
        } finally {
            if ( srcdataNode != null ) {
                srcdataNode.disconnect();
            }
            if ( destDataNode != null ) {
                destDataNode.disconnect();
            }
        }
    }

    public void insertAndCheck( Sequoiadb sdb, Sequoiadb destDataNode,
            Sequoiadb srcdataNode ) {
        try {
            // 插入数据
            DBCollection cl = sdb.getCollectionSpace( csName )
                    .getCollection( clName );
            cl.insert( ( BSONObject ) JSON.parse( "{a:10,b:-1}" ) );// 期望此数据落入目标组
            cl.insert( ( BSONObject ) JSON.parse( "{a:500,b:-2}" ) );// 期望此数据落入源数据组

            // 目标组落入情况
            DBCollection destGroupCL = destDataNode.getCollectionSpace( csName )
                    .getCollection( clName );
            if ( !SplitUtils.isCollectionContainThisJSON( destGroupCL,
                    "{a:10,b:-1}" ) ) {
                Assert.fail( "check query data not pass" );
            }

            // 源组落入情况
            DBCollection srcGroupCL = srcdataNode.getCollectionSpace( csName )
                    .getCollection( clName );
            if ( !SplitUtils.isCollectionContainThisJSON( srcGroupCL,
                    "{a:500,b:-2}" ) ) {
                Assert.fail( "check query data not pass" );
            }
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() + "\r\n"
                    + SplitUtils.getKeyStack( e, this ) );
        }

    }

    @SuppressWarnings("deprecation")
    @AfterClass(enabled = true)
    public void tearDown() {
        try {
            CollectionSpace commCS = commSdb.getCollectionSpace( csName );
            commCS.dropCollection( clName );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() + "\r\n"
                    + SplitUtils.getKeyStack( e, this ) );
        } finally {
            if ( commSdb != null ) {
                commSdb.disconnect();
            }
        }
    }

    class FindAndRemove extends SdbThreadBase {

        @SuppressWarnings({ "resource", "deprecation" })
        @Override
        public void exec() throws Exception {
            Sequoiadb sdb = null;
            DBCursor dbc = null;
            try {
                sdb = new Sequoiadb( coordUrl, "", "" );
                sdb.setSessionAttr(
                        ( BSONObject ) JSON.parse( "{PreferedInstance:'M'}" ) );
                DBCollection cl = sdb.getCollectionSpace( csName )
                        .getCollection( clName );
                // 删除a:0 - a:50 的记录
                dbc = cl.queryAndRemove(
                        ( BSONObject ) JSON.parse( "{a:{$gte:0,$lt:50}}" ),
                        null, null, null, 10, 50, 0 );
                while ( dbc.hasNext() ) {
                    dbc.getNext();
                    removeNum++;
                    Thread.sleep( 50 );
                }
            } catch ( BaseException e ) {
                throw e;
            } finally {
                if ( dbc != null ) {
                    dbc.close();
                }
                if ( sdb != null ) {
                    sdb.disconnect();
                }
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
                cl.split( srcGroupName, destGroupName,
                        ( BSONObject ) JSON.parse( "{a:0}" ),
                        ( BSONObject ) JSON.parse( "{a:100}" ) );
            } catch ( BaseException e ) {
                throw e;
            } finally {
                if ( sdb != null ) {
                    sdb.disconnect();
                }
            }
        }
    }

}
