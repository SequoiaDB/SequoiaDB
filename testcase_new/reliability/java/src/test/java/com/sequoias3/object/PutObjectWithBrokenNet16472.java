package com.sequoias3.object;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.*;
import com.sequoiadb.fault.BrokenNetwork;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;
import com.sequoias3.commlibs3.CommLibS3;
import com.sequoias3.commlibs3.S3TestBase;
import com.sequoias3.commlibs3.TestTools;
import com.sequoias3.commlibs3.s3utils.ObjectUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import java.util.Random;
import java.util.concurrent.CopyOnWriteArrayList;

/**
 * @Description seqDB-16472 ::开启版本控制，创建对象过程中SequoiaS3和sdb节点网络异常
 * @author fanyu
 * @Date 2019.01.17
 * @version 1.00
 */
public class PutObjectWithBrokenNet16472 extends S3TestBase {
    private boolean runSuccess = false;
    private AmazonS3 s3Client = null;
    private int fileSize = 1024 * new Random().nextInt( 1025 );
    private int objectNums = 10;
    private int versionNums = 2;
    private String filePath = null;
    private String bucketName = "bucket16472";
    private String objectNameBase = "PutObject16472";
    private List< String > objectNames = new ArrayList< String >();
    private List< String > objectNameList = new CopyOnWriteArrayList< String >();
    private File localPath = null;

    @BeforeClass
    private void setUp() throws IOException {
        localPath = new File( S3TestBase.workDir + File.separator
                + TestTools.getClassName() );
        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );
        filePath = localPath + File.separator + "localFile_" + fileSize
                + ".txt";
        TestTools.LocalFile.createFile( filePath, fileSize );
        s3Client = CommLibS3.buildS3Client();
        CommLibS3.clearBucket( s3Client, bucketName );
        s3Client.createBucket( bucketName );
        CommLibS3.setBucketVersioning( s3Client, bucketName,
                BucketVersioningConfiguration.ENABLED );
        for ( int i = 0; i < objectNums; i++ ) {
            objectNames.add( objectNameBase + "_" + i + "_"
                    + TestTools.getRandomString( 1 ) );
        }
    }

    @Test(enabled = false)
    public void test() throws Exception {
        // brokenNetwork
        FaultMakeTask faultTask = BrokenNetwork
                .getFaultMakeTask( S3TestBase.s3HostName, 10, 10 );
        TaskMgr mgr = new TaskMgr( faultTask );
        for ( int i = 0; i < objectNums; i++ ) {
            mgr.addTask( new PutObject( objectNames.get( i ) ) );
        }
        mgr.execute();
        Assert.assertTrue( mgr.isAllSuccess(), mgr.getErrorMsg() );
        // 检查故障前创建的对象
        for ( String objectName : objectNameList ) {
            String versionId = "0";
            S3Object s3Object = s3Client.getObject(
                    new GetObjectRequest( bucketName, objectName, versionId ) );
            chectGetResult( s3Object, objectName, versionId );
        }
        // put again
        // 故障后，再次创建对象
        objectNames.removeAll( objectNameList );
        for ( String objectName : objectNames ) {
            for ( int i = 0; i < versionNums; i++ ) {
                PutObjectResult obj = s3Client.putObject( bucketName,
                        objectName, new File( filePath ) );
            }
        }
        // 随机检查故障后，创建的对象
        if ( !objectNames.isEmpty() ) {
            int index = new Random().nextInt( objectNames.size() );
            String versionId = "1";
            S3Object s3Object = s3Client.getObject( new GetObjectRequest(
                    bucketName, objectNames.get( index ), versionId ) );
            chectGetResult( s3Object, objectNames.get( index ), versionId );
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

    public class PutObject extends OperateTask {
        private String objectName = null;

        public PutObject( String objectName ) {
            this.objectName = objectName;
        }

        @Override
        public void exec() throws Exception {
            try {
                for ( int i = 0; i < versionNums; i++ ) {
                    PutObjectResult obj = s3Client.putObject( bucketName,
                            this.objectName, new File( filePath ) );
                }
                objectNameList.add( this.objectName );
            } catch ( AmazonS3Exception e ) {
                if ( e.getStatusCode() != 500 ) {
                    throw e;
                }
            }
        }
    }

    private void chectGetResult( S3Object object, String objectName,
            String versionId ) throws Exception {
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
