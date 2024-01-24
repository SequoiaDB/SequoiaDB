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
 * @Description: seqDB-22489:同时执行插入、切分操作，检查会话快照中IsBlocked和Doing字段信息
 * @Author Zhao Xiaoni
 * @Date 2020.7.30
 */
public class Snapshot22489 extends SdbTestBase {
    private Sequoiadb sdb;
    private String clName = "cl_22489";
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
        sdb.getCollectionSpace( csName ).createCollection( clName, (BSONObject)JSON.parse( "{ ReplSize: 7, "
                + "ShardingKey: { 'a': 1 }, ShardingType: 'hash', Group: '" + groupName + "' }" ) );
    }
    
    @Test
    public void test() throws Exception{
        WriteLob writeLob = new WriteLob();
        writeLob.start();
        
        DBCursor cursor = null;
        do{ 
            Thread.sleep( 100 );
            cursor = sdb.getSnapshot( Sequoiadb.SDB_SNAP_SESSIONS, "{ 'NodeSelect': 'master', 'IsBlocked': true, "
                    + "'Doing': 'Waiting for freezing window(Name:story_java_test.cl_22489)' }", null, null );
            if( cursor.hasNext() ){
                isSuccess = true;
                break;
            }
        }while( times < totalTimes );

        Assert.assertTrue( writeLob.isSuccess() );
    }
    
    
    public class WriteLob extends SdbThreadBase{
        DBCollection cl = null;
        Split split = new Split();
        @Override
        public void exec() throws Exception {
            // TODO Auto-generated method stub
            try( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "", "" ) ){
                cl = db.getCollectionSpace( csName ).getCollection( clName );
                while( !isSuccess && times < totalTimes ){
                    times++;
                    DBLob lob = cl.createLob();
                    lob.write( lobSb.getBytes() );
                    lob.close();
                    if( times == 20 ){
                        split.start();
                    }
                }
                if( times >= totalTimes ){
                    System.err.println( "Insert time out!");
                }
                Assert.assertTrue( split.isSuccess() );
            }
        }
    }    
    
    public class Split extends SdbThreadBase{
        DBCollection cl = null;
        @Override
        public void exec() throws Exception {
            // TODO Auto-generated method stub
            try( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "", "" ) ){
                cl = db.getCollectionSpace( csName ).getCollection( clName );
                String tmpGroup = null;
                String srcGroup = groupName;
                String desGroup = groupNames.get( 1 );
                while( !isSuccess && times < totalTimes ){
                    cl.split( srcGroup, desGroup, 100 );
                    tmpGroup = srcGroup;
                    srcGroup = desGroup;
                    desGroup = tmpGroup;
                }
            }
        }
    }
    
    @AfterClass
    public void tearDown(){
        sdb.getCollectionSpace( csName ).dropCollection( clName );
        sdb.close();
    }
}
