package com.sequoiadb.metaopr.killnode;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.base.result.InsertResult;
import com.sequoiadb.commlib.*;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.fault.KillNode;
import com.sequoiadb.metaopr.Utils;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;
import java.util.Date;

/**
 * @Descreption seqDB-31136:创建集合，数据节点异常
 * @Author biqin
 * @CreateDate 2023/4/17
 * @UpdateUser biqin
 * @UpdateDate 2023/4/19
 * @UpdateRemark
 * @Version 1.0
 */
public class CreateCL31136 extends SdbTestBase {
    private Integer insertValue = 1;
    private Sequoiadb sdb = null;
    private CollectionSpace cs;
    private String groupName;
    private GroupMgr groupMgr;
    private String csName = "cs_31136";
    private String clNameBase = "cl_31136";
    private static final int clNUM = 500;

    @BeforeClass()
    private void setUp() throws ReliabilityException {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "---Skip testCase" );
        }
        groupMgr = GroupMgr.getInstance();
        if ( !groupMgr.checkBusiness() ) {
            throw new SkipException( "checkBusiness return false" );
        }
        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }
        cs = sdb.createCollectionSpace( csName );
        groupName = groupMgr.getAllDataGroup().get( 0 ).getGroupName();
    }

    @Test
    private void test() throws Exception {
        GroupWrapper dataGroup = groupMgr.getGroupByName( groupName );
        NodeWrapper priNode = dataGroup.getMaster();
        FaultMakeTask faultTask = KillNode.getFaultMakeTask( priNode.hostName(),
                priNode.svcName(), 0 );

        TaskMgr mgr = new TaskMgr( faultTask );
        CreateCl createCl = new CreateCl( clNameBase );
        mgr.addTask( createCl );
        mgr.execute();
        Assert.assertEquals( mgr.isAllSuccess(), true, mgr.getErrorMsg() );

        // check cluster
        Assert.assertEquals( groupMgr.checkBusinessWithLSN( 600 ), true,
                "failed to restore business" );

        createCLAgain();
        operateOnCL();

        Assert.assertEquals( groupMgr.checkBusinessWithLSN( 600 ), true,
                "failed to restore business" );
        checkCLResult();
    }

    @AfterClass
    public void tearDown() {
        try {
            sdb.dropCollectionSpace( csName );
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    private void createCLAgain() {
        for ( int i = 0; i < clNUM; i++ ) {
            String clName = clNameBase + "_" + i;
            if ( !( cs.isCollectionExist( clName ) ) ) {
                cs.createCollection( clName,
                        new BasicBSONObject( "Group", groupName ) );
            }
        }
    }

    private void operateOnCL() {
        for ( int i = 0; i < clNUM; i++ ) {
            String clName = clNameBase + "_" + i;
            DBCollection cl = cs.getCollection( clName );
            InsertResult insertResult = cl
                    .insertRecord( new BasicBSONObject( "a", 1 ) );
            Assert.assertEquals( insertResult.getInsertNum(), 1 );
        }
    }

    private void checkCLResult() {
        for ( int i = 0; i < clNUM; i++ ) {
            String clName = clNameBase + "_" + i;
            DBCollection cl = cs.getCollection( clName );
            DBCursor cursor = cl.query();
            int count = 0;
            while ( cursor.hasNext() ) {
                BSONObject result = cursor.getNext();
                Integer value = ( Integer ) result.get( "a" );
                Assert.assertEquals( value, insertValue );
                count++;
            }
            Assert.assertEquals( count, 1 );
            cursor.close();
        }
    }

    private class CreateCl extends OperateTask {
        private String clName;

        private CreateCl( String clName ) {
            this.clName = clName;
        }

        @Override
        public void exec() throws Exception {
            System.out.println(
                    new Date() + " " + this.getClass().getName().toString() );
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                for ( int i = 0; i < clNUM; i++ ) { // 0-499
                    String Name = clName + "_" + i;
                    cs.createCollection( Name,
                            new BasicBSONObject( "Group", groupName ) );
                }
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_COORD_REMOTE_DISC
                        .getErrorCode()
                        && e.getErrorCode() != SDBError.SDB_CLS_NOT_PRIMARY
                                .getErrorCode() ) {
                    throw e;
                }
            }
        }
    }
}
