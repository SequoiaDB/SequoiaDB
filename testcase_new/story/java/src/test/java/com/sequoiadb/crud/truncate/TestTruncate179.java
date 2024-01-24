package com.sequoiadb.crud.truncate;

import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
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
 * @FileName:seqDB-179:dropCL与truncate的并发 插入数据，一条线程执行dropCL，另一条线程执行truncate
 * @Author linsuqiang
 * @Date 2016-12-06
 * @Version 1.00
 */
public class TestTruncate179 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String clName = "cl179";

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        DBCollection cl = TruncateUtils.createCL( sdb, csName, clName );
        TruncateUtils.insertData( cl );
    }

    @AfterClass
    public void tearDown() {
        try {
            CollectionSpace cs = sdb.getCollectionSpace( csName );
            if ( cs.isCollectionExist( clName ) ) {
                cs.dropCollection( clName );
            }
        } finally {
            sdb.close();
        }
    }

    @Test
    public void test() {
        TruncateThread truncateThread = new TruncateThread();
        DropClThread dropClThread = new DropClThread();

        truncateThread.start();
        dropClThread.start();

        if ( !( truncateThread.isSuccess() && dropClThread.isSuccess() ) ) {
            Assert.fail(
                    truncateThread.getErrorMsg() + dropClThread.getErrorMsg() );
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
                cl = db.getCollectionSpace( csName ).getCollection( clName );
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

    private class DropClThread extends SdbThreadBase {
        @Override
        public void exec() throws BaseException {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                db.setSessionAttr(
                        ( BSONObject ) JSON.parse( "{PreferedInstance:'M'}" ) );
                // doing drop CL
                db.getCollectionSpace( csName ).dropCollection( clName );
            } catch ( Exception e ) {
                // all exceptions are acceptable, as long as no core dump and
                // hanging
            } finally {
                db.close();
            }
        }
    }
}