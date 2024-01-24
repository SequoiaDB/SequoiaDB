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
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * 
 * FileName: DropMainSub83 test content: 未对子表进行detach，drop子表 testlink case:
 * seqDB-83
 * 
 * @author zengxianquan
 * @date 2016年12月12日
 * @version 1.00
 */
public class DropMainSub83 extends SdbTestBase {

    private Sequoiadb db = null;
    private DBCollection maincl = null;
    private CollectionSpace cs = null;
    private String subclName1 = "subcl_83_1";
    private String subclName2 = "subcl_83_2";
    private String mainclName = "maincl_83";

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
                    "failed to drop cl " + "ErrorMsg:\n" + e.getMessage() );
        } finally {
            db.disconnect();
        }
    }

    @Test
    public void dropSubNoDetach() {
        cs = db.getCollectionSpace( SdbTestBase.csName );

        createAndAttachSubcl();

        // 插入数据
        List< BSONObject > insertor1 = new ArrayList<>();
        for ( int i = 0; i < 100; i++ ) {
            BSONObject bson = new BasicBSONObject();
            bson.put( "time", i );
            bson.put( "test", "testData" );
            insertor1.add( bson );
        }
        try {
            maincl.bulkInsert( insertor1, 1 );
        } catch ( BaseException e ) {
            Assert.fail( "failed to bulkInsert " + e.getMessage() );
        }
        // 未detach就删除子表1
        try {
            cs.dropCollection( subclName1 );
        } catch ( BaseException e ) {
            Assert.fail( "failed to drop subcl" + e.getMessage() );
        }
        // 在主表向删除的子表插入数据
        List< BSONObject > insertor2 = new ArrayList<>();
        for ( int i = 0; i < 100; i++ ) {
            BSONObject bson = new BasicBSONObject();
            bson.put( "time", i );
            bson.put( "test", "testData" );
            insertor2.add( bson );
        }
        try {
            maincl.bulkInsert( insertor2, 1 );
        } catch ( BaseException e ) {
            Assert.assertEquals( e.getErrorCode(), -135, e.getMessage() );
        }
        // 在主表向未删除的子表插入数据
        List< BSONObject > insertor3 = new ArrayList<>();
        for ( int i = 100; i < 200; i++ ) {
            BSONObject bson = new BasicBSONObject();
            bson.put( "time", i );
            bson.put( "test", "testData" );
            insertor3.add( bson );
        }
        try {
            maincl.bulkInsert( insertor3, 1 );
        } catch ( BaseException e ) {
            Assert.fail( "failed to bulkInsert " + e.getMessage() );
        }
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
}
