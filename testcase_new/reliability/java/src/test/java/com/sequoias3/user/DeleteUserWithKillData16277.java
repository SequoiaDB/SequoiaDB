package com.sequoias3.user;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.CopyOnWriteArrayList;

import org.springframework.http.HttpStatus;
import org.springframework.web.client.HttpClientErrorException;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

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
 * @Description seqDB-16277:删除用户的过程中db的节点异常重启
 * @author fanyu
 * @version 1.00
 * @Date 2020.02.03
 */
public class DeleteUserWithKillData16277 extends S3TestBase {
    private String userNameBase = "user16277-";
    private int userNum = 50;
    private List< String > userNames = new ArrayList<>();
    private List< String > successUserNames = new CopyOnWriteArrayList<>();

    @BeforeClass
    private void setUp() throws Exception {
        for ( int i = 0; i < userNum; i++ ) {
            userNames.add( userNameBase + i );
            CommLibS3.clearUser( userNameBase + i );
            UserUtils.createUser( userNameBase+i, UserCommDefind.normal);
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
            DeleteUser dTask = new DeleteUser(
                    userNames.get( i ) );
            mgr.addTask( dTask );
        }
        mgr.execute();
        Assert.assertEquals( mgr.isAllSuccess(), true, mgr.getErrorMsg() );

        Assert.assertEquals( groupMgr.checkBusinessWithLSN( 600 ), true,
                "checkBusinessWithLSN() occurs timeout" );

        // check and delete user again then check
        for(String userName : successUserNames){
            try {
                UserUtils.getUser( userName );
                Assert.fail( "exp fail but act success,userName = " +
                        userName );
            }catch ( HttpClientErrorException e){
                if ( e.getStatusCode() != ( HttpStatus.NOT_FOUND ) ) {
                    throw e;
                }
            }
        }
        userNames.removeAll( successUserNames );
        for(String userNmae : userNames){
            UserUtils.deleteUser( userNmae );
        }
        for(String userName : userNames){
            try {
                UserUtils.getUser( userName );
                Assert.fail( "exp fail but act success,userName = " +
                        userName );
            }catch ( HttpClientErrorException e){
                if ( e.getStatusCode() != ( HttpStatus.NOT_FOUND ) ) {
                    throw e;
                }
            }
        }
    }

    @AfterClass
    private void tearDown() throws Exception {
    }

    private class DeleteUser extends OperateTask {
        private String userName;

        public DeleteUser( String userName ) {
            this.userName = userName;
        }

        @Override
        public void exec() throws Exception {
            try {
                UserUtils.deleteUser( userName );
                successUserNames.add( userName );
            } catch ( AmazonS3Exception e ) {
                if ( e.getStatusCode() != 500 ) {
                    throw e;
                }
            }
        }
    }
}