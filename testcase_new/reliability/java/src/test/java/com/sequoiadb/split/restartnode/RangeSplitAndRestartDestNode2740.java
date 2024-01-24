package com.sequoiadb.split.restartnode;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.GroupWrapper;
import com.sequoiadb.commlib.NodeWrapper;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.fault.NodeRestart;
import com.sequoiadb.split.brokennetwork.Utils;
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
 * FileName: RangeSplitAndRestartDestNode2740.java test content:when the range
 * cl split by persent , restart the dest group master node* testlink
 * case:seqDB-2740
 * 
 * @author wuyan
 * @Date 2017.4.19
 * @version 1.00
 */

public class RangeSplitAndRestartDestNode2740 extends SdbTestBase {
    private final String clName = "split2740";
    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private DBCollection cl;
    private String srcGroupName;
    private String destGroupName;
    private GroupMgr groupMgr = null;
    private boolean clearFlag = false;
    private boolean isSplitComplete = false;

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
            // insert 10000 records,the "no" value is 0-10000
            bulkInsert( cl, 0, 10000 );
        } catch ( ReliabilityException e ) {
            Assert.fail( this.getClass().getName()
                    + " setUp error, error description:" + e.getMessage()
                    + "\r\n" + Utils.getStackString( e ) );
        }
    }

    @Test
    public void test() {
        try {
            // get the dest group master hostname and port
            GroupWrapper destGroup = groupMgr.getGroupByName( destGroupName );
            NodeWrapper destMaster = destGroup.getMaster();

            // create concurrent tasks
            FaultMakeTask faultTask = NodeRestart.getFaultMakeTask( destMaster,
                    1, 10, 10 );
            TaskMgr mgr = new TaskMgr( faultTask );
            mgr.addTask( new Split() );
            mgr.execute();

            // TaskMgr check if there is any exception
            Assert.assertEquals( mgr.isAllSuccess(), true, mgr.getErrorMsg() );

            // check whether the cluster is normal，the longest waiting time is
            // 600S
            Assert.assertEquals( groupMgr.checkBusiness( 600 ), true,
                    "failed to restore business" );

            // split success,then check result
            if ( isSplitComplete ) {
                // insert 500 records after split,the incremental value of 0,the
                // "no" value is 10000-10500
                bulkInsert( cl, 10000, 10500 );

                checkSplitResult();
            }

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
        String test = "{ShardingKey:{no:1},ShardingType:'range',"
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
            try ( Sequoiadb db1 = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                db1.setSessionAttr(
                        ( BSONObject ) JSON.parse( "{PreferedInstance:'M'}" ) );
                DBCollection dbcl = db1.getCollectionSpace( csName )
                        .getCollection( clName );
                try {
                    dbcl.split( srcGroupName, destGroupName, 50 );
                    isSplitComplete = true;
                } catch ( BaseException e ) {
                    System.out.println(
                            "split have exception:" + e.getMessage() );
                }

            } catch ( BaseException e ) {
                throw e;
            }
        }
    }

    private long checkGroupData( String groupName, String matcher,
            long expCount ) {
        long count = 0;
        try ( Sequoiadb dataNode = sdb.getReplicaGroup( groupName ).getMaster()
                .connect()) {
            DBCollection cl1 = dataNode.getCollectionSpace( SdbTestBase.csName )
                    .getCollection( clName );
            count = cl1.getCount( matcher );
            Assert.assertEquals( count, expCount,
                    "the count of " + groupName + " is:" + count );
        } catch ( BaseException e ) {
            e.printStackTrace();
            Assert.fail( e.getMessage() + "\r\n" + Utils.getStackString( e ) );
        }
        return count;
    }

    private void checkSplitResult() {
        try {
            // check data for source and target groups
            long expectRecNums = 10500;
            String destMatcher = "{no:{$gte:5000}}";
            long destExpectNums = 5500;
            String srcMatcher = "{no:{$lt:5000}}";
            long srcExpectNums = 5000;
            long destCount = checkGroupData( destGroupName, destMatcher,
                    destExpectNums );
            long srcCount = checkGroupData( srcGroupName, srcMatcher,
                    srcExpectNums );
            long actRecNums = destCount + srcCount;
            Assert.assertEquals( actRecNums, expectRecNums,
                    "insert records num error: " + actRecNums );

            // check all records,check the value of "no"
            DBCursor tmpCursor = cl.query( null, null, "{ _id: 1 }", null );
            for ( long i = 0; i < expectRecNums; i++ ) {
                long actValue = ( long ) tmpCursor.getNext().get( "no" );
                long expValue = i;
                Assert.assertEquals( actValue, expValue,
                        "incorrect record number is " + i );
            }
            tmpCursor.close();

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
