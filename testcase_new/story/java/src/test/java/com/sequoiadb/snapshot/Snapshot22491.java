package com.sequoiadb.snapshot;

import java.util.List;
import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.DBLob;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.lob.utils.LobOprUtils;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;
/**
 * @Description: seqDB-22491:离线全量备份的同时向集合中插入数据，检查会话快照中IsBlocked和Doing字段信息
 * @Author Zhao Xiaoni
 * @Date 2020.7.30
 */
public class Snapshot22491 extends SdbTestBase {
    private Sequoiadb sdb;
    private String clName = "cl_22491";
    private DBCollection cl = null;
    private List< String > groupNames;
    private String groupName;
    private String lobSb;
    private int times = 0;
    private int totalTimes = 100;
    private static boolean isSuccess = false;
    
    @BeforeClass
    public void setup(){
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        lobSb = LobOprUtils.getRandomString( 1024*1024*10 );
        
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "STANDALONE MODE" );
        }
        groupNames = CommLib.getDataGroupNames( sdb );
        if ( groupNames.size() < 2 ) {
            throw new SkipException( "ONE GROUP MODE" );
        }
        
        groupName = groupNames.get( 0 );
        cl = sdb.getCollectionSpace( csName ).createCollection( clName, 
                (BSONObject)JSON.parse( "{ Group: '" + groupName + "' }" ) );
        for( int i = 0; i < 20; i++ ){
            DBLob lob = cl.createLob();
            lob.write( lobSb.getBytes() );
            lob.close();
        }
    }
    
    @Test
    public void test() throws Exception{
        Backup backup = new Backup();
        backup.start();

        DBCursor cursor = null;
        do{ 
            Thread.sleep( 100 );
            cursor = sdb.getSnapshot( Sequoiadb.SDB_SNAP_SESSIONS, "{ 'NodeSelect': 'master', "
                    + "'IsBlocked': true, 'Doing': 'Waiting for dms writable' }", null, null );
            if( cursor.hasNext() ){
                isSuccess = true;
                break;
            }
        }while( times < totalTimes );
        
        Assert.assertTrue( backup.isSuccess() );
    }
    
    public class Backup extends SdbThreadBase {
        InsertData insertData = new InsertData();
        @Override
        public void exec() {
            try( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "", "" ) ){
                while( !isSuccess && times < totalTimes ){
                    times++;
                    insertData.start();
                    db.backup( (BSONObject)JSON.parse( "{ 'GroupName': '" + groupName + "', "
                            + "Name: 'backup_22491_" + times + "' }" ));
                }
                if( times >= totalTimes ){
                    Assert.fail( "Backup time out!" );
                }
                Assert.assertTrue( insertData.isSuccess() );
            }
        }
    }
    
    public class InsertData extends SdbThreadBase{
        DBCollection cl = null;
        @Override
        public void exec() throws Exception {
            try( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "", "" ) ){
                cl = db.getCollectionSpace( csName ).getCollection( clName );
                while( !isSuccess && times < totalTimes ){
                    cl.insert( "{ 'a': " + times + "}" );
                }
            }
        }
    }
    
    @AfterClass
    public void tearDown(){
        for( int i = 0; i <= times; i++ ){
            sdb.removeBackup( (BSONObject)JSON.parse( "{ Name: 'backup_22491_" + i + "', "
                    + "'GroupName': '" + groupName + "' }" ) );
        }
        sdb.getCollectionSpace( csName ).dropCollection( clName );
        sdb.close();
    }
}
