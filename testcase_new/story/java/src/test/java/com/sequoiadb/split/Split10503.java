package com.sequoiadb.split;

import java.util.ArrayList;
import java.util.Date;

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

/**
 * @FileName:SEQDB-10503 hash子表指定范围切分数据 1、向主表中插入数据记录 2、子表执行split，设置范围切分条件
 *                       3、查看数据切分结果（分别连接coord查询主表数据、源组data、目标组data查询子表数据；执行find/
 *                       count查询）， 4、再次向主表插入数据（数据分别在源和目标组范围内） 5、查看插入数据结果
 * @author huangqiaohui
 * @version 1.00
 *
 */

public class Split10503 extends SdbTestBase {
    private String subCLName = "testcaseSubCL10503";
    private String mainCLName = "testcaseMainCL10503";
    private String srcGroupName;
    private String destGroupName;
    private Sequoiadb commSdb = null;
    private DBCollection subCL;
    private DBCollection mainCL;
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
            mainCL = commCS.createCollection( mainCLName, ( BSONObject ) JSON
                    .parse( "{ShardingKey:{sk:1},IsMainCL:true}" ) );
            subCL = commCS.createCollection( subCLName,
                    ( BSONObject ) JSON.parse(
                            "{ShardingKey:{sk:1},Partition:4096,ShardingType:'hash',Group:'"
                                    + srcGroupName + "'}" ) );
            mainCL.attachCollection( subCL.getFullName(), // 挂载
                    ( BSONObject ) JSON
                            .parse( "{LowBound:{sk:0},UpBound:{sk:200}}" ) );
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
            subCL.split( srcGroupName, destGroupName,
                    ( BSONObject ) JSON.parse( "{Partition:2048}" ), // 切分
                    ( BSONObject ) JSON.parse( "{Partition:4096}" ) );

            // 校验源和目标组普通记录
            ArrayList< BSONObject > insertedDataCopy = new ArrayList< BSONObject >(
                    insertedData );
            checkGroupData( insertedDataCopy, destGroupName );// 目标组中的数据应当是insertDataCopy的子集，校验完成后，删除insertDataCopy中属于子集的元素
            checkGroupData( insertedDataCopy, srcGroupName );// 校验源组
            Assert.assertEquals( insertedDataCopy.size() == 0, true, // 经过两次校验，insertDataCopy应当为空
                    "split error,destGroup and srcGroup can not find:"
                            + insertedDataCopy );

            // 插入数据，检查落入
            insertAndCheckAgain();
            // 通过协调节点比对已插入所有数据
            checkCoord();
        } catch ( BaseException e ) {
            e.printStackTrace();
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
                    .getCollection( subCLName );
            DBCollection srcCL = srcDataNode.getCollectionSpace( csName )
                    .getCollection( subCLName );

            // 得到一个目标组的记录，添加flag字段后再次插入，期望此记录落入目标组
            BSONObject destRecord = null;
            cursor1 = destCL.query( null, "{sk:''}", "{sk:1}", null, 0, 1 );
            if ( cursor1.hasNext() ) {
                destRecord = cursor1.getNext();
            } else {
                Assert.fail( "query fail" );
            }
            destRecord.put( "falg", -1 );
            mainCL.insert( destRecord );
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
            mainCL.insert( srcRecord );
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
            cursor1 = subCL.query( null, null, "{sk:1}", null );
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
                    .getCollection( subCLName );

            cursor = cl.query( null, null, "{sk:1}", null );
            while ( cursor.hasNext() ) {
                BSONObject actual = cursor.getNext();
                Assert.assertEquals( insertedData.contains( actual ), true,
                        "insertedData can not find this record:" + actual );
                insertedData.remove( actual );
            }
            long count = cl.getCount();
            // 组的数据量应该在100条左右（总量200，切分范围2048-4096）
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
            commCS.dropCollection( subCLName );
            commCS.dropCollection( mainCLName );
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
            for ( int i = 0; i < 200; i++ ) {
                insertedData
                        .add( ( BSONObject ) JSON.parse( "{sk:" + i + "}" ) );
            }
            mainCL.bulkInsert( insertedData, 0 );
        } catch ( BaseException e ) {
            throw e;
        }
    }
}
