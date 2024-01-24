package com.sequoiadb.crud.truncate;

import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

/**
 * @FileName:seqDB-178:dropCS与truncate的并发 插入数据，一条线程执行dropCS，另一条线程执行truncate
 * @Author linsuqiang
 * @Date 2016-12-06
 * @Version 1.00
 */
public class TestTruncate178 extends SdbTestBase {

    private Sequoiadb sdb = null;
    private String myCsName = "cs_178";
    private String clName = "cl_178";

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( !sdb.isCollectionSpaceExist( myCsName ) ) {
            sdb.createCollectionSpace( myCsName );
        }
        DBCollection cl = TruncateUtils.createCL( sdb, myCsName, clName );
        TruncateUtils.insertData( cl );
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( sdb.isCollectionSpaceExist( myCsName ) ) {
                sdb.dropCollectionSpace( myCsName );
            }
        } finally {
            sdb.close();
        }
    }

    @Test
    public void test() {
        TruncateThread truncateThread = new TruncateThread();
        DropCsThread dropCsThread = new DropCsThread();

        truncateThread.start();
        dropCsThread.start();

        if ( !( truncateThread.isSuccess() && dropCsThread.isSuccess() ) ) {
            Assert.fail(
                    truncateThread.getErrorMsg() + dropCsThread.getErrorMsg() );
        }
    }

    private class TruncateThread extends SdbThreadBase {
        @Override
        public void exec() throws BaseException {
            Sequoiadb db = null;
            DBCollection cl = null;
            try {
                db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                db.setSessionAttr(
                        ( BSONObject ) JSON.parse( "{PreferedInstance:'M'}" ) );
                cl = db.getCollectionSpace( myCsName ).getCollection( clName );
                // doing truncate
                cl.truncate();
            } catch ( Exception e ) {
                // all exceptions are acceptable, as long as no core dump and
                // hanging
            } finally {
                db.close();
            }
        }
    }

    private class DropCsThread extends SdbThreadBase {
        @Override
        public void exec() throws BaseException {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                db.setSessionAttr(
                        ( BSONObject ) JSON.parse( "{PreferedInstance:'M'}" ) );
                // doing drop CS
                db.dropCollectionSpace( myCsName );
            } catch ( Exception e ) {
                // all exceptions are acceptable, as long as no core dump and
                // hanging
            } finally {
                db.close();
            }
        }
    }
}