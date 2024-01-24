package com.sequoiadb.split.brokennetwork;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.commlib.CommLib;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.GroupWrapper;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.fault.BrokenNetwork;
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
 * FileName: RangeSplitAndDestNodeCutNet2582.java test content:when the range cl
 * split by range ,the dest group master node breaks the net testlink
 * case:seqDB-2582
 * 
 * @author wuyan
 * @Date 2017.4.13
 * @version 1.00
 */

public class RangeSplitAndDestNodeCutNet2582 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private final String clName = "split2582";
    private CollectionSpace cs = null;
    private DBCollection cl;
    private String srcGroupName;
    private String destGroupName;
    private GroupMgr groupMgr = null;
    private String connectUrl;
    private String brokenNetHost;
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

            brokenNetHost = groupMgr.getGroupByName( destGroupName ).getSlave()
                    .hostName();
            Utils.reelect( brokenNetHost, srcGroupName, Utils.CATA_RG_NAME );
            connectUrl = CommLib.getSafeCoordUrl( brokenNetHost );
            groupMgr.refresh();
            System.out.println( "brokenHost:" + brokenNetHost + " connectUrl:"
                    + connectUrl );

            sdb = new Sequoiadb( connectUrl, "", "" );
            sdb.setSessionAttr(
                    ( BSONObject ) JSON.parse( "{PreferedInstance:'M'}" ) );
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
            // create concurrent tasks
            FaultMakeTask faultTask = BrokenNetwork
                    .getFaultMakeTask( brokenNetHost, 0, 15 );
            TaskMgr mgr = new TaskMgr( faultTask );
            mgr.addTask( new Split() );
            mgr.addTask( new Insert() );
            mgr.execute();

            // TaskMgr check if there is any exception
            Assert.assertEquals( mgr.isAllSuccess(), true, mgr.getErrorMsg() );

            // check whether the cluster is normal，the longest waiting time is
            // 120S
            Assert.assertEquals( groupMgr.checkBusiness( 600 ), true,
                    "failed to restore business" );

            // insert 500 records after split,the incremental value of 0,the
            // "no" value is 2000-2500
            bulkInsert( cl, 2000, 2500 );

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
        String test = "{ShardingKey:{no:1},ShardingType:'range',"
                + "Compressed:true,Group:'" + srcGroupName + "'}";
        BSONObject options = ( BSONObject ) JSON.parse( test );
        try {
            cs = sdb.getCollectionSpace( SdbTestBase.csName );
            cl = cs.createCollection( clName, options );
        } catch ( BaseException e ) {
            Assert.fail( "create cl fail " + e.getErrorType() + ":"
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
            Assert.fail(
                    "bulkinsert fail " + e.getErrorCode() + e.getMessage() );
        }
    }

    private long checkGroupData( String groupName, String matcher,
            long expCount ) {
        Sequoiadb dataNode = null;
        long count = 0;
        try {
            dataNode = sdb.getReplicaGroup( groupName ).getMaster().connect();
            DBCollection cl1 = dataNode.getCollectionSpace( SdbTestBase.csName )
                    .getCollection( clName );
            count = cl1.getCount( matcher );
            Assert.assertEquals( count, expCount,
                    "the count of " + groupName + " is:" + count );
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
            long expectRecNums = 2500;
            String destMatcher = "{no:{$gte:1000,$lt:2000}}";
            long destExpectNums = 1000;
            String srcMatcher = "{$or:[{no:{$gte:2000}},{no:{$lt:1000}}]}";
            long srcExpectNums = 1500;
            long destCount = checkGroupData( destGroupName, destMatcher,
                    destExpectNums );
            long srcCount = checkGroupData( srcGroupName, srcMatcher,
                    srcExpectNums );
            long actRecNums = destCount + srcCount;
            System.out.println( "total=" + actRecNums );
            Assert.assertEquals( actRecNums, expectRecNums,
                    "insert records num error: " + actRecNums );

            // check all records,check the value of "no"
            DBCursor tmpCursor = cl.query( null, null, "{ no: 1 }", null );
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

    class Insert extends OperateTask {
        @Override
        public void exec() throws Exception {
            Sequoiadb db1 = null;
            try {
                db1 = new Sequoiadb( connectUrl, "", "" );
                DBCollection cl1 = db1.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( clName );
                // insert 1000 records,the "no" value is 1000-2000
                bulkInsert( cl1, 1000, 2000 );
            } catch ( BaseException e ) {
                throw e;
            } finally {
                if ( db1 != null ) {
                    db1.close();
                }
            }
        }
    }

    class Split extends OperateTask {
        @Override
        public void exec() throws Exception {
            Sequoiadb db2 = null;
            try {
                db2 = new Sequoiadb( connectUrl, "", "" );
                db2.setSessionAttr(
                        ( BSONObject ) JSON.parse( "{PreferedInstance:'M'}" ) );
                DBCollection cl2 = db2.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( clName );
                cl2.split( srcGroupName, destGroupName,
                        ( BSONObject ) JSON.parse( "{no:1000}" ),
                        ( BSONObject ) JSON.parse( "{no:2000}" ) );
            } catch ( BaseException e ) {
                throw e;
            } finally {
                if ( db2 != null ) {
                    db2.close();
                }
            }
        }
    }

}
