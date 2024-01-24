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
 * @FileName:SEQDB-511 数据切分过程中插入大量普通记录数据1.在cl下指定分区键进行数据切分
 *                     2、切分过程中向cl中插入大量数据，如插入1百万条记录 3、查看数据切分结果 4、再次插入数据，查看写数据情况
 * @author huangqiaohui
 * @version 1.00
 *
 */

public class Split511 extends SdbTestBase {
    private String clName = "testcaseCL511";
    private String srcGroupName;
    private String destGroupName;
    Sequoiadb commSdb = null;

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
                    clName );
            srcGroupName = tmp.get( 0 );
            destGroupName = tmp.get( 1 );
            prepareData( commSdb );// 写入待切分的记录（1000）
        } catch ( BaseException e ) {
            if ( commSdb != null ) {
                commSdb.disconnect();
            }
            Assert.fail( this.getClass().getName()
                    + " setUp error, error description:" + e.getMessage()
                    + "\r\n" + SplitUtils.getKeyStack( e, this )
                    + e.getStackTrace().toString() );
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

    // 切分过程中写入记录(1000)，检查目标组数据，插入数据，检查落入
    @SuppressWarnings("deprecation")
    @Test
    public void insertData() {
        Sequoiadb db = null;
        Split split = new Split();

        InsertDataToCL insertDataToCL = new InsertDataToCL();
        split.start();
        insertDataToCL.start();

        try {

            db = new Sequoiadb( coordUrl, "", "" );
            if ( !split.isSuccess() ) {
                Assert.fail( split.getErrorMsg() );
            }
            if ( !insertDataToCL.isSuccess() ) {
                Assert.fail( split.getErrorMsg() );
            }
            checkCatalog( db );// 检查切分后编目信息
            checkData( db ); // 检查源和目标组数据量，插入数据，检测落入情况
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() + "\r\n"
                    + SplitUtils.getKeyStack( e, this ) );
        } finally {
            if ( db != null )
                db.disconnect();
        }
    }

    // 检查目标组编目
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
            BSONObject expectLowBound = ( BSONObject ) JSON.parse( "{a:0}" );
            BSONObject expectUpBound = ( BSONObject ) JSON.parse( "{a:500}" );
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

    // 检查目标组数据，插入数据，检查落入
    @SuppressWarnings("deprecation")
    private void checkData( Sequoiadb sdb ) {
        Sequoiadb destDataNode = null;
        Sequoiadb srcDataNode = null;
        try {
            // 目标组数据量检查
            destDataNode = sdb.getReplicaGroup( destGroupName ).getMaster()
                    .connect();
            DBCollection destCL = destDataNode.getCollectionSpace( csName )
                    .getCollection( clName );
            long count = destCL.getCount( "{a:{$gte:0,$lt:500}}" );
            Assert.assertEquals( count, 1000 );// 目标组数据应当含有上述查询数据
            Assert.assertEquals( destCL.getCount(), 1000 ); //// 目标组数据应当仅含有上述查询数据

            // 插入数据
            DBCollection cl = sdb.getCollectionSpace( csName )
                    .getCollection( clName );
            cl.insert( ( BSONObject ) JSON.parse( "{a:-2000,b:1}" ) );// 期望此数据落入源数据组
            cl.insert( ( BSONObject ) JSON.parse( "{a:200,b:1}" ) );// 期望此数据落入目标组

            // 目标组落入情况
            DBCollection destGroupCL = destDataNode.getCollectionSpace( csName )
                    .getCollection( clName );
            if ( !SplitUtils.isCollectionContainThisJSON( destGroupCL,
                    "{a:200,b:1}" ) ) {
                Assert.fail( "check query data not pass" );
            }

            // 源组落入情况
            srcDataNode = sdb.getReplicaGroup( srcGroupName ).getMaster()
                    .connect();
            DBCollection srcGroupCL = srcDataNode.getCollectionSpace( csName )
                    .getCollection( clName );
            if ( !SplitUtils.isCollectionContainThisJSON( srcGroupCL,
                    "{a:-2000,b:1}" ) ) {
                Assert.fail( "check query data not pass" );
            }
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() + "\r\n"
                    + SplitUtils.getKeyStack( e, this ) );
        } finally {
            if ( srcDataNode != null ) {
                srcDataNode.disconnect();
            }
            if ( destDataNode != null ) {
                destDataNode.disconnect();
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

    class InsertDataToCL extends SdbThreadBase {

        @SuppressWarnings({ "resource", "deprecation" })
        @Override
        public void exec() throws Exception {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( coordUrl, "", "" );
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                // ArrayList< BSONObject > arr = new ArrayList<>();
                for ( int i = 0; i < 1000; i++ ) {
                    // arr.add((BSONObject) JSON.parse("{a:" + i + "}"));
                    cl.insert( ( BSONObject ) JSON.parse( "{a:" + i + "}" ) );
                }
                // cl.bulkInsert(arr, SplitUtils.FLG_INSERT_CONTONDUP);
            } finally {
                if ( db != null )
                    db.disconnect();
            }
        }

    }

    class Split extends SdbThreadBase {

        @SuppressWarnings({ "resource", "deprecation" })
        @Override
        public void exec() throws Exception {

            // 切分
            Sequoiadb sdb = null;
            try {
                sdb = new Sequoiadb( coordUrl, "", "" );
                DBCollection cl = sdb.getCollectionSpace( csName )
                        .getCollection( clName );
                cl.split( srcGroupName, destGroupName,
                        ( BSONObject ) JSON.parse( "{a:0}" ),
                        ( BSONObject ) JSON.parse( "{a:500}" ) );
            } catch ( BaseException e ) {
                throw e;
            } finally {
                if ( sdb != null )
                    sdb.disconnect();
            }

        }

    }

}
