package com.sequoiadb.autoincrement;

import java.util.ArrayList;
import java.util.List;

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
 * @TestLink: seqDB-16626
 * @describe: 创建/删除多个自增字段
 * @author wangkexin
 * @Date 2018.12.29
 * @version 1.00
 */
public class AutoIncrement16626 extends SdbTestBase {
    private Sequoiadb sdb;
    private CollectionSpace cs;
    private String clName = "cl16626";
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
        // 同时创建多个自增字段
        TestMultiAutoIncrementOption();
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
        List< BSONObject > optionsList = new ArrayList< BSONObject >();
        BSONObject options = new BasicBSONObject();
        optionsList.add( options );
        // empty list
        try {
            cl.createAutoIncrement( optionsList );
            Assert.fail( "options is empty should be fail!" );
        } catch ( BaseException e ) {
            Assert.assertEquals( e.getErrorCode(), -6 );
        }
        // invalid arg list
        try {
            optionsList = new ArrayList< BSONObject >();
            options = new BasicBSONObject();
            options.put( "Field", "number" );
            optionsList.add( options );

            options = new BasicBSONObject();
            options.put( "test16626", true );
            optionsList.add( options );
            cl.createAutoIncrement( optionsList );
            Assert.fail( "options is invalid parameter should fail!" );
        } catch ( BaseException e ) {
            Assert.assertEquals( e.getErrorCode(), -6 );
        }

        try {
            optionsList = new ArrayList< BSONObject >();
            options = new BasicBSONObject();
            options.put( "invalid_arg16626_1", "number" );
            optionsList.add( options );

            options = new BasicBSONObject();
            options.put( "invalid_arg16626_2", true );
            optionsList.add( options );
            cl.createAutoIncrement( optionsList );
            Assert.fail( "options is invalid parameter should fail!" );
        } catch ( BaseException e ) {
            Assert.assertEquals( e.getErrorCode(), -6 );
        }
        cs.dropCollection( clName );
    }

    public void TestMultiAutoIncrementOption() {
        DBCollection cl = cs.createCollection( clName );
        List< BSONObject > optionsList = new ArrayList< BSONObject >();
        BSONObject options = new BasicBSONObject();
        options.put( "Field", "num16626" );
        options.put( "Increment", 2 );
        options.put( "StartValue", 1 );
        options.put( "MinValue", 1 );
        options.put( "MaxValue", 5000 );
        options.put( "CacheSize", 2000 );
        options.put( "AcquireSize", 2000 );
        options.put( "Cycled", true );
        options.put( "Generated", "strict" );
        optionsList.add( options );

        // put another one options to optionsList
        options = new BasicBSONObject();
        options.put( "Field", "age16626" );
        options.put( "Increment", 2 );
        options.put( "StartValue", 1 );
        options.put( "MinValue", 1 );
        options.put( "MaxValue", 5000 );
        options.put( "CacheSize", 2000 );
        options.put( "AcquireSize", 2000 );
        options.put( "Cycled", true );
        options.put( "Generated", "strict" );
        optionsList.add( options );

        cl.createAutoIncrement( optionsList );
        List< BSONObject > autoIncrementInfos = Commlib.GetAutoIncrementList(
                sdb, SdbTestBase.csName + "." + clName, 2 );
        List< String > fieldNames = new ArrayList<>();
        fieldNames.add( autoIncrementInfos.get( 0 ).get( "Field" ).toString() );
        fieldNames.add( autoIncrementInfos.get( 1 ).get( "Field" ).toString() );
        Assert.assertTrue( fieldNames.contains( "num16626" ) );
        Assert.assertTrue( fieldNames.contains( "age16626" ) );

        for ( int i = 0; i < 2; i++ ) {
            Assert.assertEquals(
                    autoIncrementInfos.get( i ).get( "Generated" ).toString(),
                    "strict" );

            String sequenceName = autoIncrementInfos.get( i )
                    .get( "SequenceName" ).toString();
            BSONObject actOptions = Commlib.GetSequenceSnapshot( sdb,
                    sequenceName );
            Assert.assertEquals( actOptions.get( "AcquireSize" ).toString(),
                    "2000" );
            Assert.assertEquals( actOptions.get( "CacheSize" ).toString(),
                    "2000" );
            Assert.assertEquals( actOptions.get( "Increment" ).toString(),
                    "2" );
            Assert.assertEquals( actOptions.get( "MaxValue" ).toString(),
                    "5000" );
            Assert.assertEquals( actOptions.get( "MinValue" ).toString(), "1" );
            Assert.assertEquals( actOptions.get( "StartValue" ).toString(),
                    "1" );
            Assert.assertEquals( actOptions.get( "Cycled" ).toString(),
                    "true" );
        }

        // insert records
        BSONObject record = new BasicBSONObject();
        record.put( "_id", 1 );
        cl.insert( record );

        record = new BasicBSONObject();
        record.put( "_id", 2 );
        cl.insert( record );

        BSONObject matcher = new BasicBSONObject();
        matcher.put( "_id", 2 );
        // 3 = StartValue(1) + Increment(2)
        matcher.put( "num16626", 3 );
        matcher.put( "age16626", 3 );
        Assert.assertEquals( cl.getCount( matcher ), 1 );
        cs.dropCollection( clName );
    }

    public void TestDropAutoIncrement() {
        DBCollection cl = cs.createCollection( clName );
        List< BSONObject > optionsList = new ArrayList< BSONObject >();
        BSONObject options = new BasicBSONObject();
        options.put( "Field", "num16626" );
        optionsList.add( options );

        options = new BasicBSONObject();
        options.put( "Field", "age16626" );
        optionsList.add( options );
        cl.createAutoIncrement( optionsList );

        List< String > fieldNames = new ArrayList<>();
        fieldNames.add( "test16626_1" );
        fieldNames.add( "test16626_2" );
        // 删除不存在的自增字段
        try {
            cl.dropAutoIncrement( fieldNames );
            Assert.fail(
                    "dropAutocrement with nonexistent field names should fail!" );
        } catch ( BaseException e ) {
            Assert.assertEquals( e.getErrorCode(), -333 );
        }

        // 删除自增字段为空
        try {
            List< String > noneIncrement = new ArrayList<>();
            cl.dropAutoIncrement( noneIncrement );
            Assert.fail( "dropAutocrement with '' field should fail!" );
        } catch ( BaseException e ) {
            Assert.assertEquals( e.getErrorCode(), -6 );
        }
        // 删除已存在的多个自增字段
        fieldNames = new ArrayList<>();
        fieldNames.add( "num16626" );
        fieldNames.add( "age16626" );

        cl.dropAutoIncrement( fieldNames );
        List< BSONObject > autoIncrementInfos = new ArrayList<>();
        autoIncrementInfos = Commlib.GetAutoIncrementList( sdb,
                SdbTestBase.csName + "." + clName, 1 );
        Assert.assertEquals( autoIncrementInfos.size(), 0,
                "the autoincrement still exist!" );
        cs.dropCollection( clName );
    }
}
