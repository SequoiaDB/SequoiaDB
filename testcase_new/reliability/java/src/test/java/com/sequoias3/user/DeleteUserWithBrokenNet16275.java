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
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.fault.BrokenNetwork;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;
import com.sequoias3.commlibs3.CommLibS3;
import com.sequoias3.commlibs3.S3TestBase;
import com.sequoias3.commlibs3.s3utils.UserUtils;
import com.sequoias3.commlibs3.s3utils.bean.UserCommDefind;

/**
 * @Description  seqDB-16275:删除用户的过程中db端网络异常
 * @author fanyu
 * @version 1.00
 * @Date 2020.02.03
 */
public class DeleteUserWithBrokenNet16275 extends S3TestBase {
    private String userNameBase = "user16275-";
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
        // delete region when network broken
        FaultMakeTask faultTask = BrokenNetwork
                .getFaultMakeTask( SdbTestBase.hostName, 1, 10 );
        TaskMgr mgr = new TaskMgr( faultTask );

        for ( int i = 0; i < userNum; i++ ) {
            DeleteUser dTask = new DeleteUser(
                    userNames.get( i ) );
            mgr.addTask( dTask );
        }
        mgr.execute();
        Assert.assertEquals( mgr.isAllSuccess(), true, mgr.getErrorMsg() );

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