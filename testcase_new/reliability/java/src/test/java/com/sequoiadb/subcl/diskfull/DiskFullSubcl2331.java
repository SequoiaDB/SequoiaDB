package com.sequoiadb.subcl.diskfull;

import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

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
import com.sequoiadb.fault.DiskFull;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;

/**
 * @FileName:SEQDB-2331 在主表做基本操作时dataRG主节点所在服务器磁盘满
 * @author huangqiaohui
 * @version 1.00
 *
 */

public class DiskFullSubcl2331 extends SdbTestBase {
    private String mainClName = "testcaseCL2331_main";
    private String subClName = "testcaseCL2331_sub";
    private String csName = "testcaseCL2331_CS";
    private String subClGroupName;
    private CollectionSpace commCS;
    private DBCollection mainCL;
    private GroupMgr groupMgr = null;
    private Sequoiadb commSdb;
    private boolean clearFlag = false;
    private DBCollection subCL;
    public int insertCount = 0;

    @BeforeClass()
    public void setUp() {
        try {

            groupMgr = GroupMgr.getInstance();

            // CheckBusiness(true),检测当前集群环境，若存在异常返回false，
            if ( !groupMgr.checkBusiness( 20 ) ) {
                throw new SkipException( "checkBusiness return false" );
            }
            subClGroupName = groupMgr.getAllDataGroup().get( 0 ).getGroupName();

            commSdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            commSdb.setSessionAttr(
                    ( BSONObject ) JSON.parse( "{PreferedInstance:'M'}" ) );
            commCS = commSdb.createCollectionSpace( csName );
            mainCL = commCS.createCollection( mainClName, ( BSONObject ) JSON
                    .parse( "{ShardingKey:{'sk':1},ShardingType:'range',IsMainCL:true}" ) );
            subCL = commCS.createCollection( subClName,
                    ( BSONObject ) JSON.parse(
                            "{ShardingKey:{sk:1},ShardingType:'range',Group:'"
                                    + subClGroupName + "'}" ) );
            mainCL.attachCollection( subCL.getFullName(), ( BSONObject ) JSON
                    .parse( "{LowBound:{sk:0},UpBound:{sk:10000}}" ) );

        } catch ( ReliabilityException e ) {
            if ( commSdb != null ) {
                commSdb.close();
            }
            Assert.fail( this.getClass().getName()
                    + " setUp error, error description:" + e.getMessage()
                    + "\r\n" + Utils.getStackString( e ) );
        }
    }

    @Test
    public void test() {
        try {
            GroupWrapper subGroup = groupMgr.getGroupByName( subClGroupName );
            NodeWrapper subCLGroupMaster = subGroup.getMaster();
            System.out.println( "fillUpDisk:" + subCLGroupMaster.hostName()
                    + "subCLGroupName" + subClGroupName );

            FaultMakeTask faultMakeTask = DiskFull.getFaultMakeTask(
                    subCLGroupMaster.hostName(), SdbTestBase.reservedDir, 1,
                    10 );
            TaskMgr taskMgr = new TaskMgr( faultMakeTask );
            taskMgr.addTask( new Insert() );
            taskMgr.execute();
            Assert.assertEquals( taskMgr.isAllSuccess(), true,
                    taskMgr.getErrorMsg() );

            checkAndInsert();

            Assert.assertEquals( subGroup.checkInspect( 120 ), true );
            clearFlag = true;
        } catch ( ReliabilityException e ) {
            e.printStackTrace();
            Assert.fail( e.getMessage() );
        }

    }

    private void checkAndInsert() {
        String padStr = Utils.getString( 1024 * 1024 );
        for ( int i = insertCount; i < insertCount + 2; i++ ) {
            mainCL.insert( "{sk:" + i + ",pad:'" + padStr + "'}" );
        }
        insertCount += 2;

        DBCursor cursor = null;
        cursor = mainCL.query( null, "{sk:1,pad:1}", "{sk:1}", null );
        int count = 0;
        while ( cursor.hasNext() ) {
            BSONObject obj = cursor.getNext();
            Assert.assertEquals( obj,
                    JSON.parse( "{sk:" + count + ",pad:'" + padStr + "'}" ) );
            count++;
        }

        Assert.assertEquals( count, insertCount );
    }

    @AfterClass
    public void tearDown() throws InterruptedException {
        try {
            if ( clearFlag ) {
                for ( int i = 0; i < 30; i++ ) {
                    try {
                        commSdb.dropCollectionSpace( csName );
                        break;
                    } catch ( BaseException e ) {
                        if ( e.getErrorCode() == -147 && i < 29 ) {
                            Thread.sleep( 1000 );
                        } else {
                            throw e;
                        }
                    }
                }
            }
        } finally {
            if ( commSdb != null ) {
                commSdb.close();
            }

        }
    }

    class Insert extends OperateTask {
        @Override
        public void exec() throws Exception {
            String padStr = Utils.getString( 1024 * 1024 );

            try {
                for ( int i = 0; i < 128; i++ ) {
                    mainCL.insert( "{sk:" + i + ",pad:'" + padStr + "'}" );
                    insertCount++;
                }
            } catch ( BaseException e ) {
                System.out.println( "InsertException:" + e.getErrorCode()
                        + " InserCount:" + insertCount );
            }
        }
    }
}
