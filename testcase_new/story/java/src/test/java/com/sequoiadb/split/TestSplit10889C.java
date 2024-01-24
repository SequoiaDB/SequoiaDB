package com.sequoiadb.split;

import java.util.ArrayList;
import java.util.Date;
import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
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
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * 1、向主表中插入数据，其中分区键字段分别覆盖多种数据类型，参考如下组合： a、long、timestamp b、date、float、int
 * c、_id+decimal 2、执行数据切分（根据1中组合覆盖同步切分和异步切分）
 * 3、查看数据切分结果（分别连接coord、源组data、目标组data查询 )
 * 
 * @author chensiqin
 * @Date 2016-12-21
 *
 */
public class TestSplit10889C extends SdbTestBase {
    private Sequoiadb sdb;
    private CollectionSpace cs;
    private DBCollection mainCL;
    private DBCollection subCL;
    private String mainCLName = "mainCL10889C_0";
    private String subCLName = "subCL10889C_0";
    private ArrayList< BSONObject > insertRecords;

    @BeforeClass
    public void setUp() {
        try {
            this.sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            // 跳过 standAlone 和数据组不足的环境
            SplitUtils2 util = new SplitUtils2();
            if ( util.isStandAlone( this.sdb ) ) {
                throw new SkipException( "skip StandAlone" );
            }
            if ( SplitUtils2.getDataRgNames( this.sdb ).size() < 2 ) {
                throw new SkipException(
                        "current environment less than tow groups " );
            }
            BSONObject options = new BasicBSONObject();
            options.put( "PreferedInstance", "M" );
            this.sdb.setSessionAttr( options );
            this.cs = this.sdb.getCollectionSpace( SdbTestBase.csName );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        }
    }

    @Test
    public void test() {
        // 得到数据组
        List< String > rgNames = SplitUtils2.getDataRgNames( this.sdb );
        // 创建主表
        BSONObject option = ( BSONObject ) JSON.parse(
                "{IsMainCL:true,ShardingKey:{_id:-1,bdecimal:1},ShardingType:\"range\"}" );
        this.mainCL = SplitUtils2.createCL( this.cs, this.mainCLName, option );

        // a、_id+decimal
        option = ( BSONObject ) JSON.parse( "{ShardingKey:{_id:-1,bdecimal:1},"
                + "ShardingType:\"hash\",Partition:1024,Group:\""
                + rgNames.get( 0 ) + "\"}" );
        this.subCL = SplitUtils2.createCL( this.cs, this.subCLName, option );
        // 挂载子表
        option = ( BSONObject ) JSON.parse( "{LowBound:{_id:111,"
                + "bdecimal:{\"decimal\":\"12344.06789123456789012345000\"}}, "
                + "UpBound:{_id:-10,"
                + "bdecimal:{\"decimal\":\"12346.06789123456789012345900\"}}}" );
        this.mainCL.attachCollection( SdbTestBase.csName + "." + this.subCLName,
                option );
        this.insertRecords = ( ArrayList< BSONObject > ) SplitUtils2
                .insertData( this.mainCL, 100 );

        BSONObject startCondition = new BasicBSONObject();
        BSONObject endCondition = new BasicBSONObject();
        startCondition.put( "Partition", 256 );
        endCondition.put( "Partition", 768 );
        // 执行切分
        this.subCL.split( rgNames.get( 0 ), rgNames.get( 1 ), startCondition,
                endCondition );

        // 主表连接coord节点验证数据是否正确
        testCoordSplitResult( rgNames );
        // 子表连接源组data验证数据
        testSrcDataSplitResult( rgNames );
        // 子表连接目标组data验证数据
        testDestDataSplitResult( rgNames );
    }

    public void testCoordSplitResult( List< String > rgNames ) {
        try {
            // 连接coord节点验证数据是否正确
            List< BSONObject > actual = new ArrayList< BSONObject >();
            DBCursor cursor = this.mainCL.query( null, null, "{\"_id\":1}",
                    null );
            while ( cursor.hasNext() ) {
                BSONObject obj = cursor.getNext();
                actual.add( obj );
            }
            cursor.close();
            Assert.assertEquals( actual, this.insertRecords );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        }

    }

    /**
     * 连接源数据组检查切分结果
     * 
     * @param rgNames
     */
    public void testSrcDataSplitResult( List< String > rgNames ) {
        Sequoiadb dataDb = null;
        try {
            // 连接源组data验证数据
            String url = SplitUtils2.getGroupIPByGroupName( this.sdb,
                    rgNames.get( 0 ) );
            dataDb = new Sequoiadb( url, "", "" );
            CollectionSpace cs = dataDb
                    .getCollectionSpace( SdbTestBase.csName );
            DBCollection dbcl = cs.getCollection( this.subCLName );
            long actual = dbcl.getCount();
            // partition[0,512]
            if ( Math.abs( ( 50 - actual ) ) / 50.0 > 0.3 ) {
                Assert.fail( "hash 范围切分不够准确！" );
            }
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        } finally {
            dataDb.disconnect();
        }
    }

    /**
     * 在目标数组组上查询切分结果
     * 
     * @param rgNames
     */
    public void testDestDataSplitResult( List< String > rgNames ) {
        Sequoiadb dataDb = null;
        try {
            // 连接目标组data查询
            String url = SplitUtils2.getGroupIPByGroupName( this.sdb,
                    rgNames.get( 1 ) );
            dataDb = new Sequoiadb( url, "", "" );
            CollectionSpace cs = dataDb
                    .getCollectionSpace( SdbTestBase.csName );
            DBCollection dbcl = cs.getCollection( this.subCLName );
            long actual = dbcl.getCount();
            // partition[0,512]
            if ( Math.abs( ( 50 - actual ) ) / 50.0 > 0.3 ) {
                Assert.fail( "hash 范围切分不够准确！" );
            }
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        } finally {
            dataDb.disconnect();
        }

    }

    @AfterClass
    public void tearDown() {
        try {
            if ( this.cs.isCollectionExist( this.mainCLName ) ) {
                this.cs.dropCollection( this.mainCLName );
            }
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        } finally {
            this.sdb.disconnect();
        }
    }
}
