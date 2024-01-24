package com.sequoiadb.split;

import java.util.ArrayList;
import java.util.Date;

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
 * @FileName:SEQDB-10504 hash分区表指定百分比切分数据 1、向cl中插入数据，分别覆盖如下情况： a、普通记录 b、lob对象
 *                       c、普通记录+lob 2、执行split，设置百分比切分条件
 *                       3、查看数据切分结果（分别连接coord、源组data、目标组data查询，记录执行find/count查询，
 *                       lob可执行listlobs查询）， 4、再次插入数据（数据分别在源和目标组范围内）
 *                       备注：此Class验证A情况
 * @author huangqiaohui
 * @version 1.00
 *
 */

public class Split10504A extends SdbTestBase {
    private String clName = "testcaseCL10504A";
    private String srcGroupName;
    private String destGroupName;
    private Sequoiadb commSdb = null;
    private DBCollection commCL;
    private ArrayList< BSONObject > insertedData = new ArrayList< BSONObject >();// 记录所有已插入的普通数据

    @BeforeClass
    public void setUp() {
        try {
            commSdb = new Sequoiadb( coordUrl, "", "" );
            commSdb.setSessionAttr(
                    ( BSONObject ) JSON.parse( "{PreferedInstance:'M'}" ) );
            // 跳过 standAlone 和数据组不足的环境
            CommLib commlib = new CommLib();
            if ( commlib.isStandAlone( commSdb ) ) {
                throw new SkipException( "skip StandAlone" );
            }
            ArrayList< String > groupsName = commlib
                    .getDataGroupNames( commSdb );
            if ( groupsName.size() < 2 ) {
                throw new SkipException(
                        "current environment less than tow groups " );
            }

            CollectionSpace commCS = commSdb.getCollectionSpace( csName );
            srcGroupName = groupsName.get( 0 );
            destGroupName = groupsName.get( 1 );
            commCL = commCS.createCollection( clName, ( BSONObject ) JSON.parse(
                    "{ShardingKey:{sk:1},Partition:4096,ShardingType:'hash',Group:'"
                            + srcGroupName + "'}" ) );
        } catch ( BaseException e ) {
            if ( commSdb != null ) {
                commSdb.disconnect();
            }
            Assert.fail( this.getClass().getName()
                    + " setUp error, error description:" + e.getMessage()
                    + "\r\n" + SplitUtils.getKeyStack( e, this ) );
        }
    }

    @Test
    public void split() {
        try {
            insertData();// 写入待切分的记录[sk:0,sk:200)
            commCL.split( srcGroupName, destGroupName, 50 );

            // 校验源和目标组普通记录
            ArrayList< BSONObject > insertedDataCopy = new ArrayList< BSONObject >(
                    insertedData );
            checkGroupData( insertedDataCopy, destGroupName );// 目标组中的数据应当是insertDataCopy的子集，校验完成后，删除insertDataCopy中属于子集的元素
            checkGroupData( insertedDataCopy, srcGroupName );// 校验源组
            Assert.assertEquals( insertedDataCopy.size() == 0, true, // 经过两次校验，insertDataCopy应当为空
                    "split error,destGroup and srcGroup can not find:"
                            + insertedDataCopy );
            // 检查编目信息
            checkCatalog();
            // 插入数据，检查落入
            insertAndCheckAgain();
            // 通过协调节点比对已插入所有数据
            checkCoord();
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() + "\r\n"
                    + SplitUtils.getKeyStack( e, this ) );
        }
    }

    private void checkCatalog() {
        DBCursor dbc = null;
        try {
            dbc = commSdb.getSnapshot( Sequoiadb.SDB_SNAP_CATALOG,
                    "{Name:\"" + csName + "." + clName + "\"}", null, null );
            BasicBSONList list = null;
            if ( dbc.hasNext() ) {
                list = ( BasicBSONList ) dbc.getNext().get( "CataInfo" );
            } else {
                Assert.fail( clName + " collection catalog not found" );
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
                if ( groupName.equals( destGroupName ) ) {// 目标组编目信息检查
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
                if ( groupName.equals( srcGroupName ) ) {// 源组编目信息检查
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
            Assert.assertEquals( srcCheckFlag && destCheckFlag, true,
                    "srcCheckFalg:" + srcCheckFlag + " destCheckFlag:"
                            + destCheckFlag );

        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() + "\r\n"
                    + SplitUtils.getKeyStack( e, this ) );
        } finally {
            if ( dbc != null ) {
                dbc.close();
            }
        }

    }

    public void insertAndCheckAgain() {

        Sequoiadb destDataNode = null;
        Sequoiadb srcDataNode = null;
        DBCursor cursor1 = null;
        DBCursor cursor2 = null;
        try {
            destDataNode = commSdb.getReplicaGroup( destGroupName ).getMaster()
                    .connect();// 获得目标组主节点链接
            srcDataNode = commSdb.getReplicaGroup( srcGroupName ).getMaster()
                    .connect();// 获得目标组主节点链接
            DBCollection destCL = destDataNode.getCollectionSpace( csName )
                    .getCollection( clName );
            DBCollection srcCL = srcDataNode.getCollectionSpace( csName )
                    .getCollection( clName );

            // 得到一个目标组的记录，添加flag字段后再次插入，期望此记录落入目标组
            BSONObject destRecord = null;
            cursor1 = destCL.query( null, "{sk:''}", "{sk:1}", null, 0, 1 );
            if ( cursor1.hasNext() ) {
                destRecord = cursor1.getNext();
            } else {
                Assert.fail( "query fail" );
            }
            destRecord.put( "falg", -1 );
            commCL.insert( destRecord );
            insertedData.add( destRecord );

            // 得到一个源组的记录，添加flag字段后再次插入，期望此记录落入源组
            BSONObject srcRecord = null;
            cursor2 = srcCL.query( null, "{sk:''}", "{sk:1}", null, 0, 1 );
            if ( cursor2.hasNext() ) {
                srcRecord = cursor2.getNext();
            } else {
                Assert.fail( "query fail" );
            }
            srcRecord.put( "falg", -2 );
            commCL.insert( srcRecord );
            insertedData.add( srcRecord );

            // 比对结果
            Assert.assertEquals( destCL.getCount( destRecord ), 1,
                    "query fail" );
            Assert.assertEquals( destCL.getCount( srcRecord ), 0,
                    "query fail" );
            Assert.assertEquals( srcCL.getCount( destRecord ), 0,
                    "query fail" );
            Assert.assertEquals( srcCL.getCount( srcRecord ), 1, "query fail" );

        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() + "\r\n"
                    + SplitUtils.getKeyStack( e, this ) );
        } finally {
            if ( cursor1 != null ) {
                cursor1.close();
            }
            if ( cursor2 != null ) {
                cursor2.close();
            }
            if ( destDataNode != null ) {
                destDataNode.disconnect();
            }
            if ( srcDataNode != null ) {
                srcDataNode.disconnect();
            }
        }
    }

    private void checkCoord() {
        DBCursor cursor1 = null;
        DBCursor cursor2 = null;
        try {
            // 比对所有数据
            cursor1 = commCL.query( null, null, "{sk:1}", null );
            while ( cursor1.hasNext() ) {
                BSONObject actual = cursor1.getNext();
                Assert.assertEquals( insertedData.contains( actual ), true,
                        "insertedData can not find this record:" + actual );
                insertedData.remove( actual );
            }
            if ( insertedData.size() > 0 ) {
                Assert.fail( "missing data:" + insertedData.toString() );
            }
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() + "\r\n"
                    + SplitUtils.getKeyStack( e, this ) );
        } finally {
            if ( cursor1 != null ) {
                cursor1.close();
            }
            if ( cursor2 != null ) {
                cursor2.close();
            }
        }
    }

    private void checkGroupData( ArrayList< BSONObject > insertedData,
            String groupName ) {
        Sequoiadb dataNode = null;
        DBCursor cursor = null;
        try {
            dataNode = commSdb.getReplicaGroup( groupName ).getMaster()
                    .connect();// 获得目标组主节点链接
            DBCollection cl = dataNode.getCollectionSpace( csName )
                    .getCollection( clName );

            cursor = cl.query( null, null, "{sk:1}", null );
            while ( cursor.hasNext() ) {
                BSONObject actual = cursor.getNext();
                Assert.assertEquals( insertedData.contains( actual ), true,
                        "insertedData can not find this record:" + actual );
                insertedData.remove( actual );
            }
            long count = cl.getCount();
            // 组的数据量应该在100条左右（总量200，切分范围50%）
            Assert.assertEquals(
                    count > 100 - ( 100 * 0.3 ) && count < 100 + ( 100 * 0.3 ),
                    true, "destGroup data count:" + count );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() + "\r\n"
                    + SplitUtils.getKeyStack( e, this ) );
        } finally {
            if ( cursor != null ) {
                cursor.close();
            }
            if ( dataNode != null ) {
                dataNode.disconnect();
            }
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            CollectionSpace commCS = commSdb.getCollectionSpace( csName );
            commCS.dropCollection( clName );
        } catch ( BaseException e ) {
            e.printStackTrace();
            Assert.fail( e.getMessage() + "\r\n"
                    + SplitUtils.getKeyStack( e, this ) );
        } finally {
            if ( commSdb != null ) {
                commSdb.disconnect();
            }
        }
    }

    public void insertData() {
        try {
            DBCollection cl = commSdb.getCollectionSpace( csName )
                    .getCollection( clName );
            for ( int i = 0; i < 200; i++ ) {
                insertedData
                        .add( ( BSONObject ) JSON.parse( "{sk:" + i + "}" ) );
            }
            cl.bulkInsert( insertedData, 0 );
        } catch ( BaseException e ) {
            throw e;
        }
    }
}
