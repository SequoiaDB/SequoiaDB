package com.sequoias3.delimiter;

import java.io.File;
import java.io.IOException;
import java.util.Random;
import java.util.concurrent.atomic.AtomicInteger;

import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.amazonaws.services.s3.model.BucketVersioningConfiguration;
import com.amazonaws.services.s3.model.GetObjectRequest;
import com.amazonaws.services.s3.model.S3Object;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.GroupWrapper;
import com.sequoiadb.commlib.NodeWrapper;
import com.sequoiadb.fault.KillNode;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;
import com.sequoias3.commlibs3.CommLibS3;
import com.sequoias3.commlibs3.S3TestBase;
import com.sequoias3.commlibs3.TestTools;
import com.sequoias3.commlibs3.s3utils.DelimiterUtils;
import com.sequoias3.commlibs3.s3utils.ObjectUtils;

/**
 * @Description seqDB-18199 :: 更新对象过程中db端节点异常
 * @author fanyu
 * @version 1.00
 * @Date 2019.05.23
 */
public class UpdateObjectWithKillCoord18199 extends S3TestBase {
    private boolean runSuccess = false;
    private AmazonS3 s3Client = null;
    private int fileSize = 1024 * new Random().nextInt( 1025 );
    private String filePath = null;
    private String bucketName = "bucket18204";
    private String objectName = "PutObject18199?%";
    private String delimiter = "?";
    private String updateDelimiter = "%";
    private int versionNum = 100;
    private AtomicInteger count = new AtomicInteger( 0 );
    private File localPath = null;
    private GroupMgr groupMgr = null;
    private GroupWrapper coordGroup = null;

    @BeforeClass
    private void setUp() throws Exception {
        localPath = new File( S3TestBase.workDir + File.separator
                + TestTools.getClassName() );
        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );
        filePath = localPath + File.separator + "localFile_" + fileSize
                + ".txt";
        TestTools.LocalFile.createFile( filePath );
        groupMgr = GroupMgr.getInstance();
        coordGroup = groupMgr.getGroupByName( "SYSCoord" );

        s3Client = CommLibS3.buildS3Client();
        CommLibS3.clearBucket( s3Client, bucketName );
        s3Client.createBucket( bucketName );
        CommLibS3.setBucketVersioning( s3Client, bucketName,
                BucketVersioningConfiguration.ENABLED );
        DelimiterUtils.putBucketDelimiter( bucketName, delimiter );
        DelimiterUtils.checkCurrentDelimiteInfo( bucketName, delimiter );
        s3Client.putObject( bucketName, objectName, new File( filePath ) );
        DelimiterUtils.updateDelimiterSuccessAgain( bucketName,
                updateDelimiter );
    }

    @Test
    public void test() throws Exception {
        // kill coord
        TaskMgr mgr = new TaskMgr();
        for ( NodeWrapper node : coordGroup.getNodes() ) {
            FaultMakeTask faultTask = KillNode.getFaultMakeTask( node, 2 );
            mgr.addTask( faultTask );
        }
        for ( int i = 0; i < versionNum; i++ ) {
            mgr.addTask( new PutObject( objectName ) );
        }
        mgr.execute();
        Assert.assertEquals( mgr.isAllSuccess(), true, mgr.getErrorMsg() );

        // check result
        getObjectAndCheck( 0, count.get() );

        // put last failed version
        for ( int i = count.get(); i < versionNum; i++ ) {
            s3Client.putObject( bucketName, this.objectName,
                    new File( filePath ) );
        }

        // check again
        getObjectAndCheck( count.get(), versionNum );
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
            if ( s3Client != null ) {
                s3Client.shutdown();
            }
        }
    }

    public class PutObject extends OperateTask {
        private String objectName = null;

        public PutObject( String objectName ) {
            this.objectName = objectName;
        }

        @Override
        public void exec() throws Exception {
            AmazonS3 s3 = null;
            try {
                s3 = CommLibS3.buildS3Client();
                s3.putObject( bucketName, this.objectName,
                        new File( filePath ) );
                count.incrementAndGet();
            } catch ( AmazonS3Exception e ) {
                if ( e.getStatusCode() != 500 ) {
                    throw e;
                }
            } finally {
                if ( s3 != null ) {
                    s3.shutdown();
                }
            }
        }
    }

    private void getObjectAndCheck( int startVersionId, int endVersionId )
            throws IOException {
        String expMd5 = TestTools.getMD5( filePath );
        for ( int i = startVersionId; i < endVersionId; i++ ) {
            GetObjectRequest request = new GetObjectRequest( bucketName,
                    objectName ).withVersionId( String.valueOf( i ) );
            S3Object object = s3Client.getObject( request );
            String downloadPath = localPath + File.separator + "download" + i;
            Assert.assertEquals( object.getObjectMetadata().getETag(), expMd5,
                    "objectName = " + objectName + "versionId = " + i );
            ObjectUtils.inputStream2File( object.getObjectContent(),
                    downloadPath );
            Assert.assertEquals( TestTools.getMD5( downloadPath ), expMd5,
                    "objectName = " + objectName + ",versionId = " + i );
        }
        // check current version
        S3Object object = s3Client.getObject( bucketName, objectName );
        int currentVersionId = Integer
                .parseInt( object.getObjectMetadata().getVersionId() );
        Assert.assertTrue( currentVersionId >= endVersionId,
                "currentVersionId = " + currentVersionId + "expVersionIdNum = "
                        + endVersionId );
        if ( currentVersionId > endVersionId ) {
            String downloadPath = localPath + File.separator + "download"
                    + currentVersionId;
            ObjectUtils.inputStream2File( object.getObjectContent(),
                    downloadPath );
            Assert.assertEquals( TestTools.getMD5( downloadPath ),
                    expMd5 );
        }
    }
}
