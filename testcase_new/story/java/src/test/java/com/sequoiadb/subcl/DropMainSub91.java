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
 * FileName: DropMainSub91 test content: 未对子表进行detach，drop主表 testlink case:
 * seqDB_91
 * 
 * @author zengxianquan
 * @date 2016年12月12日
 * @version 1.00
 */
public class DropMainSub91 extends SdbTestBase {
    private Sequoiadb db = null;
    private DBCollection maincl = null;
    private CollectionSpace cs = null;
    private String subclName = "subcl_91";
    private String mainclName = "maincl_91";

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
                    "failed to drop maincl " + "ErrorMsg:\n" + e.getMessage() );
        } finally {
            db.disconnect();
        }
    }

    @Test
    public void dropSubNoDetach() {
        cs = db.getCollectionSpace( SdbTestBase.csName );
        // 创建主子表，并挂载主子表
        createAndAttachSubcl();

        // 插入数据
        List< BSONObject > insertor = new ArrayList<>();
        for ( int i = 0; i < 100; i++ ) {
            BSONObject bson = new BasicBSONObject();
            bson.put( "time", i );
            bson.put( "test", "testData" );
            insertor.add( bson );
        }
        try {
            maincl.bulkInsert( insertor, 1 );
        } catch ( BaseException e ) {
            Assert.fail( "failed to bulkInsert " + e.getMessage() );
        }
        // 删除主表
        try {
            cs.dropCollection( mainclName );
        } catch ( BaseException e ) {
            Assert.fail( "failed to drop maincl" + e.getMessage() );
        }
        // 检验是否还存在主子表
        checkIsExistCl();
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
            cs.createCollection( subclName );
        } catch ( BaseException e ) {
            Assert.fail( "failed to create subcl" + e.getMessage() );
        }
        BSONObject attachOpt = new BasicBSONObject();
        BSONObject lowBound = new BasicBSONObject();
        BSONObject upBound = new BasicBSONObject();
        lowBound.put( "time", 0 );
        upBound.put( "time", 100 );
        attachOpt.put( "LowBound", lowBound );
        attachOpt.put( "UpBound", upBound );
        try {
            maincl.attachCollection( csName + "." + subclName, attachOpt );
        } catch ( BaseException e ) {
            Assert.fail( "failed to attach subcl" + e.getMessage() );
        }
    }

    public void checkIsExistCl() {
        boolean existMaincl = true;
        boolean existsubcl = true;
        try {
            existMaincl = cs.isCollectionExist( mainclName );
            existsubcl = cs.isCollectionExist( subclName );
        } catch ( BaseException e ) {
            Assert.fail( "failed to judge " + e.getMessage() );
        }
        if ( existMaincl ) {
            Assert.fail( "failed to drop maincl" );
        }
        if ( existsubcl ) {
            Assert.fail( "failed to drop subcl" );
        }
    }
}