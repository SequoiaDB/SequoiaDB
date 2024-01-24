package com.sequoiadb.subcl;

import java.util.Date;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * FileName: CreateMainCl18.java test content: 创建主表时指定分区类型为hash testlink case:
 * seqDB-18
 * 
 * @author zengxianquan
 * @date 2016年12月9日
 * @version 1.00
 */
public class CreateMainCl18 extends SdbTestBase {

    private String clName = "maincl_18";
    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;

    @BeforeClass
    public void setUp() {
        try {
            sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        } catch ( BaseException e ) {
            Assert.fail( "connect  failed " + "ErrorMsg:\n" + e.getMessage() );
        }
        if ( SubCLUtils.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }
    }

    @AfterClass
    public void tearDown() {

        try {
            if ( cs.isCollectionExist( clName ) ) {
                cs.dropCollection( clName );
            }
        } catch ( BaseException e ) {
            Assert.assertTrue( false,
                    "clean up failed " + "ErrorMsg:\n" + e.getMessage() );
        } finally {
            if ( sdb != null ) {
                sdb.disconnect();
            }
        }
    }

    @Test
    public void testCreateMainclByHash() {
        try {
            cs = sdb.getCollectionSpace( SdbTestBase.csName );
        } catch ( BaseException e ) {
            Assert.fail( "get collectionSpace failed " + "ErrorMsg:\n"
                    + e.getMessage() );
        }
        BSONObject options = new BasicBSONObject();
        options.put( "IsMainCL", true );
        BSONObject opt = new BasicBSONObject();
        opt.put( "a", 1 );
        options.put( "ShardingKey", opt );
        options.put( "ShardingType", "hash" );
        try {
            cs.createCollection( clName, options );
            Assert.fail( "create maincl by hash successfully " );
        } catch ( BaseException e ) {
            Assert.assertEquals( e.getErrorCode(), -244, e.getMessage() );
        }
    }
}
