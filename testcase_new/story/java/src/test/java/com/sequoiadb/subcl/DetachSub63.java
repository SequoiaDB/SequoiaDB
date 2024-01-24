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
 * 
 * FileName: DetachSub63 test content: detach一个未挂载的子表 testlink case: seqDB-63
 * 
 * @author zengxianquan
 * @date 2016年12月30日
 * @version 1.00 other: 存在的BUG已经修复 对应JIRA问题单：2134
 */
public class DetachSub63 extends SdbTestBase {

    private Sequoiadb db = null;
    private DBCollection maincl = null;
    private CollectionSpace cs = null;
    private String subclName1 = "subcl_63_1";
    private String subclName2 = "subcl_63_2";
    private String mainclName = "maincl_63";

    @BeforeClass
    public void setUp() {
        try {
            db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            cs = db.getCollectionSpace( SdbTestBase.csName );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
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
            if ( cs.isCollectionExist( subclName1 ) ) {
                cs.dropCollection( subclName1 );
            }
        } catch ( BaseException e ) {
            Assert.fail(
                    "failed to drop cl " + "ErrorMsg:\n" + e.getMessage() );
        } finally {
            db.disconnect();
        }
    }

    @Test
    public void testDetachButNotAttach() {
        createMainclAndSubcl();
        try {
            maincl.detachCollection( SdbTestBase.csName + "." + subclName1 );
            Assert.fail( "detach subclName1 successfully" );
        } catch ( BaseException e ) {
            // 在detach一个未attach的子表时，不报错，提了问题单，等到修复完成后，请下面一句的注释释放，同时加上对应的错误码
            Assert.assertEquals( e.getErrorCode(), -242, e.getMessage() );
        }
        try {
            maincl.detachCollection( SdbTestBase.csName + "." + subclName2 );
            Assert.fail( "detach subclName2 successfully" );
        } catch ( BaseException e ) {
            Assert.assertEquals( e.getErrorCode(), -23, e.getMessage() );
        }
    }

    public void createMainclAndSubcl() {
        BSONObject options = new BasicBSONObject();
        options.put( "IsMainCL", true );
        BSONObject opt = new BasicBSONObject();
        opt.put( "a", 1 );
        options.put( "ShardingKey", opt );
        options.put( "ShardingType", "range" );
        try {
            maincl = cs.createCollection( mainclName, options );
            cs.createCollection( subclName1 );
        } catch ( BaseException e ) {
            e.printStackTrace();
            Assert.fail( e.getMessage() );
        }
    }

}
