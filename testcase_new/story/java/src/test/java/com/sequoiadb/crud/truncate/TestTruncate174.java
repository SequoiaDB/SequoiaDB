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
 * @FileName:seqDB-174:truncate与truncate的并发 插入数据，多条线程同时执行truncate
 * @Author linsuqiang
 * @Date 2016-12-06
 * @Version 1.00
 */
public class TestTruncate174 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String clName = "cl174";

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
            cs.dropCollection( clName );
        } finally {
            sdb.close();
        }
    }

    @Test
    public void test() {
        TruncateThread truncateThread = new TruncateThread();

        truncateThread.start( 3 );

        if ( !truncateThread.isSuccess() ) {
            Assert.fail( truncateThread.getErrorMsg() );
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
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != -321 && e.getErrorCode() != -190
                        && e.getErrorCode() != -147 ) {
                    throw e;
                }
            } finally {
                db.close();
            }
        }
    }
}
