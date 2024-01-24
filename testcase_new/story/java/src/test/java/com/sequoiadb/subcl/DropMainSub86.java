package com.sequoiadb.subcl;

import java.util.ArrayList;
import java.util.Date;
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
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * FileName: DropMainSub86 test content: detach子表后，drop子表 testlink case:
 * seqDB_86
 * 
 * @author zengxianquan
 * @date 2016年12月12日
 * @version 1.00
 */
public class DropMainSub86 extends SdbTestBase {

    private Sequoiadb db = null;
    private DBCollection maincl = null;
    private CollectionSpace cs = null;
    private String subclName1 = "subcl_86_1";
    private String subclName2 = "subcl_86_2";
    private String mainclName = "maincl_86";

    @BeforeClass
    public void setUp() {
        try {
            db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        } catch ( BaseException e ) {
            Assert.assertTrue( false, "connect  failed," + SdbTestBase.coordUrl
                    + e.getMessage() );
        }
        if ( SubCLUtils.isStandAlone( db ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( cs.isCollectionExist( mainclName ) ) {
                cs.dropCollection( mainclName );
            }
        } catch ( BaseException e ) {
            Assert.fail(
                    "failed to drop maincl" + "ErrorMsg:\n" + e.getMessage() );
        } finally {
            db.disconnect();
        }
    }

    @Test
    public void dropSubAfterDetach() {
        cs = db.getCollectionSpace( SdbTestBase.csName );
        // 创建主子表，并挂载子表到主表中
        createAndAttachSubcl();
        // 在主表向表1插入数据，detach子表，drop子表，在主表查询该子表数据为空
        subcl1Option();
        // 在主表向另外范围的子表插入数据，插入成功
        subcl2Option();
    }

    public void createAndAttachSubcl() {
        BSONObject options = new BasicBSONObject();
        options.put( "IsMainCL", true );
        BSONObject opt = new BasicBSONObject();
        opt.put( "time", 1 );
        options.put( "ShardingKey", opt );
        try {
            maincl = cs.createCollection( mainclName, options );
        } catch ( BaseException e ) {
            Assert.fail( "failed to create maincl" + e.getMessage() );
        }
        try {
            cs.createCollection( subclName1 );
            cs.createCollection( subclName2 );
        } catch ( BaseException e ) {
            Assert.fail( "failed to create subcl" + e.getMessage() );
        }
        BSONObject attachOpt1 = new BasicBSONObject();
        BSONObject lowBound1 = new BasicBSONObject();
        BSONObject upBound1 = new BasicBSONObject();
        lowBound1.put( "time", 0 );
        upBound1.put( "time", 100 );
        attachOpt1.put( "LowBound", lowBound1 );
        attachOpt1.put( "UpBound", upBound1 );

        BSONObject attachOpt2 = new BasicBSONObject();
        BSONObject lowBound2 = new BasicBSONObject();
        BSONObject upBound2 = new BasicBSONObject();
        lowBound2.put( "time", 100 );
        upBound2.put( "time", 200 );
        attachOpt2.put( "LowBound", lowBound2 );
        attachOpt2.put( "UpBound", upBound2 );
        try {
            maincl.attachCollection( csName + "." + subclName1, attachOpt1 );
            maincl.attachCollection( csName + "." + subclName2, attachOpt2 );
        } catch ( BaseException e ) {
            Assert.fail( "failed to attach subcl" + e.getMessage() );
        }
    }

    public void subcl1Option() {
        // 向子表1插入数据
        List< BSONObject > insertor1 = new ArrayList<>();
        for ( int i = 0; i < 100; i++ ) {
            BSONObject bson = new BasicBSONObject();
            bson.put( "time", i );
            bson.put( "test", "testData1" );
            insertor1.add( bson );
        }
        try {
            maincl.bulkInsert( insertor1, 1 );
        } catch ( BaseException e ) {
            Assert.fail( "failed to bulkInsert " + e.getMessage() );
        }
        // detach 子表1
        try {
            maincl.detachCollection( SdbTestBase.csName + "." + subclName1 );
        } catch ( BaseException e ) {
            Assert.fail( "failed to detach subcl" + e.getMessage() );
        }
        // delete 子表1
        try {
            cs.dropCollection( subclName1 );
        } catch ( BaseException e ) {
            Assert.fail( "failed to drop subcl" + e.getMessage() );
        }
        // 在主表查询数据删除了的子表数据
        BSONObject matcher = new BasicBSONObject();
        matcher.put( "test", "testData1" );
        DBCursor cursor = null;
        try {
            cursor = maincl.query( matcher, null, null, null );
        } catch ( BaseException e ) {
            Assert.fail( "failed to query data " + e.getMessage() );
        }
        if ( cursor.hasNext() ) {
            Assert.fail( "failed to delete data" );
        }
    }

    public void subcl2Option() {
        // 在主表向未删除的子表插入数据
        List< BSONObject > insertor2 = new ArrayList<>();
        for ( int i = 100; i < 200; i++ ) {
            BSONObject bson = new BasicBSONObject();
            bson.put( "time", i );
            bson.put( "test", "testData2" );
            insertor2.add( bson );
        }
        try {
            maincl.bulkInsert( insertor2, 1 );
        } catch ( BaseException e ) {
            Assert.fail( "failed to bulkInsert " + e.getMessage() );
        }
    }

}
