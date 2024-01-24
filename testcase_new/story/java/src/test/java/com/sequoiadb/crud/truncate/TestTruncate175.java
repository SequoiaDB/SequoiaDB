package com.sequoiadb.crud.truncate;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
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
 * @FileName:seqDB-175:update与truncate的并发 插入数据，一条线程执行update，另一条线程执行truncate
 * @Author linsuqiang
 * @Date 2016-12-06
 * @Version 1.00
 */
public class TestTruncate175 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String clName = "cl175";
    private BSONObject modifier = null;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        DBCollection cl = TruncateUtils.createCL( sdb, csName, clName );
        // doing insert
        TruncateUtils.insertData( cl );
        // prepare data for update
        modifier = new BasicBSONObject();
        BSONObject updatedValue = new BasicBSONObject();
        updatedValue.put( "newKey", "is updated" );
        modifier.put( "$set", updatedValue );
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
        UpdateThread updateThread = new UpdateThread();

        updateThread.start();
        truncateThread.start();

        if ( !( truncateThread.isSuccess() && updateThread.isSuccess() ) ) {
            Assert.fail(
                    truncateThread.getErrorMsg() + updateThread.getErrorMsg() );
        }
    }

    private class TruncateThread extends SdbThreadBase {
        @Override
        public void exec() throws BaseException {
            Sequoiadb db = null;
            DBCollection cl = null;
            try {
                db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                cl = db.getCollectionSpace( csName ).getCollection( clName );
                // doing truncate
                cl.truncate();
                // check truncate
                TruncateUtils.checkTruncated( db, cl );
            } finally {
                db.close();
            }
        }
    }

    private class UpdateThread extends SdbThreadBase {
        @Override
        public void exec() throws BaseException {
            Sequoiadb db = null;
            DBCollection cl = null;
            try {
                db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                cl = db.getCollectionSpace( csName ).getCollection( clName );
                // doing update
                cl.update( null, modifier, null );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != -321 ) {
                    throw e;
                }
            } finally {
                db.close();
            }
        }
    }
}
