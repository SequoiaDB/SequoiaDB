package com.sequoias3.privilege;

import java.io.File;
import java.io.IOException;

import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.amazonaws.SdkClientException;
import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.amazonaws.services.s3.model.EmailAddressGrantee;
import com.amazonaws.services.s3.model.Grant;
import com.amazonaws.services.s3.model.Permission;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;
import com.sequoias3.commlibs3.CommLibS3;
import com.sequoias3.commlibs3.S3TestBase;
import com.sequoias3.commlibs3.TestTools;
import com.sequoias3.commlibs3.s3utils.PrivilegeUtils;
import com.sequoias3.commlibs3.s3utils.S3NodeRestart;
import com.sequoias3.commlibs3.s3utils.bean.S3NodeWrapper;

/**
 * @description seqDB-19485 :获取对象acl过程中s3节点异常
 * @author wangkexin
 * @date 2019.09.26
 * @updateUser wuyan
 * @updateDate 2021.11.1
 * @updateRemark 减少操作对象数据量，在满足测试场景基础上减少大数据量同步等待时间
 * @version 1.00
 */
public class SetObjectAclAndS3ReStart19485 extends S3TestBase {
    private boolean runSuccess = false;
    private AmazonS3 s3Client = null;
    private String bucketName = "bucket19485";
    private String keyName = "key19485";
    private int fileSize = 200 * 1024;
    private String filePath = null;
    private File localPath = null;
    private File file = null;
    private int threadNum = 30;
    private Grant expGrants;

    @BeforeClass
    private void setUp() throws IOException {
        localPath = new File( SdbTestBase.workDir + File.separator
                + TestTools.getClassName() );
        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );
        filePath = localPath + File.separator + "localFile_" + fileSize
                + ".txt";
        TestTools.LocalFile.createFile( filePath, fileSize );
        file = new File( filePath );

        s3Client = CommLibS3.buildS3Client();
        CommLibS3.clearBucket( s3Client, bucketName );
        s3Client.createBucket( bucketName );
        s3Client.putObject( bucketName, keyName, file );

        // set object acl
        expGrants = new Grant(
                new EmailAddressGrantee( "test email adress 19485" ),
                Permission.Write );
        PrivilegeUtils.setObjectAclByBody( s3Client, bucketName, keyName,
                expGrants );
    }

    @Test
    public void test() throws Exception {
        FaultMakeTask faultMakeTask = S3NodeRestart
                .getFaultMakeTask( new S3NodeWrapper(), 0, 8 );
        TaskMgr mgr = new TaskMgr( faultMakeTask );

        for ( int i = 0; i < threadNum; i++ ) {
            mgr.addTask( new GetObjectAcl( expGrants ) );
        }

        mgr.execute();
        Assert.assertTrue( mgr.isAllSuccess(), mgr.getErrorMsg() );

        // get obeject acl again
        PrivilegeUtils.checkSetObjectAclResult( s3Client, bucketName, keyName,
                expGrants );
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( runSuccess ) {
                CommLibS3.clearBucket( s3Client, bucketName );
                TestTools.LocalFile.removeFile( localPath );
            }
        } finally {
            if ( s3Client != null )
                s3Client.shutdown();

        }
    }

    public class GetObjectAcl extends OperateTask {
        private Grant[] grants;
        private AmazonS3 s3 = CommLibS3.buildS3Client();

        private GetObjectAcl( Grant... grants ) {
            this.grants = grants;
        }

        @Override
        public void exec() throws Exception {
            try {
                PrivilegeUtils.checkSetObjectAclResult( s3, bucketName, keyName,
                        grants );
            } catch ( AmazonS3Exception e ) {
                if ( e.getStatusCode() != 500 ) {
                    throw new Exception( "keyName : " + keyName, e );
                }
            } catch ( SdkClientException e ) {
                if ( !e.getMessage()
                        .contains( "Unable to execute HTTP request" ) ) {
                    throw new Exception( "keyName : " + keyName, e );
                }
            } catch ( Exception e ) {
                if ( !e.getMessage().contains( "I/O error on POST request" ) ) {
                    throw e;
                }
            } finally {
                if ( s3 != null ) {
                    s3.shutdown();
                }
            }
        }
    }
}
