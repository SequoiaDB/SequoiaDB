package com.sequoiadb.crud.backup;

import java.util.Date;

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
 * @FileName:seqDB-172:离线备份与truncate的并发 插入数据，一条线程执行离线备份，另一条线程执行truncate
 * @Author linsuqiang
 * @Date 2016-12-06
 * @Version 1.00
 */
public class TestTruncate172 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String clName = "cl_172";
    BSONObject options = null;

    @BeforeClass
    public void setUp() {
        try {
            sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        }
        try {
            DBCollection cl = BackUpUtils.createCL( sdb, csName, clName );
            // doing insert
            BackUpUtils.insertData( cl );
            // prepare data for backup offline
            options = new BasicBSONObject();
            options.put( "Name", "backupName" );
            options.put( "Description", "backup for all" );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            CollectionSpace cs = sdb.getCollectionSpace( csName );
            if ( cs.isCollectionExist( clName ) ) {
                cs.dropCollection( clName );
            }
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        } finally {
            sdb.disconnect();
        }
    }

    @Test
    public void test() {
        TruncateThread truncateThread = new TruncateThread();
        BackupThread backupThread = new BackupThread();

        backupThread.start();
        truncateThread.start();

        if ( !( truncateThread.isSuccess() && backupThread.isSuccess() ) ) {
            Assert.fail(
                    truncateThread.getErrorMsg() + backupThread.getErrorMsg() );
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
                BackUpUtils.checkTruncated( db, cl, hostName );
            } catch ( BaseException e ) {
                throw e;
            } finally {
                db.disconnect();
            }
        }
    }

    private class BackupThread extends SdbThreadBase {
        @Override
        public void exec() throws BaseException {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                // doing backup
                db.backupOffline( options );
            } catch ( Exception e ) {
                throw e;
            } finally {
                db.disconnect();
            }
        }
    }
}