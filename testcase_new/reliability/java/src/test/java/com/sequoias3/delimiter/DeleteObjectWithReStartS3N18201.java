package com.sequoias3.delimiter;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.CopyOnWriteArrayList;

import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.amazonaws.SdkClientException;
import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;
import com.sequoias3.commlibs3.CommLibS3;
import com.sequoias3.commlibs3.S3TestBase;
import com.sequoias3.commlibs3.TestTools;
import com.sequoias3.commlibs3.s3utils.DelimiterUtils;
import com.sequoias3.commlibs3.s3utils.S3NodeRestart;
import com.sequoias3.commlibs3.s3utils.UserUtils;
import com.sequoias3.commlibs3.s3utils.bean.S3NodeWrapper;

/**
 * test content: 删除对象过程中S3点异常 testlink-case: seqDB-18201
 *
 * @author wangkexin
 * @Date 2019.01.30
 * @version 1.00
 */

public class DeleteObjectWithReStartS3N18201 extends S3TestBase {
    private String bucketName = "bucket18201";
    private String userName = "user18201";
    private int objectNums = 100;
    private String keyName = "deleteObject18201";
    private String delimiter = "?";
    private String roleName = "normal";
    private List< String > keyNames = new ArrayList< String >();
    private List< String > keyNameList = new CopyOnWriteArrayList< String >();
    private String[] accessKeys = null;
    private AmazonS3 s3Client = null;
    private boolean runSuccess = false;

    @BeforeClass
    private void setUp() {
        CommLibS3.clearUser( userName );
        accessKeys = UserUtils.createUser( userName, roleName );
        s3Client = CommLibS3.buildS3Client( accessKeys[ 0 ], accessKeys[ 1 ] );
        CommLibS3.clearBucket( s3Client, bucketName );
        s3Client.createBucket( bucketName );
        DelimiterUtils.putBucketDelimiter( bucketName, delimiter,
                accessKeys[ 0 ] );
        for ( int i = 0; i < objectNums; i++ ) {
            keyNames.add( keyName + "_" + i + delimiter
                    + TestTools.getRandomString( 3 ) );
        }
    }

    @Test
    public void testCreateRegion() throws Exception {
        FaultMakeTask faultMakeTask = S3NodeRestart
                .getFaultMakeTask( new S3NodeWrapper(), 1, 10 );
        TaskMgr mgr = new TaskMgr( faultMakeTask );
        for ( int i = 0; i < objectNums; i++ ) {
            mgr.addTask( new DeleteObject( keyNames.get( i ) ) );
        }
        mgr.execute();
        Assert.assertTrue( mgr.isAllSuccess(), mgr.getErrorMsg() );
        // delete again
        keyNames.removeAll( keyNameList );
        for ( String keyName : keyNames ) {
            s3Client.deleteObject( bucketName, keyName );
            keyNameList.add( keyName );
        }
        Assert.assertEquals( keyNameList.size(), objectNums,
                "keyNameList = " + keyNameList.toString() );
        for ( String objectName : keyNameList ) {
            Assert.assertFalse(
                    s3Client.doesObjectExist( bucketName, objectName ),
                    "onject : " + objectName + " is still exist" );
        }
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() throws Exception {
        try {
            if ( runSuccess ) {
                UserUtils.deleteUser( userName );
            }
        } finally {
            if ( s3Client != null ) {
                s3Client.shutdown();
            }
        }
    }

    private class DeleteObject extends OperateTask {
        private String keyName = null;

        public DeleteObject( String keytName ) {
            this.keyName = keytName;
        }

        @Override
        public void exec() throws Exception {
            AmazonS3 s3Client = CommLibS3.buildS3Client( accessKeys[ 0 ],
                    accessKeys[ 1 ] );
            try {
                s3Client.deleteObject( bucketName, keyName );
                keyNameList.add( this.keyName );
            } catch ( AmazonS3Exception e ) {
                if ( e.getStatusCode() != 500 ) {
                    throw new Exception( bucketName + ":" + keyName, e );
                }
            } catch ( SdkClientException e ) {
                if ( !e.getMessage()
                        .contains( "Unable to execute HTTP request" ) ) {
                    throw e;
                }
            } catch ( Exception e ) {
                if ( !e.getMessage().contains( "I/O error on POST request" ) ) {
                    throw e;
                }
            } finally {
                if ( s3Client != null ) {
                    s3Client.shutdown();
                }
            }
        }
    }
}
