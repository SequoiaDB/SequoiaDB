package com.sequoiadb.subcl;

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
import com.sequoiadb.testcommon.SdbThreadBase;

/**
 * 
 * FileName: DetachSub64 test content: 对于同一个CL，一边做attach，一边做detach testlink
 * case: seqDB64
 * 
 * @author zengxianquan
 * @date 2016年12月19日
 * @version 1.00 other: 存在的BUG已修复 对应JIRA问题单：2134
 */
public class DetachSub64 extends SdbTestBase {

    private Sequoiadb sdb = null;
    private String subclName = "subcl_64";
    private String mainclName = "maincl_64";

    @BeforeClass
    public void setUp() {
        try {
            sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        }
        if ( SubCLUtils.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }
        createMainclAndSubcl();
    }

    @AfterClass
    public void tearDown() {
        try {
            CollectionSpace cs = sdb.getCollectionSpace( SdbTestBase.csName );
            if ( cs.isCollectionExist( mainclName ) ) {
                cs.dropCollection( mainclName );
            }
            if ( cs.isCollectionExist( subclName ) ) {
                cs.dropCollection( subclName );
            }
        } catch ( BaseException e ) {
            Assert.fail(
                    "failed to drop cl " + "ErrorMsg:\n" + e.getMessage() );
        } finally {
            sdb.disconnect();
        }
    }

    @Test
    public void test() {
        SdbThreadBase attachThread = new AttachThread();
        SdbThreadBase detachThread = new DetachThread();
        attachThread.start( 10 );
        detachThread.start( 10 );
        if ( !attachThread.isSuccess() ) {
            Assert.fail( attachThread.getErrorMsg() );
        }
        if ( !detachThread.isSuccess() ) {
            Assert.fail( detachThread.getErrorMsg() );
        }
    }

    class AttachThread extends SdbThreadBase {

        @Override
        public void exec() throws BaseException {
            Sequoiadb db = null;
            DBCollection maincl = null;

            BSONObject attachOpt = new BasicBSONObject();
            BSONObject lowBound = new BasicBSONObject();
            BSONObject upBound = new BasicBSONObject();
            lowBound.put( "time", 0 );
            upBound.put( "time", 100 );
            attachOpt.put( "LowBound", lowBound );
            attachOpt.put( "UpBound", upBound );
            try {
                db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                maincl = db.getCollectionSpace( csName )
                        .getCollection( mainclName );
                maincl.attachCollection( SdbTestBase.csName + "." + subclName,
                        attachOpt );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != -235 && e.getErrorCode() != -147 ) {
                    throw e;
                }
            } finally {
                if ( db != null ) {
                    db.disconnect();
                }
            }
        }

    }

    class DetachThread extends SdbThreadBase {

        @Override
        public void exec() throws Exception {
            Sequoiadb db = null;
            DBCollection maincl = null;
            try {
                db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                maincl = db.getCollectionSpace( csName )
                        .getCollection( mainclName );
                maincl.detachCollection( SdbTestBase.csName + "." + subclName );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != -242 && e.getErrorCode() != -147
                        && e.getErrorCode() != -190 ) {
                    throw e;
                }
            } finally {
                if ( db != null ) {
                    db.disconnect();
                }
            }
        }

    }

    public void createMainclAndSubcl() {
        CollectionSpace cs = null;
        BSONObject options = new BasicBSONObject();
        options.put( "IsMainCL", true );
        BSONObject opt = new BasicBSONObject();
        opt.put( "time", 1 );
        options.put( "ShardingKey", opt );
        options.put( "ShardingType", "range" );
        try {
            cs = sdb.getCollectionSpace( SdbTestBase.csName );
            cs.createCollection( mainclName, options );
            cs.createCollection( subclName );
        } catch ( BaseException e ) {
            e.printStackTrace();
            Assert.fail( e.getMessage() );
        }
    }
}
