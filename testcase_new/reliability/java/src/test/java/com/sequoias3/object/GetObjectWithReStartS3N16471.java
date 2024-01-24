package com.sequoias3.object;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import java.util.Random;
import java.util.concurrent.CopyOnWriteArrayList;

import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.amazonaws.SdkClientException;
import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.amazonaws.services.s3.model.S3Object;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;
import com.sequoias3.commlibs3.CommLibS3;
import com.sequoias3.commlibs3.S3TestBase;
import com.sequoias3.commlibs3.TestTools;
import com.sequoias3.commlibs3.s3utils.ObjectUtils;
import com.sequoias3.commlibs3.s3utils.S3NodeRestart;
import com.sequoias3.commlibs3.s3utils.bean.S3NodeWrapper;

/**
 * @Description seqDB-16471:获取对象过程中SequiaS3Client端异常
 * @author fanyu
 * @version 1.00
 * @Date 2019.01.17
 */
public class GetObjectWithReStartS3N16471 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket16471";
    private String objectNameBase = "aa/bb/object16471";
    private List< String > objectNames = new ArrayList< String >();
    private List< String > objectNameList = new CopyOnWriteArrayList< String
            >();
    private AmazonS3 s3Client = null;
    private int fileSize = 1024 * new Random().nextInt( 1025 );
    private int objectNums = 1024;
    private File localPath = null;
    private String filePath = null;

    @BeforeClass
    private void setUp() throws IOException {
        localPath = new File( S3TestBase.workDir + File.separator
                + TestTools.getClassName() );
        filePath = localPath + File.separator + "localFile_" + fileSize
                + ".txt";

        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );
        TestTools.LocalFile.createFile( filePath, fileSize );
        s3Client = CommLibS3.buildS3Client();
        CommLibS3.clearBucket( s3Client, bucketName );
        s3Client.createBucket( bucketName );
        for ( int i = 0; i < objectNums; i++ ) {
            String objectName = objectNameBase + "_" + i + "_"
                    + TestTools.getRandomString( 1 );
            objectNames.add( objectName );
            s3Client.putObject( bucketName, objectName, new File( filePath ) );
        }
    }

    @Test
    private void test() throws Exception {
        FaultMakeTask faultMakeTask = S3NodeRestart
                .getFaultMakeTask( new S3NodeWrapper(), 1, 10 );
        TaskMgr mgr = new TaskMgr( faultMakeTask );
        for ( int i = 0; i < objectNums; i++ ) {
            mgr.addTask( new GetObject( objectNames.get( i ) ) );
        }
        mgr.execute();
        mgr.isAllSuccess();
        s3Client = CommLibS3.buildS3Client();
        // 检查故障时获取成功的对象
        for ( String objectName : objectNameList ) {
            getObjectAndCheck( objectName );
        }
        objectNames.removeAll( objectNameList );
        // 检查故障时获取失败的对象
        for ( String objectName : objectNames ) {
            getObjectAndCheck( objectName );
        }
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
            s3Client.shutdown();
        }
    }

    private class GetObject extends OperateTask {
        private String key = null;

        public GetObject( String key ) {
            this.key = key;
        }

        @Override
        public void exec() throws Exception {
            try {
                s3Client.getObject( bucketName, key );
                objectNameList.add( key );
            } catch ( AmazonS3Exception e ) {
                if ( e.getStatusCode() != 500 ) {
                    throw e;
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
            }
        }
    }

    private void getObjectAndCheck( String objectName ) throws Exception {
        S3Object obj = s3Client.getObject( bucketName, objectName );
        Assert.assertEquals( obj.getKey(), objectName );
        String downloadPath = null;
        try {
            downloadPath = TestTools.LocalFile.initDownloadPath( localPath,
                    TestTools.getMethodName(), Thread.currentThread().getId() );
            ObjectUtils.inputStream2File( obj.getObjectContent(),
                    downloadPath );
            Assert.assertEquals( TestTools.getMD5( downloadPath ),
                    TestTools.getMD5( filePath ) );
        } catch ( Exception e ) {
            throw new Exception( objectName, e );
        }
    }
}
