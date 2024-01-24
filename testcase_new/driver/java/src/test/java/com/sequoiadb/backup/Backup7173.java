package com.sequoiadb.backup;

import java.io.File;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * 此用例需要保证环境的每一台主机都有SdbTestBase.workDir指定的目录，并且权限应为777
 */
public class Backup7173 extends SdbTestBase {
    private Sequoiadb sdb;
    private String groupName = "group7173";
    private String csName = "cs7173";
    private String clName = "cl7173";
    private String backupName1 = "backup7173_1";
    private String backupName2 = "backup7173_2";
    private String path = "";
    private int nodeNum = 1;
    private boolean success = false;

    @BeforeClass
    public void setUp() {
        path = SdbTestBase.workDir + "/" + groupName;
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "run mode is standalone,test case skip" );
        }
        if ( sdb.isReplicaGroupExist( groupName ) ) {
            if ( sdb.isCollectionSpaceExist( csName ) ) {
                sdb.dropCollectionSpace( csName );
            }
            sdb.removeReplicaGroup( groupName );
        }
        File backupDir = new File( path );
        if ( backupDir.exists() ) {
            backupDir.delete();
        }
        BackupUtil.createRGAndNode( sdb, groupName, nodeNum );
        BSONObject options = new BasicBSONObject();
        options.put( "Group", groupName );
        DBCollection cl = sdb.createCollectionSpace( csName )
                .createCollection( clName, options );
        BackupUtil.insertData( cl );
    }

    @AfterClass
    public void tearDown() {
        BSONObject removeOption = new BasicBSONObject();
        removeOption.put( "GroupName", groupName );
        removeOption.put( "Path", path );
        if ( success ) {
            sdb.removeBackup( removeOption );
            sdb.dropCollectionSpace( csName );
            sdb.removeReplicaGroup( groupName );
            sdb.close();
        }
    }

    @Test
    public void test() {
        backup();
        listBackup();
        removeBackupAndCheck();
        success = true;
    }

    private void backup() {
        // set backup configure
        BSONObject option1 = new BasicBSONObject();
        option1.put( "Name", backupName1 );
        option1.put( "GroupName", groupName );
        option1.put( "Path", path );

        BSONObject option2 = new BasicBSONObject();
        option2.put( "Name", backupName2 );
        option2.put( "GroupName", groupName );
        option2.put( "Path", path );
        // backup
        // SEQUOIADBMAINSTREAM-4396 backupOffline改名为backup
        sdb.backup( option1 );
        sdb.backupOffline( option2 );
    }

    private void listBackup() {
        // list
        String hostName = sdb.getReplicaGroup( groupName ).getMaster()
                .getHostName();
        BSONObject listOption = new BasicBSONObject();
        listOption.put( "Path", path );
        listOption.put( "HostName", hostName );
        listOption.put( "GroupName", groupName );

        BSONObject matcher = new BasicBSONObject();
        matcher.put( "Name", backupName1 );

        BSONObject selector = new BasicBSONObject();
        selector.put( "Name", 1 );
        selector.put( "NodeName", 1 );
        selector.put( "GroupName", 1 );
        selector.put( "StartTime", 1 );

        BSONObject orderBy = new BasicBSONObject();
        orderBy.put( "GroupName", -1 );

        DBCursor cursor = sdb.listBackup( listOption, matcher, selector,
                orderBy );
        while ( cursor.hasNext() ) {
            BSONObject record = cursor.getNext();
            if ( record.containsField( "Name" ) ) {
                String actualBackupName = ( String ) record.get( "Name" );
                // check matcher
                Assert.assertEquals( actualBackupName, backupName1 );

                // check selector
                int keySetSize = record.keySet().size();
                Assert.assertEquals( keySetSize, 4 );
                Assert.assertTrue( record.containsField( "Name" ) );
                Assert.assertTrue( record.containsField( "NodeName" ) );
                Assert.assertTrue( record.containsField( "GroupName" ) );
                Assert.assertTrue( record.containsField( "StartTime" ) );
            }
        }
        cursor.close();
    }

    private void removeBackupAndCheck() {
        // remove backup
        BSONObject removeOption = new BasicBSONObject();
        removeOption.put( "Name", backupName1 );
        removeOption.put( "GroupName", groupName );
        removeOption.put( "Path", path );

        sdb.removeBackup( removeOption );

        BSONObject listOption2 = new BasicBSONObject();
        listOption2.put( "GroupName", groupName );
        listOption2.put( "Path", path );
        int returnedCount = 0;
        // check
        DBCursor cursor = sdb.listBackup( listOption2, null, null, null );
        while ( cursor.hasNext() ) {
            BSONObject record = cursor.getNext();
            String actualBackupName = ( String ) record.get( "Name" );
            Assert.assertEquals( actualBackupName, backupName2 );
            returnedCount++;
        }
        Assert.assertEquals( returnedCount, 1 );
    }
}
