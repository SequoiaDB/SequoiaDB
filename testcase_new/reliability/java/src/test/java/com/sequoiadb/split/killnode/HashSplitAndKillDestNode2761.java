package com.sequoiadb.split.killnode;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.GroupWrapper;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.fault.KillNode;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BSONDecimal;
import org.bson.types.BasicBSONList;
import org.bson.types.ObjectId;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.ArrayList;
import java.util.Date;
import java.util.List;
import java.util.regex.Pattern;

/**
 * FileName: HashSplitAndKillDestNode2761.java test content:when the hash cl
 * split by percent , kill -9 the dest group master node* testlink
 * case:seqDB-2761
 * 
 * @author wuyan
 * @Date 2017.4.11
 * @version 1.00
 */

public class HashSplitAndKillDestNode2761 extends SdbTestBase {
    private String clName = "split2761";
    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private DBCollection cl;
    private String srcGroupName;
    private String destGroupName;
    private GroupMgr groupMgr = null;
    private boolean clearFlag = false;

    @BeforeClass()
    public void setUp() {
        try {

            // check the current cluster,if there is an exception to return
            // false
            groupMgr = GroupMgr.getInstance();
            if ( !groupMgr.checkBusiness() ) {
                throw new SkipException( "checkBusiness return false" );
            }

            // specify source and target groups
            List< GroupWrapper > glist = groupMgr.getAllDataGroup();
            srcGroupName = glist.get( 0 ).getGroupName();
            destGroupName = glist.get( 1 ).getGroupName();
            System.out.println( "split srcRG:" + srcGroupName + " destRG:"
                    + destGroupName );

            sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            createCL();
            // insert 1000 records,the "no" value is 0-1000
            bulkInsert( cl, 0, 1000 );
        } catch ( ReliabilityException e ) {
            Assert.fail( this.getClass().getName()
                    + " setUp error, error description:" + e.getMessage()
                    + "\r\n" + Utils.getStackString( e ) );
        }
    }

    @Test
    public void test() {
        try {
            // get the source group master hostname and port
            String destHostName = sdb.getReplicaGroup( destGroupName )
                    .getMaster().getHostName();
            int destPort = sdb.getReplicaGroup( destGroupName ).getMaster()
                    .getPort();
            System.out.println( "KillNode:" + destHostName + ":" + destPort );

            // create concurrent tasks
            FaultMakeTask faultTask = KillNode.getFaultMakeTask( destHostName,
                    String.valueOf( destPort ), 5, 50 );
            TaskMgr mgr = new TaskMgr( faultTask );
            mgr.addTask( new Split() );
            mgr.execute();

            // TaskMgr check if there is any exception
            Assert.assertEquals( mgr.isAllSuccess(), true, mgr.getErrorMsg() );

            // check whether the cluster is normal，the longest waiting time is
            // 600S
            Assert.assertEquals( groupMgr.checkBusiness( 600 ), true,
                    "failed to restore business" );

            // insert 500 records after split,the incremental value of 0,the
            // "no" value is 1000-1500
            bulkInsert( cl, 1000, 1500 );

            checkSplitResult();

            // Normal operating environment
            clearFlag = true;
        } catch ( ReliabilityException e ) {
            Assert.fail( e.getMessage() + "\r\n" + Utils.getStackString( e ) );
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( clearFlag ) {
                cs.dropCollection( clName );
            }
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() + "\r\n" + Utils.getStackString( e ) );
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    public void createCL() {
        try {
            if ( !sdb.isCollectionSpaceExist( SdbTestBase.csName ) ) {
                sdb.createCollectionSpace( SdbTestBase.csName );
            }
        } catch ( BaseException e ) {
            // -33 CS exist,ignore exceptions
            Assert.assertEquals( -33, e.getErrorCode(), e.getMessage() );
        }
        String test = "{ShardingKey:{no:1},ShardingType:'hash',Partition:4096,"
                + "Compressed:true,Group:'" + srcGroupName + "'}";
        BSONObject options = ( BSONObject ) JSON.parse( test );
        try {
            cs = sdb.getCollectionSpace( SdbTestBase.csName );
            cl = cs.createCollection( clName, options );
        } catch ( BaseException e ) {
            Assert.assertTrue( false, "create cl fail " + e.getErrorType() + ":"
                    + e.getMessage() );
        }
    }

    public static void bulkInsert( DBCollection cl, long startNo, long endNo ) {
        try {
            List< BSONObject > list = new ArrayList< BSONObject >();
            // long num = 10;
            for ( long i = startNo; i < endNo; i++ ) {
                BSONObject obj = new BasicBSONObject();
                ObjectId id = new ObjectId();
                obj.put( "_id", id );
                obj.put( "test", "test" + i );
                // insert the decimal type data
                String str = "32345.067891234567890123456789" + i;
                BSONDecimal decimal = new BSONDecimal( str );
                obj.put( "decimal", decimal );
                obj.put( "no", i );
                obj.put( "str", "test_" + String.valueOf( i ) );
                // the numberlong type data
                BSONObject numberlong = new BasicBSONObject();
                numberlong.put( "$numberLong", "-9223372036854775808" );
                obj.put( "numlong", numberlong );
                // the obj type
                BSONObject subObj = new BasicBSONObject();
                subObj.put( "a", 1 + i );
                obj.put( "obj", subObj );
                // the array type
                BSONObject arr = new BasicBSONList();
                arr.put( "0", ( int ) ( Math.random() * 100 ) );
                arr.put( "1", "test" );
                arr.put( "2", 2.34 );
                obj.put( "arr", arr );
                obj.put( "boolf", false );
                // the data type
                Date now = new Date();
                obj.put( "date", now );
                // the regex type
                Pattern regex = Pattern.compile( "^2001",
                        Pattern.CASE_INSENSITIVE );
                obj.put( "binary", regex );
                list.add( obj );
            }
            cl.insert( list, DBCollection.FLG_INSERT_CONTONDUP );
        } catch ( BaseException e ) {
            Assert.assertTrue( false,
                    "bulkinsert fail " + e.getErrorCode() + e.getMessage() );
        }
    }

    class Split extends OperateTask {
        @Override
        public void exec() throws Exception {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                db.setSessionAttr(
                        ( BSONObject ) JSON.parse( "{PreferedInstance:'M'}" ) );
                DBCollection dbcl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                dbcl.split( srcGroupName, destGroupName, 50 );
            } catch ( BaseException e ) {
                throw e;
            } finally {
                if ( db != null ) {
                    db.close();
                }
            }
        }
    }

    private long checkGroupData( long expectRecNums, String groupName ) {
        Sequoiadb dataNode = null;
        long count = 0;
        try {
            dataNode = sdb.getReplicaGroup( groupName ).getMaster().connect();
            DBCollection cl1 = dataNode.getCollectionSpace( SdbTestBase.csName )
                    .getCollection( clName );
            count = cl1.getCount();

            // the count of the deviation should be less than 0.3
            long expCount = expectRecNums / 2;
            float testabs = Math.abs( expectRecNums / 2 - count );
            float offset = testabs / expCount;
            Assert.assertEquals( offset < 0.3, true, "actual split count:"
                    + count + " offset gerater than 0.3" );
        } catch ( BaseException e ) {
            e.printStackTrace();
            Assert.fail( e.getMessage() + "\r\n" + Utils.getStackString( e ) );
        } finally {
            if ( dataNode != null ) {
                dataNode.close();
            }
        }
        return count;
    }

    private void checkSplitResult() {
        try {
            // check data for source and target groups
            long expectRecNums = 1500;
            long destCount = checkGroupData( expectRecNums, destGroupName );
            long srcCount = checkGroupData( expectRecNums, srcGroupName );
            long actRecNums = destCount + srcCount;
            Assert.assertEquals( actRecNums, expectRecNums,
                    "insert records num error: " + actRecNums );

            // check all records
            for ( long i = 0; i < expectRecNums; i++ ) {
                Assert.assertEquals( cl.getCount( "{no:" + i + "}" ), 1,
                        "incorrect record number is " + i );
            }

            // data consistency check between groups，try 60 times at most
            GroupWrapper srcGroup = groupMgr.getGroupByName( srcGroupName );
            GroupWrapper destGroup = groupMgr.getGroupByName( destGroupName );
            Assert.assertEquals( srcGroup.checkInspect( 60 ), true );
            Assert.assertEquals( destGroup.checkInspect( 60 ), true );
        } catch ( BaseException | ReliabilityException e ) {
            Assert.fail( e.getMessage() + "\r\n" + Utils.getStackString( e ) );
        }

    }

}
