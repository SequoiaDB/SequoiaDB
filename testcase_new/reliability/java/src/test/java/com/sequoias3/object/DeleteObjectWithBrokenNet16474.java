package com.sequoias3.object;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.amazonaws.services.s3.model.BucketVersioningConfiguration;
import com.sequoiadb.fault.BrokenNetwork;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;
import com.sequoias3.commlibs3.CommLibS3;
import com.sequoias3.commlibs3.S3TestBase;
import com.sequoias3.commlibs3.TestTools;
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
public class DeleteObjectWithBrokenNet16474 extends S3TestBase {
    private boolean runSuccess = false;
    private AmazonS3 s3Client = null;
    private int fileSize = 1024 * new Random().nextInt( 1025 );
    private int objectNums = 10;
    private int versionNums = 3;
    private String filePath = null;
    private String updatePath = null;
    private String bucketName = "bucket16474";
    private String objectNameBase = "object16474";
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
            for ( int j = 0; j < versionNums; j++ ) {
                s3Client.putObject( bucketName, objectNames.get( i ),
                        new File( filePath ) );
            }
        }
    }

    @Test(enabled = false)
    public void test() throws Exception {
        // kill coord when Delete objects
        FaultMakeTask faultTask = BrokenNetwork
                .getFaultMakeTask( S3TestBase.s3HostName, 10, 20 );
        TaskMgr mgr = new TaskMgr( faultTask );
        for ( int i = 0; i < objectNums; i++ ) {
            mgr.addTask( new DeleteVersion( objectNames.get( i ) ) );
        }
        mgr.execute();
        Assert.assertTrue( mgr.isAllSuccess(), mgr.getErrorMsg() );
        // delete again
        objectNames.removeAll( objectNameList );
        for ( String objectName : objectNames ) {
            for ( int i = 0; i < versionNums; i++ ) {
                s3Client.deleteVersion( bucketName, objectName,
                        String.valueOf( i ) );
            }
        }
        Assert.assertEquals( s3Client.listObjectsV2( bucketName ).getKeyCount(),
                0 );
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

    public class DeleteVersion extends OperateTask {
        private String objectName = null;

        public DeleteVersion( String objectName ) {
            this.objectName = objectName;
        }

        @Override
        public void exec() throws Exception {
            try {
                for ( int i = 0; i < versionNums; i++ ) {
                    s3Client.deleteVersion( bucketName, objectName,
                            String.valueOf( i ) );
                }
                objectNameList.add( this.objectName );
            } catch ( AmazonS3Exception e ) {
                if ( e.getStatusCode() != 500 ) {
                    throw e;
                }
            }
        }
    }
}
