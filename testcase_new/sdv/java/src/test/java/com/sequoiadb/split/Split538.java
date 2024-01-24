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

/**
 * @FileName:SEQDB-538 范围切分时，指定分区范围不正确:1、在cl下指定范围条件进行数据切分
 *                     2、执行split操作，其中设置的范围区间不在CL分区范围内 3、查看数据切分是否成功
 *                     4、插入该范围区间的数据，查看数据写入情况
 * @author huangqiaohui
 * @version 1.00
 *
 */

public class Split538 extends SdbTestBase {
    private String clName = "testcaseCL538";
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
                    clName );// 获取目标组名，和源组名
            srcGroupName = tmp.get( 0 );
            destGroupName = tmp.get( 1 );
            prepareData( commSdb ); // 写入待切分的记录（1000）
        } catch ( BaseException e ) {
            if ( commSdb != null )
                commSdb.disconnect();
            Assert.fail( this.getClass().getName()
                    + " setUp error, error description:" + e.getMessage()
                    + "\r\n" + SplitUtils.getKeyStack( e, this ) );
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

    // 切分(2000,3000)，检查编目
    @SuppressWarnings("deprecation")
    @Test(enabled = true)
    public void splitCL() {
        Sequoiadb sdb = null;
        try {
            sdb = new Sequoiadb( coordUrl, "", "" );
            DBCollection cl = sdb.getCollectionSpace( csName )
                    .getCollection( clName );
            cl.split( srcGroupName, destGroupName,
                    ( BSONObject ) JSON.parse( "{a:2000}" ),
                    ( BSONObject ) JSON.parse( "{a:3000}" ) );
            checkCatalog( sdb ); // 检查编目信息
            insertAndCheck( sdb ); // 插入数据，检查落入
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() + "\r\n"
                    + SplitUtils.getKeyStack( e, this ) );
        } finally {
            if ( sdb != null )
                sdb.disconnect();
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
                    .parse( "{\"a\":2000}" );
            BSONObject expectUpBound = ( BSONObject ) JSON
                    .parse( "{\"a\":3000}" );
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
    public void insertAndCheck( Sequoiadb sdb ) {
        DBCursor dbc2 = null;
        DBCursor dbc3 = null;
        Sequoiadb destDataNode = null;
        Sequoiadb srcdataNode = null;
        try {
            // 获取目标组主节点链接
            destDataNode = sdb.getReplicaGroup( destGroupName ).getMaster()
                    .connect();
            // 获取源组主节点链接
            srcdataNode = sdb.getReplicaGroup( srcGroupName ).getMaster()
                    .connect();

            // 插入数据
            DBCollection cl = sdb.getCollectionSpace( csName )
                    .getCollection( clName );
            cl.insert( ( BSONObject ) JSON.parse( "{a:2500}" ) ); // 期望落入目标组
            cl.insert( ( BSONObject ) JSON.parse( "{a:-500}" ) );// 期望落入源组

            // 目标组落入情况
            DBCollection destGroupCL = destDataNode.getCollectionSpace( csName )
                    .getCollection( clName );
            if ( !SplitUtils.isCollectionContainThisJSON( destGroupCL,
                    "{a:2500}" ) ) {
                Assert.fail( "check query data not pass" );
            }

            // 源组落入情况
            DBCollection srcGroupCL = srcdataNode.getCollectionSpace( csName )
                    .getCollection( clName );
            if ( !SplitUtils.isCollectionContainThisJSON( srcGroupCL,
                    "{a:-500}" ) ) {
                Assert.fail( "check query data not pass" );
            }
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() + "\r\n"
                    + SplitUtils.getKeyStack( e, this ) );
        } finally {
            if ( dbc2 != null ) {
                dbc2.close();
            }
            if ( dbc3 != null ) {
                dbc3.close();
            }
            if ( destDataNode != null ) {
                destDataNode.disconnect();
            }
            if ( srcdataNode != null ) {
                srcdataNode.disconnect();
            }
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

}
