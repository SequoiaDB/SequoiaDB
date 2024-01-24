package com.sequoiadb.backup;

import java.util.Date;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.metadata.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;

public class Backup7174 extends SdbTestBase {
    private Sequoiadb sdb;
    private CommLib commlib = new CommLib();
    private String coordAddr;

    @BeforeClass
    public void setUp() {
        this.coordAddr = SdbTestBase.coordUrl;
        try {
            sdb = new Sequoiadb( coordAddr, "", "" );
            if ( commlib.isStandAlone( sdb ) ) {
                throw new SkipException(
                        "run mode is standalone,test case skip" );
            }
        } catch ( BaseException e ) {
            Assert.fail( "prepare env failed" + e.getMessage() );
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            sdb.removeBackup( null );
            sdb.disconnect();
        } catch ( BaseException e ) {
            Assert.fail( "clear env failed, errMsg:" + e.getMessage() );
        }
    }

    @Test
    public void test() {
        // set backup configure
        String backupName = "backup7174";
        BSONObject option = new BasicBSONObject();
        option.put( "Name", backupName );

        // backup
        try {
            sdb.backupOffline( option );
        } catch ( BaseException e ) {
            Assert.fail( "backup failed, errMsg:" + e.getMessage() );
        }

        // list
        try {
            DBCursor cursor = sdb.listBackup( null, null, null, null );
            while ( cursor.hasNext() ) {
                BSONObject record = cursor.getNext();
                if ( record.containsField( "Name" ) ) {
                    String actualBackupName = ( String ) record.get( "Name" );

                    // check
                    Assert.assertEquals( actualBackupName, backupName );
                }
            }
            cursor.close();
        } catch ( BaseException e ) {
            Assert.fail( "list backup failed, errMsg:" + e.getMessage() );
        }

        // remove backup
        try {
            sdb.removeBackup( null );
            // check
            DBCursor cursor = sdb.listBackup( null, null, null, null );
            while ( cursor.hasNext() ) {
                BSONObject record = cursor.getNext();
                if ( record.containsField( "Name" ) ) {
                    String actualBackupName = ( String ) record.get( "Name" );
                    Assert.assertNotEquals( actualBackupName, backupName );
                }
            }
        } catch ( BaseException e ) {
            Assert.fail( "remove backup failed, errMsg:" + e.getMessage() );
        } finally {
            sdb.removeBackup( null );
        }
    }
}
