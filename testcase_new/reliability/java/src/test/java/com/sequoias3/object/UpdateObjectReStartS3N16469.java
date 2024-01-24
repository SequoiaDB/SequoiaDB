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
import com.amazonaws.services.s3.model.BucketVersioningConfiguration;
import com.amazonaws.services.s3.model.GetObjectRequest;
import com.amazonaws.services.s3.model.ObjectMetadata;
import com.amazonaws.services.s3.model.S3Object;
import com.amazonaws.services.s3.model.S3ObjectInputStream;
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
 * @Description seqDB-16469:更新对象过程中SequiaS3Client端异常
 * @author fanyu
 * @version 1.00
 * @Date 2019.01.21
 */
public class UpdateObjectReStartS3N16469 extends S3TestBase {

    private boolean runSuccess = false;
    private AmazonS3 s3Client = null;
    private int fileSize = 1024 * new Random().nextInt( 1025 );
    private int objectNums = 10;
    private String filePath = null;
    private String updatePath = null;
    private String bucketName = "bucket16469";
    private String objectNameBase = "object16469";
    private List< String > objectNames = new ArrayList< String >();
    private List< String > objectNameList = new CopyOnWriteArrayList< String
            >();
    private File localPath = null;

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
        CommLibS3.setBucketVersioning( s3Client, bucketName,
                BucketVersioningConfiguration.ENABLED );
        for ( int i = 0; i < objectNums; i++ ) {
            objectNames.add( objectNameBase + "_" + i + "_"
                    + TestTools.getRandomString( 1 ) );
            s3Client.putObject( bucketName, objectNames.get( i ),
                    new File( filePath ) );
        }
    }

    @Test
    public void test() throws Exception {
        FaultMakeTask faultMakeTask = S3NodeRestart
                .getFaultMakeTask( new S3NodeWrapper(), 1, 10 );
        TaskMgr mgr = new TaskMgr( faultMakeTask );
        for ( int i = 0; i < objectNums; i++ ) {
            mgr.addTask( new PutObject( objectNames.get( i ) ) );
        }
        mgr.execute();
        mgr.isAllSuccess();
        s3Client = CommLibS3.buildS3Client();
        // 检查故障前创建的对象
        for ( String objectName : objectNameList ) {
            String versionId = "1";
            S3Object s3Object = s3Client.getObject(
                    new GetObjectRequest( bucketName, objectName ) );
            chectGetResult( s3Object, objectName, versionId, updatePath );
        }

        // 故障恢复后，重新创建对象
        objectNames.removeAll( objectNameList );
        for ( String objectName : objectNames ) {
            s3Client.putObject( bucketName, objectName,
                    new File( updatePath ) );
        }
        // 随机检查故障恢复后的创建的对象
        if ( !objectNames.isEmpty() ) {
            int index = new Random().nextInt( objectNames.size() );
            String versionId = "1";
            S3Object s3Object = s3Client.getObject( new GetObjectRequest(
                    bucketName, objectNames.get( index ) ) );
            chectGetResult( s3Object, objectNames.get( index ), versionId,
                    updatePath );

            String versionId1 = "0";
            S3Object s3Object1 = s3Client.getObject( new GetObjectRequest(
                    bucketName, objectNames.get( index ), versionId1 ) );
            chectGetResult( s3Object1, objectNames.get( index ), versionId1,
                    filePath );
        }
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

        private String objectName = null;

        public PutObject( String objectName ) {
            this.objectName = objectName;
        }

        @Override
        public void exec() throws Exception {
            try {
                s3Client.putObject( bucketName, this.objectName,
                        new File( updatePath ) );
                objectNameList.add( this.objectName );
            } catch ( AmazonS3Exception e ) {
                if ( e.getStatusCode() != 500 ) {
                    throw new Exception( "bucketName = " + bucketName
                            + ",objectName = " + objectName, e );
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

    private void chectGetResult( S3Object object, String objectName,
            String versionId, String filePath ) throws Exception {
        Assert.assertEquals( object.getKey(), objectName );
        Assert.assertEquals( object.getBucketName(), bucketName );
        ObjectMetadata objectMetadata = object.getObjectMetadata();
        Assert.assertEquals( objectMetadata.getETag(),
                TestTools.getMD5( filePath ) );
        Assert.assertEquals( objectMetadata.getVersionId(), versionId );
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
