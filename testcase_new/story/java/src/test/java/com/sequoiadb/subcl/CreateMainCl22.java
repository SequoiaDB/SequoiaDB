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
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * FileName: CreateMainCl22.java test content: alter主表 testlink case: seqDB-22
 * 
 * @author zengxianquan
 * @date 2016年12月9日
 * @version 1.00
 */
public class CreateMainCl22 extends SdbTestBase {

    private Sequoiadb db = null;
    private String clName = "maincl_22";
    private CollectionSpace cs = null;
    private DBCollection cl = null;

    @BeforeClass
    public void setUp() {
        try {
            db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        } catch ( BaseException e ) {
            Assert.fail( "connect  failed," + SdbTestBase.coordUrl
                    + e.getMessage() );
        }
        if ( SubCLUtils.isStandAlone( db ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            cs.dropCollection( clName );
        } catch ( BaseException e ) {
            Assert.assertEquals( e.getErrorCode(), -23, e.getMessage() );
        } finally {
            if ( db != null ) {
                db.disconnect();
            }
        }
    }

    @Test
    public void testAlterMaincl() {
        try {
            cs = db.getCollectionSpace( SdbTestBase.csName );
            BSONObject options = new BasicBSONObject();
            options.put( "IsMainCL", true );
            BSONObject opt = new BasicBSONObject();
            opt.put( "a", 1 );
            options.put( "ShardingKey", opt );
            options.put( "ShardingType", "range" );
            cl = cs.createCollection( clName, options );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        }
        BSONObject alterOptions = new BasicBSONObject();
        BSONObject shardingKey = new BasicBSONObject();
        shardingKey.put( "a", -1 );
        alterOptions.put( "ShardingKey", shardingKey );
        try {
            cl.alterCollection( alterOptions );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        }
    }
}
