package com.sequoiadb.metaopr.killnode;

import java.util.ArrayList;
import java.util.List;
import java.util.Random;

import com.sequoiadb.metaopr.Utils;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
import org.bson.types.ObjectId;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.GroupWrapper;
import com.sequoiadb.commlib.NodeWrapper;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.fault.KillNode;
import com.sequoiadb.metaopr.commons.MyUtil;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.TaskMgr;
import com.sequoiadb.datasync.CreateCLTask;

/**
 * FileName: CreateCLAndKillPrimaryCatalog2280.java test content:when create
 * cl(designated domain), kill -9 the catalog group master node testlink
 * case:seqDB-2280
 * 
 * @author wuyan
 * @Date 2017.4.24
 * @version 1.00
 */

public class CreateCLAndKillPrimaryCatalog2280 extends SdbTestBase {
    private GroupMgr groupMgr = null;
    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private boolean clearFlag = false;
    private String preCLName = "cl_2280";
    private String domainName;
    private String csName = "cs_2280";
    private final int CL_NUM = 1000;
    private int count = 0;
    private List< String > groupNames = null;

    @BeforeClass
    public void setUp() {
        try {

            groupMgr = GroupMgr.getInstance();
            if ( !groupMgr.checkBusiness() ) {
                throw new SkipException( "checkBusiness failed" );
            }

            sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            domainName = "domain_2280";
            createDomain( domainName );
            createCS();
        } catch ( ReliabilityException e ) {
            Assert.fail( this.getClass().getName()
                    + " setUp error, error description:" + e.getMessage()
                    + "\r\n" + Utils.getKeyStack( e, this ) );
        }
    }

    @Test
    public void test() {
        try {
            GroupWrapper cataGroup = groupMgr
                    .getGroupByName( "SYSCatalogGroup" );
            NodeWrapper priNode = cataGroup.getMaster();

            FaultMakeTask faultTask = KillNode.getFaultMakeTask(
                    priNode.hostName(), priNode.svcName(), 5 );
            TaskMgr mgr = new TaskMgr( faultTask );
            CreateCLTask cTask = new CreateCLTask( preCLName, CL_NUM, csName );
            cTask.setOption( ( BSONObject ) JSON.parse(
                    "{ShardingKey:{no:1},ShardingType:'hash',Partition:4096}" ) );
            mgr.addTask( cTask );
            mgr.execute();

            // TaskMgr check if there is any exception
            Assert.assertEquals( mgr.isAllSuccess(), true, mgr.getErrorMsg() );

            Assert.assertEquals( groupMgr.checkBusinessWithLSN( 600 ), true,
                    "check LSN consistency fail" );

            // check result
            count = cTask.getCreateNum();
            checkCreateCLResult();
            Utils.checkConsistency( groupMgr );

            // Normal operating environment
            clearFlag = true;
        } catch ( ReliabilityException | InterruptedException e ) {
            e.printStackTrace();
            Assert.fail( e.getMessage() );
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( clearFlag ) {
                sdb.dropCollectionSpace( csName );
                sdb.dropDomain( domainName );
            }
        } catch ( BaseException e ) {
            Assert.fail(
                    e.getMessage() + "\r\n" + Utils.getKeyStack( e, this ) );
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    private void createDomain( String doaminName ) {
        try {
            groupNames = groupMgr.getAllDataGroupName();
            BSONObject option = new BasicBSONObject();
            BSONObject groups = new BasicBSONList();
            for ( int i = 0; i < groupNames.size(); i++ ) {
                groups.put( "" + i, groupNames.get( i ) );
            }
            option.put( "Groups", groups );
            option.put( "AutoSplit", true );
            sdb.createDomain( domainName, option );
        } catch ( BaseException e ) {
            Assert.fail( "create domain failed, errMsg:" + e.getMessage() );
        }
    }

    private void createCS() {
        try {
            BSONObject option = new BasicBSONObject();
            option.put( "Domain", domainName );
            cs = sdb.createCollectionSpace( csName, option );
        } catch ( BaseException e ) {
            Assert.fail( "create csName failed, errMsg:" + e.getMessage() );
        }
    }

    /**
     * check the result of create cl the result: 1. to create cl success,create
     * the same cl again failed 2. to create cl fail,create the same cl again
     * successfully 3. to create cl fail,the actual cl created successfully did
     * not return, creating the same cl failed again
     */
    private void checkCreateCLResult() {
        if ( CL_NUM == count ) {
            try {
                String sameCLName = preCLName + "_" + ( count - 1 );
                cs.createCollection( sameCLName );
                Assert.fail( "create the same cl should be fail" );
            } catch ( BaseException e ) {
                // -22 SDB_DMS_EXIST
                if ( e.getErrorCode() != -22 ) {
                    Assert.fail( "the error not -22: " + e.getErrorCode()
                            + e.getMessage() );
                }
            }

            try {
                String newclName = preCLName + "_" + count;
                cs.createCollection( newclName,
                        new BasicBSONObject( "ReplSize", 0 ) );
            } catch ( BaseException e ) {
                Assert.fail( "create new cl fail: " + e.getErrorCode()
                        + e.getMessage() );

            }

        } else {
            // create cl fail,the count is not equals CL_NUM
            try {
                cs.createCollection( preCLName + "_" + count,
                        new BasicBSONObject( "ReplSize", 0 ) );
            } catch ( BaseException e ) {
                // -22 SDB_DMS_EXIST
                if ( e.getErrorCode() != -22 ) {
                    Assert.fail( "the error not -22: " + e.getErrorCode()
                            + e.getErrorType() );
                }
            }
        }
        MyUtil.checkListCL( sdb, csName, preCLName, count + 1 );
        insertByCL();
    }

    /**
     * randomly select a cl insert data,check the data split groups by domain
     */
    private void insertByCL() {
        String clName = "";
        if ( count != 0 ) {
            Random random = new Random();
            clName = preCLName + "_" + random.nextInt( count );
        } else {
            clName = preCLName + "_" + count;
        }

        long insertNum = 1000;
        try {
            List< BSONObject > list = new ArrayList< BSONObject >();
            for ( long i = 0; i < insertNum; i++ ) {
                BSONObject obj = new BasicBSONObject();
                ObjectId id = new ObjectId();
                obj.put( "_id", id );
                obj.put( "no", i );
                obj.put( "str", "test_" + String.valueOf( i ) );
                list.add( obj );
            }
            DBCollection cl = sdb.getCollectionSpace( csName )
                    .getCollection( clName );
            cl.insert( list, DBCollection.FLG_INSERT_CONTONDUP );
        } catch ( BaseException e ) {
            Assert.assertTrue( false,
                    "insert fail " + e.getErrorCode() + e.getMessage() );
        }

        // check data split the groups of domains
        long dataCount = 0;
        long recsNum = 0;
        for ( int i = 0; i < groupNames.size(); i++ ) {
            String groupName = groupNames.get( i );
            try ( Sequoiadb dataNode = sdb.getReplicaGroup( groupName )
                    .getMaster().connect()) {
                DBCollection cl = dataNode.getCollectionSpace( csName )
                        .getCollection( clName );
                dataCount = cl.getCount();
                recsNum += dataCount;
                if ( dataCount == 0 ) {
                    Assert.fail( "no data on group:" + groupName );
                }
            } catch ( BaseException e ) {
                Assert.fail( e.getMessage() + e.getErrorCode() );
            }
        }
        Assert.assertEquals( recsNum, insertNum,
                "incorrect number of the cl,actnum=" + recsNum );
    }

}
