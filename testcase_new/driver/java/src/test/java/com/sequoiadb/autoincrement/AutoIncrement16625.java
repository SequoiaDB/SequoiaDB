package com.sequoiadb.autoincrement;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @TestLink: seqDB-16625
 * @describe: 创建/删除自增字段
 * @author wangkexin
 * @Date 2018.12.28
 * @version 1.00
 */
public class AutoIncrement16625 extends SdbTestBase {
    private Sequoiadb sdb;
    private CollectionSpace cs;
    private String clName = "cl16625";
    private String coordAddr;

    @BeforeClass
    public void setUp() {
        this.coordAddr = SdbTestBase.coordUrl;
        this.sdb = new Sequoiadb( this.coordAddr, "", "" );
        this.cs = this.sdb.getCollectionSpace( SdbTestBase.csName );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "run mode is standalone,test case skip" );
        }
    }

    @Test
    public void test() {
        // options 为非法值
        TestInvalidArg();
        // 所有参数均给非默认值
        TestNoDefaultOption();
        // 只传入自增字段名
        TestOptionField();
        // 删除自增字段
        TestDropAutoIncrement();
    }

    @AfterClass
    public void tearDown() {
        if ( cs.isCollectionExist( clName ) ) {
            cs.dropCollection( clName );
        }
        sdb.close();
    }

    public void TestInvalidArg() {
        DBCollection cl = cs.createCollection( clName );
        BSONObject options;
        // empty
        try {
            cl.createAutoIncrement( new BasicBSONObject() );
            Assert.fail( "options is empty should be fail!" );
        } catch ( BaseException e ) {
            Assert.assertEquals( e.getErrorCode(), -6 );
        }
        // invalid arg
        try {
            options = new BasicBSONObject();
            options.put( "test16625", true );
            cl.createAutoIncrement( options );
            Assert.fail( "options is invalid parameter should fail!" );
        } catch ( BaseException e ) {
            Assert.assertEquals( e.getErrorCode(), -6 );
        }
        cs.dropCollection( clName );
    }

    public void TestNoDefaultOption() {
        DBCollection cl = cs.createCollection( clName );
        BSONObject options = new BasicBSONObject();
        options.put( "Field", "name" );
        options.put( "Increment", 5 );
        options.put( "StartValue", 3 );
        options.put( "MinValue", 2 );
        options.put( "MaxValue", 10000 );
        options.put( "CacheSize", 1500 );
        options.put( "AcquireSize", 1200 );
        options.put( "Cycled", true );
        options.put( "Generated", "strict" );
        cl.createAutoIncrement( options );
        BSONObject autoIncrementInfo = new BasicBSONObject();
        autoIncrementInfo = Commlib.GetAutoIncrement( sdb,
                SdbTestBase.csName + "." + clName );
        Assert.assertEquals( autoIncrementInfo.get( "Field" ).toString(),
                "name" );
        Assert.assertEquals( autoIncrementInfo.get( "Generated" ).toString(),
                "strict" );

        String sequenceName = autoIncrementInfo.get( "SequenceName" )
                .toString();
        BSONObject actOptions = new BasicBSONObject();
        actOptions = Commlib.GetSequenceSnapshot( sdb, sequenceName );
        Assert.assertEquals( actOptions.get( "AcquireSize" ).toString(),
                "1200" );
        Assert.assertEquals( actOptions.get( "CacheSize" ).toString(), "1500" );
        Assert.assertEquals( actOptions.get( "Increment" ).toString(), "5" );
        Assert.assertEquals( actOptions.get( "MaxValue" ).toString(), "10000" );
        Assert.assertEquals( actOptions.get( "MinValue" ).toString(), "2" );
        Assert.assertEquals( actOptions.get( "StartValue" ).toString(), "3" );
        Assert.assertEquals( actOptions.get( "Cycled" ).toString(), "true" );

        // insert records
        BSONObject record = new BasicBSONObject();
        record.put( "_id", 1 );
        cl.insert( record );

        record = new BasicBSONObject();
        record.put( "_id", 2 );
        cl.insert( record );

        BSONObject matcher = new BasicBSONObject();
        matcher.put( "_id", 2 );
        // 8 = StartValue(3) + Increment(5)
        matcher.put( "name", 8 );
        Assert.assertEquals( cl.getCount( matcher ), 1 );
        cs.dropCollection( clName );
    }

    public void TestOptionField() {
        DBCollection cl = cs.createCollection( clName );
        BSONObject options = new BasicBSONObject();
        options.put( "Field", "number" );
        cl.createAutoIncrement( options );
        BSONObject autoIncrementInfo = new BasicBSONObject();
        autoIncrementInfo = Commlib.GetAutoIncrement( sdb,
                SdbTestBase.csName + "." + clName );
        Assert.assertEquals( autoIncrementInfo.get( "Field" ).toString(),
                "number" );
        Assert.assertEquals( autoIncrementInfo.get( "Generated" ).toString(),
                "default" );

        // insert records
        BSONObject record = new BasicBSONObject();
        record.put( "_id", 1 );
        cl.insert( record );

        record = new BasicBSONObject();
        record.put( "_id", 2 );
        cl.insert( record );

        BSONObject matcher = new BasicBSONObject();
        matcher.put( "_id", 2 );
        matcher.put( "number", 2 );
        Assert.assertEquals( cl.getCount( matcher ), 1 );
        cs.dropCollection( clName );
    }

    public void TestDropAutoIncrement() {
        DBCollection cl = cs.createCollection( clName );
        BSONObject options = new BasicBSONObject();
        options.put( "Field", "num" );
        cl.createAutoIncrement( options );
        // 删除不存在的自增字段
        try {
            cl.dropAutoIncrement( "test_nonexistent_field16625" );
            Assert.fail(
                    "dropAutocrement with nonexistent field should fail!" );
        } catch ( BaseException e ) {
            Assert.assertEquals( e.getErrorCode(), -333 );
        }

        // 删除自增字段为空
        try {
            cl.dropAutoIncrement( "" );
            Assert.fail( "dropAutocrement with '' field should fail!" );
        } catch ( BaseException e ) {
            Assert.assertEquals( e.getErrorCode(), -6 );
        }
        // 删除已存在的自增字段
        cl.dropAutoIncrement( "num" );
        BSONObject autoIncrementInfo = new BasicBSONObject();
        autoIncrementInfo = Commlib.GetAutoIncrement( sdb,
                SdbTestBase.csName + "." + clName );
        Assert.assertEquals( autoIncrementInfo.keySet().size(), 0,
                "the autoincrement still exist!" );
        cs.dropCollection( clName );
    }

}
