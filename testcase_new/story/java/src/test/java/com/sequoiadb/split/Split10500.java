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
 * @FileName:SEQDB-10500 range分区表指定范围异步切分数据 1、向cl中插入数据记录
 *                       2、执行splitAsync异步切分，设置范围切分条件
 *                       3、查看数据切分结果（分别连接coord、源组data、目标组data查询，执行find查询覆盖边界范围数据，
 *                       执行data节点count带条件查询）， 4、再次插入数据（数据分别在源和目标组范围内） 5、查看插入数据结果
 * @author huangqiaohui
 * @version 1.00
 *
 */

public class Split10500 extends SdbTestBase {
    private String clName = "testcaseCL10500";
    private String srcGroupName;
    private String destGroupName;
    private Sequoiadb commSdb = null;
    private DBCollection commCL;
    private ArrayList< BSONObject > insertedData = new ArrayList< BSONObject >();// 记录所有已插入的数据

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
            commCL = commCS.createCollection( clName,
                    ( BSONObject ) JSON.parse(
                            "{ShardingKey:{sk:1},ShardingType:\"range\",Group:'"
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
            insertData( 0, 100 );// 写入待切分的记录[sk:0,sk:100)
            long[] taskIdArr = new long[ 1 ];
            taskIdArr[ 0 ] = commCL.splitAsync( srcGroupName, destGroupName,
                    ( BSONObject ) JSON.parse( "{sk:30}" ), // 切分
                    ( BSONObject ) JSON.parse( "{sk:60}" ) );

            commSdb.waitTasks( taskIdArr );

            // 期望目标组有30条符合{sk:{$gte:30,$lt:60}}查询条件的数据,期望目标组共有30条数据
            checkDestGroup( 30, "{sk:{$gte:30,$lt:60}}", 30 );
            // 期望源组有70条符合查询条件的数据,期望源组共有70条数据
            checkSrcGroup( 70,
                    "{$or:[{sk:{$gte:0,$lt:30}},{sk:{$gte:60,$lt:100}}]}", 70 );

            insertAndCheckAgain();// 插入数据，检查落入

            checkCoord();// 协调节点比对已插入的数据，并查询切分边界值
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() + "\r\n"
                    + SplitUtils.getKeyStack( e, this ) );
        }
    }

    public void insertAndCheckAgain() {
        BSONObject obj1 = ( BSONObject ) JSON.parse( "{sk:50,flag:-1}" );// 期望此数据落入目标组
        BSONObject obj2 = ( BSONObject ) JSON.parse( "{sk:10,flag:-1}" );// 期望此数据落入源组
        try {
            commCL.insert( obj1 );
            commCL.insert( obj2 );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() + "\r\n"
                    + SplitUtils.getKeyStack( e, this ) );
        }
        insertedData.add( obj1 );
        insertedData.add( obj2 );
        checkDestGroup( 1, "{sk:50,flag:-1}", 31 );// 检测落入
        checkSrcGroup( 1, "{sk:10,flag:-1}", 71 );// 检测落入
    }

    private void checkCoord() {
        DBCursor cursor1 = null;
        DBCursor cursor2 = null;
        DBCursor cursor3 = null;
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

            // find边界sk:30,sk:60
            cursor2 = commCL.query( "{sk:30}", "{sk:''}", "{sk:1}", null );
            cursor3 = commCL.query( "{sk:60}", "{sk:''}", "{sk:1}", null );
            ArrayList< BSONObject > actualResults = new ArrayList< BSONObject >();// 实际结果集
            while ( cursor2.hasNext() ) {
                actualResults.add( cursor2.getNext() );
            }
            while ( cursor3.hasNext() ) {
                actualResults.add( cursor3.getNext() );
            }
            ArrayList< BSONObject > expectedResults = new ArrayList< BSONObject >();// 期望结果集
            expectedResults.add( ( BSONObject ) JSON.parse( "{sk:30}" ) );
            expectedResults.add( ( BSONObject ) JSON.parse( "{sk:60}" ) );

            Assert.assertEquals( expectedResults.equals( actualResults ), true,
                    "query bound expected:" + expectedResults + " actual:"
                            + actualResults );// 比对

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
            if ( cursor3 != null ) {
                cursor3.close();
            }
        }
    }

    private void checkDestGroup( int expectedCount, String macher,
            int expectTotalCount ) {
        Sequoiadb destDataNode = null;
        try {
            destDataNode = commSdb.getReplicaGroup( destGroupName ).getMaster()
                    .connect();// 获得目标组主节点链接
            DBCollection destCL = destDataNode.getCollectionSpace( csName )
                    .getCollection( clName );
            long count = destCL.getCount( macher );
            Assert.assertEquals( count, expectedCount );// 目标组应当含有上述查询数据
            Assert.assertEquals( destCL.getCount(), expectTotalCount ); // 目标组应当含有的数据量
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() + "\r\n"
                    + SplitUtils.getKeyStack( e, this ) );
        } finally {
            if ( destDataNode != null ) {
                destDataNode.disconnect();
            }
        }
    }

    private void checkSrcGroup( int expectedCount, String macher,
            int expectTotalCount ) {
        Sequoiadb srcDataNode = null;
        try {
            srcDataNode = commSdb.getReplicaGroup( srcGroupName ).getMaster()
                    .connect();// 获得源主节点链接
            DBCollection srcCL = srcDataNode.getCollectionSpace( csName )
                    .getCollection( clName );
            long count = srcCL.getCount( macher );
            Assert.assertEquals( count, expectedCount );// 源组数据应当含有上述查询数据
            Assert.assertEquals( srcCL.getCount(), expectTotalCount ); // 源数据应当仅含有上述查询数据
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() + "\r\n"
                    + SplitUtils.getKeyStack( e, this ) );
        } finally {
            if ( srcDataNode != null ) {
                srcDataNode.disconnect();
            }
        }
    }

    @AfterClass
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

    public void insertData( int bengin, int end ) {
        try {
            for ( int i = bengin; i < end; i++ ) {
                insertedData
                        .add( ( BSONObject ) JSON.parse( "{sk:" + i + "}" ) );
            }
            commCL.bulkInsert( insertedData, 0 );
        } catch ( BaseException e ) {
            throw e;
        }
    }
}
