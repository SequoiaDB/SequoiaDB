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
 * FileName: DropMainSub92 test content: drop主表所属表空间 testlink case: seqDB-92
 * 
 * @author zengxianquan
 * @date 2016年12月12日
 * @version 1.00
 */
public class DropMainSub92 extends SdbTestBase {
    private Sequoiadb db = null;
    private DBCollection maincl = null;
    private CollectionSpace maincs = null;
    private CollectionSpace cs = null;
    private String maincsName = "maincs_92";
    private String subclName1 = "subcl_92_1";
    private String subclName2 = "subcl_92_2";
    private String mainclName = "maincl_92";

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
            if ( cs.isCollectionExist( subclName1 ) ) {
                cs.dropCollection( subclName1 );
            }
        } catch ( BaseException e ) {
            Assert.fail(
                    "failed to drop maincl " + "ErrorMsg:\n" + e.getMessage() );
        } finally {
            db.disconnect();
        }
    }

    @Test
    public void testDropCs() {
        cs = db.getCollectionSpace( SdbTestBase.csName );
        maincs = db.createCollectionSpace( maincsName );

        // 创建主子表，并挂载主子表
        createAndAttachSubcl();

        // 插入数据
        List< BSONObject > insertor = new ArrayList<>();
        for ( int i = 0; i < 200; i++ ) {
            BSONObject bson = new BasicBSONObject();
            bson.put( "time", i );
            bson.put( "test", "testData" );
            insertor.add( bson );
        }
        try {
            maincl.bulkInsert( insertor, 1 );
            // detach子表1
            maincl.detachCollection( csName + "." + subclName1 );
            // 删除主表所在的cs
            db.dropCollectionSpace( maincsName );
        } catch ( BaseException e ) {
            e.printStackTrace();
            Assert.fail( e.getMessage() );
        }

        // 判断删除的cs是否还存在
        if ( db.isCollectionSpaceExist( maincsName ) ) {
            Assert.fail( "failed to drop cs" );
        }

        // 判断detach后的子表是否还存在
        if ( !cs.isCollectionExist( subclName1 ) ) {
            Assert.fail( "subcl is not exist" );
        }
    }

    public void createAndAttachSubcl() {
        BSONObject options = new BasicBSONObject();
        options.put( "IsMainCL", true );
        BSONObject opt = new BasicBSONObject();
        opt.put( "time", 1 );
        options.put( "ShardingKey", opt );
        try {
            maincl = maincs.createCollection( mainclName, options );
            cs.createCollection( subclName1 );
            maincs.createCollection( subclName2 );
        } catch ( BaseException e ) {
            e.printStackTrace();
            Assert.fail( e.getMessage() );
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
            maincl.attachCollection( maincsName + "." + subclName2,
                    attachOpt2 );
        } catch ( BaseException e ) {
            Assert.fail( "failed to attach subcl" + e.getMessage() );
        }
    }
}
