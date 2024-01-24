package com.sequoias3.user;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.CopyOnWriteArrayList;

import org.json.JSONObject;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.GroupWrapper;
import com.sequoiadb.commlib.NodeWrapper;
import com.sequoiadb.fault.KillNode;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;
import com.sequoias3.commlibs3.CommLibS3;
import com.sequoias3.commlibs3.S3TestBase;
import com.sequoias3.commlibs3.s3utils.UserUtils;
import com.sequoias3.commlibs3.s3utils.bean.UserCommDefind;

/**
 * @Description seqDB-16274:创建用户过程中db的节点异常重启
 * @author fanyu
 * @version 1.00
 * @Date 2020.02.03
 */
public class CreateUserWithKillData16274 extends S3TestBase {
    private String userNameBase = "user16274-";
    private int userNum = 50;
    private List< String > userNames = new ArrayList<>();
    private List< String > successUserNames = new CopyOnWriteArrayList<>();

    @BeforeClass
    private void setUp() throws Exception {
        for ( int i = 0; i < userNum; i++ ) {
            CommLibS3.clearUser( userNameBase + i );
            userNames.add( userNameBase + i );
        }
    }

    @Test
    public void test() throws Exception {
        TaskMgr mgr = new TaskMgr();
        GroupMgr groupMgr = GroupMgr.getInstance();
        List< GroupWrapper > dataGroups = groupMgr.getAllDataGroup();

        for ( int i = 0; i < dataGroups.size(); i++ ) {
            String groupName = dataGroups.get( i ).getGroupName();
            GroupWrapper group = groupMgr.getGroupByName( groupName );
            NodeWrapper node = group.getMaster();
            FaultMakeTask faultTask = KillNode.getFaultMakeTask( node, 1 );
            mgr.addTask( faultTask );
        }

        for ( int i = 0; i < userNum; i++ ) {
            CreateUser cTask = new CreateUser(
                    userNames.get( i ) );
            mgr.addTask( cTask );
        }

        mgr.execute();
        Assert.assertEquals( mgr.isAllSuccess(), true, mgr.getErrorMsg() );

        // check whether the cluster is normal and lsn consistency ,the longest
        // waiting time is 600S
        Assert.assertEquals( groupMgr.checkBusinessWithLSN( 600 ), true,
                "checkBusinessWithLSN() occurs timeout" );

        // check and create user again then check
        checkResult( successUserNames );
        userNames.removeAll( successUserNames );
        for ( String userName : userNames ) {
            UserUtils.createUser( userName, UserCommDefind.normal );
            successUserNames.add( userName );
        }
        checkResult( userNames );
    }

    @AfterClass
    private void tearDown() throws Exception {
        for ( String userName : successUserNames ) {
            UserUtils.deleteUser( userName );
        }
    }

    private class CreateUser extends OperateTask {
        private String userName;

        public CreateUser( String userName ) {
            this.userName = userName;
        }

        @Override
        public void exec() throws Exception {
            try {
                UserUtils.createUser( userName, UserCommDefind.normal );
                successUserNames.add( userName );
            } catch ( AmazonS3Exception e ) {
                if ( e.getStatusCode() != 500 ) {
                    throw e;
                }
            }
        }
    }

    private void checkResult( List< String > userNames ) {
        for ( String userName : userNames ) {
            JSONObject json = UserUtils.getUser( userName );
            JSONObject adminJSON = json
                    .getJSONObject( UserCommDefind.accessKeys );
            String accessKeyID = adminJSON.getString( UserCommDefind
                    .accessKeyID );
            String secretAccessKey = adminJSON.getString( UserCommDefind
                    .secretAccessKey );
            AmazonS3 s3 = null;
            try {
                s3 = CommLibS3.buildS3Client( accessKeyID,
                        secretAccessKey );
                Assert.assertFalse( s3.doesBucketExistV2( userName ) );
            } finally {
                if ( s3 != null ) {
                    s3.shutdown();
                }
            }
        }
    }
}