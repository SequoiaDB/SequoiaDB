package com.sequoiadb.split;

import java.util.ArrayList;
import java.util.Date;
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

/**
 * @FileName:SEQDB-10507 不同coord执行查询和切分 1、连接coord1，创建cl，向cl中插入数据
 *                       2、连接coord2，执行split，指定切分条件（如指定切分范围）
 *                       3、连接coord1查看数据切分结果，指定条件查询数据，查询条件分别覆盖如下情况：
 *                       a、匹配查询源组数据（count带条件查询） b、匹配查询目标组数据 c、边界值数据查询（find查询）
 * @author huangqiaohui
 * @version 1.00
 *
 */

public class Split10507 extends SdbTestBase {
    private String clName = "testcaseCL10507";
    private String srcGroupName;
    private String destGroupName;
    private Sequoiadb commSdbA = null;
    private Sequoiadb commSdbB = null;
    private ArrayList< BSONObject > insertedData = new ArrayList< BSONObject >();// 记录所有已插入的数据

    @BeforeClass
    public void setUp() {
        Sequoiadb tmpSdb = null;
        try {
            tmpSdb = new Sequoiadb( coordUrl, "", "" );

            // 跳过 standAlone 和数据组不足的环境
            CommLib commlib = new CommLib();
            if ( commlib.isStandAlone( tmpSdb ) ) {
                throw new SkipException( "skip StandAlone" );
            }
            ArrayList< String > groupsName = commlib
                    .getDataGroupNames( tmpSdb );
            List< String > coordList = commlib.getNodeAddress( tmpSdb,
                    "SYSCoord" );
            if ( groupsName.size() < 2 ) {
                throw new SkipException(
                        "current environment less than tow groups " );
            }
            if ( coordList.size() < 2 ) {
                throw new SkipException(
                        "host list less than tow groups:" + coordList );
            }

            commSdbA = new Sequoiadb( coordList.get( 0 ), "", "" );
            commSdbB = new Sequoiadb( coordList.get( 1 ), "", "" );
            srcGroupName = groupsName.get( 0 );
            destGroupName = groupsName.get( 1 );
        } catch ( BaseException e ) {
            if ( commSdbA != null ) {
                commSdbA.disconnect();
            }
            if ( commSdbB != null ) {
                commSdbB.disconnect();
            }
            if ( tmpSdb != null ) {
                tmpSdb.disconnect();
            }
            Assert.fail( this.getClass().getName()
                    + " setUp error, error description:" + e.getMessage()
                    + "\r\n" + SplitUtils.getKeyStack( e, this ) );
        }
    }

    @Test
    public void split() {
        try {
            insertData();// 通过commSdbA建表，写入待切分的记录[sk:0,sk:100)

            // 通过commSdbB链接切分
            DBCollection commCL = commSdbB.getCollectionSpace( csName )
                    .getCollection( clName );
            commCL.split( srcGroupName, destGroupName,
                    ( BSONObject ) JSON.parse( "{sk:30}" ),
                    ( BSONObject ) JSON.parse( "{sk:60}" ) );

            checkCoordA();// 链接commSdbA查询源组数据目标组数据，并查询切分边界值
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() + "\r\n"
                    + SplitUtils.getKeyStack( e, this ) );
        }
    }

    private void checkCoordA() {
        DBCursor cursor = null;
        try {
            // 查询目标，源组数据
            commSdbA.setSessionAttr(
                    ( BSONObject ) JSON.parse( "{PreferedInstance:'M'}" ) );
            DBCollection commCL = commSdbA.getCollectionSpace( csName )
                    .getCollection( clName );
            long destCount = commCL.getCount( "{sk:{$gte:30,$lt:60}}" );
            Assert.assertEquals( destCount, 30 );

            // commCL.getCount("{$or:[{sk:{$gte:{$minKey:1},$lt:30}},{sk:{$gte:60,$lt:{$maxKey:1}}}]}");
            long srcCount1 = commCL.getCount( "{sk:{$gte:60}}" );
            long srcCount2 = commCL.getCount( "{sk:{$lt:30}}" );
            Assert.assertEquals( srcCount2 + srcCount1, 70 );

            // find边界sk:30,sk:60
            cursor = commCL.query( "{$or:[{sk:30},{sk:60}]}", "{sk:''}",
                    "{sk:1}", null );
            ArrayList< BSONObject > actualResults = new ArrayList< BSONObject >();// 实际结果集
            while ( cursor.hasNext() ) {
                actualResults.add( cursor.getNext() );
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
            if ( cursor != null ) {
                cursor.close();
            }
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            CollectionSpace commCS = commSdbA.getCollectionSpace( csName );
            commCS.dropCollection( clName );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() + "\r\n"
                    + SplitUtils.getKeyStack( e, this ) );
        } finally {
            if ( commSdbA != null ) {
                commSdbA.disconnect();
            }
            if ( commSdbB != null ) {
                commSdbB.disconnect();
            }
        }
    }

    public void insertData() {
        try {
            DBCollection cl = commSdbA.getCollectionSpace( csName )
                    .createCollection( clName, ( BSONObject ) JSON.parse(
                            "{ShardingKey:{sk:1},ShardingType:'range',Group:'"
                                    + srcGroupName + "'}" ) );
            for ( int i = 0; i < 100; i++ ) {
                insertedData
                        .add( ( BSONObject ) JSON.parse( "{sk:" + i + "}" ) );
            }
            cl.bulkInsert( insertedData, 0 );
        } catch ( BaseException e ) {
            throw e;
        }
    }
}
