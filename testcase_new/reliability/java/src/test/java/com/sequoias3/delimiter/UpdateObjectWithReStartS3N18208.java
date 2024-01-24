package com.sequoias3.delimiter;

import com.amazonaws.SdkClientException;
import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.amazonaws.services.s3.model.BucketVersioningConfiguration;
import com.amazonaws.services.s3.model.GetObjectRequest;
import com.amazonaws.services.s3.model.ListVersionsRequest;
import com.amazonaws.services.s3.model.S3Object;
import com.amazonaws.services.s3.model.S3ObjectInputStream;
import com.amazonaws.services.s3.model.VersionListing;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;
import com.sequoias3.commlibs3.CommLibS3;
import com.sequoias3.commlibs3.S3TestBase;
import com.sequoias3.commlibs3.TestTools;
import com.sequoias3.commlibs3.s3utils.DelimiterUtils;
import com.sequoias3.commlibs3.s3utils.ObjectUtils;
import com.sequoias3.commlibs3.s3utils.S3NodeRestart;
import com.sequoias3.commlibs3.s3utils.bean.S3NodeWrapper;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import java.util.Random;
import java.util.concurrent.atomic.AtomicInteger;

/**
 * @Description seqDB-18208 :: 更新对象过程中s3节点异常
 * @author fanyu
 * @Date 2019.05.24
 * @version 1.00
 */
public class UpdateObjectWithReStartS3N18208 extends S3TestBase {
    private boolean runSuccess = false;
    private AmazonS3 s3Client = null;
    private int fileSize = 1024 * new Random().nextInt( 1025 );
    private int versionNums = 100;
    private String bucketName = "bucket18208";
    private String objectName = "Put18208*Object*18208";
    private String filePath = null;
    private String updatePath = null;
    private AtomicInteger count = new AtomicInteger( 0 );
    private File localPath = null;
    private String delimiter = "*";

    @BeforeClass
    private void setUp() throws IOException {
        localPath = new File( S3TestBase.workDir + File.separator
                + TestTools.getClassName() );
        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );
        filePath = localPath + File.separator + "localFile_" + fileSize
                + ".txt";
        updatePath = localPath + File.separator + "localFile_"
                + ( fileSize + 1024 * 200 ) + ".txt";
        TestTools.LocalFile.createFile( filePath, fileSize );
        TestTools.LocalFile.createFile( updatePath, fileSize + 1024 * 200 );
        s3Client = CommLibS3.buildS3Client();
        CommLibS3.clearBucket( s3Client, bucketName );
        s3Client.createBucket( bucketName );
        DelimiterUtils.putBucketDelimiter( bucketName, delimiter );
        CommLibS3.setBucketVersioning( s3Client, bucketName,
                BucketVersioningConfiguration.ENABLED );
        s3Client.putObject( bucketName, objectName, new File( filePath ) );
    }

    @Test
    public void test() throws Exception {
        FaultMakeTask faultMakeTask = S3NodeRestart
                .getFaultMakeTask( new S3NodeWrapper(), 1, 10 );
        TaskMgr mgr = new TaskMgr( faultMakeTask );
        for ( int i = 0; i < versionNums; i++ ) {
            mgr.addTask( new PutObject( updatePath ) );
        }
        mgr.execute();
        Assert.assertTrue( mgr.isAllSuccess(), mgr.getErrorMsg() );
        s3Client = CommLibS3.buildS3Client();
        // put again
        for ( int i = count.get(); i < versionNums; i++ ) {
            s3Client.putObject( bucketName, objectName,
                    new File( updatePath ) );
        }
        // check result;
        checkResult();
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() throws Exception {
        try {
            if ( runSuccess ) {
                CommLibS3.clearBucket( s3Client, bucketName );
                TestTools.LocalFile.removeFile( localPath );
            }
        } finally {
            s3Client.shutdown();
        }
    }

    public class PutObject extends OperateTask {
        private String filePath = null;

        public PutObject( String filePath ) {
            this.filePath = filePath;
        }

        @Override
        public void exec() throws Exception {
            try {
                s3Client.putObject( bucketName, objectName,
                        new File( filePath ) );
                count.incrementAndGet();
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

    private void checkResult() throws Exception {
        VersionListing vsList = s3Client.listVersions( new ListVersionsRequest()
                .withBucketName( bucketName ).withDelimiter( delimiter ) );
        List< String > expCommonPrefixes = new ArrayList<>();
        expCommonPrefixes.add( "Put18208*" );
        Assert.assertEquals( vsList.getCommonPrefixes(), expCommonPrefixes,
                "act = " + vsList.getCommonPrefixes() + ",exp = "
                        + expCommonPrefixes );
        Assert.assertEquals( vsList.getVersionSummaries().size(), 0 );

        String currVersionId = String
                .valueOf( new Random().nextInt( versionNums ) + 1 );
        S3Object currObject = s3Client.getObject(
                new GetObjectRequest( bucketName, objectName, currVersionId ) );
        chectGetResult( currObject, updatePath );

        String histVersionId = String.valueOf( 0 );
        S3Object histObject = s3Client.getObject(
                new GetObjectRequest( bucketName, objectName, histVersionId ) );
        chectGetResult( histObject, filePath );
    }

    private void chectGetResult( S3Object object, String filePath )
            throws Exception {
        Assert.assertEquals( object.getObjectMetadata().getETag(),
                TestTools.getMD5( filePath ) );
        S3ObjectInputStream s3InputStream = null;
        try {
            s3InputStream = object.getObjectContent();
            String downloadPath = TestTools.LocalFile.initDownloadPath(
                    localPath, TestTools.getMethodName(),
                    Thread.currentThread().getId() );
            ObjectUtils.inputStream2File( s3InputStream, downloadPath );
            Assert.assertEquals( TestTools.getMD5( downloadPath ),
                    TestTools.getMD5( filePath ) );
        } finally {
            if ( s3InputStream != null ) {
                s3InputStream.close();
            }
        }
    }
}
