package com.sequoias3.object;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.amazonaws.SdkClientException;
import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.sequoiadb.commlib.SdbTestBase;
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
 * @Description seqDB-19439:复制对象过程中S3节点异常
 * @Author huangxiaoni
 * @Date 2019.08.12
 */
public class CopyObjectAndRestartS319439 extends S3TestBase {
    private boolean runSuccess = false;
    private AmazonS3 s3Client = null;
    private String bucketName = "bucket19439";
    private String srcKeyName = "srcObj19439";
    private String dstKeyNameBase = "dstObj19439";
    private List< String > dstKeyNames = new ArrayList<>();
    private int dstKeyNameNum = 20;
    private List< String > copyFailDstKeyNames = Collections
            .synchronizedList( new ArrayList< String >() );
    private int fileSize = 1024 * 1024 * 30;
    private String filePath = null;
    private File localPath = null;

    @BeforeClass
    private void setUp() throws IOException {
        localPath = new File( SdbTestBase.workDir + File.separator
                + TestTools.getClassName() );
        filePath = localPath + File.separator + "localFile_" + fileSize
                + ".txt";
        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );
        TestTools.LocalFile.createFile( filePath, fileSize );

        s3Client = CommLibS3.buildS3Client();
        CommLibS3.clearBucket( s3Client, bucketName );
        s3Client.createBucket( bucketName );
        s3Client.putObject( bucketName, srcKeyName, new File( filePath ) );

        for ( int i = 0; i < dstKeyNameNum; i++ ) {
            String dstKeyName = dstKeyNameBase + "_" + i;
            dstKeyNames.add( dstKeyName );
        }
    }

    @Test
    public void test() throws Exception {
        TaskMgr mgr = new TaskMgr();
        // task: copy object
        for ( String dstKeyName : dstKeyNames ) {
            mgr.addTask( new CopyObject( dstKeyName ) );
        }
        // task: restart s3 node
        FaultMakeTask faultMakeTask = S3NodeRestart
                .getFaultMakeTask( new S3NodeWrapper(), 1, 10 );
        mgr.addTask( faultMakeTask );
        mgr.execute();
        Assert.assertTrue( mgr.isAllSuccess(), mgr.getErrorMsg() );

        // copy failed object again after recovery
        for ( String dstKeyName : copyFailDstKeyNames ) {
            s3Client.copyObject( bucketName, srcKeyName, bucketName,
                    dstKeyName );
        }

        // check all object results
        for ( String dstKeyName : dstKeyNames ) {
            String downfileMd5 = ObjectUtils.getMd5OfObject( s3Client,
                    localPath, bucketName, dstKeyName );
            Assert.assertEquals( downfileMd5, TestTools.getMD5( filePath ) );
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
            if ( s3Client != null )
                s3Client.shutdown();

        }
    }

    public class CopyObject extends OperateTask {
        private String dstKeyName;
        private AmazonS3 s3 = CommLibS3.buildS3Client();

        private CopyObject( String dstKeyName ) {
            this.dstKeyName = dstKeyName;
        }

        @Override
        public void exec() throws Exception {
            try {
                s3Client.copyObject( bucketName, srcKeyName, bucketName,
                        dstKeyName );
            } catch ( AmazonS3Exception e ) {
                copyFailDstKeyNames.add( dstKeyName );
                // 200:CopyObjectFailed 500:INTERNAL_SERVER_ERROR
                if ( e.getStatusCode() != 200 && e.getStatusCode() != 500 ) {
                    throw new Exception( dstKeyName, e );
                }
            } catch ( SdkClientException e ) {
                copyFailDstKeyNames.add( dstKeyName );
                if ( !e.getMessage()
                        .contains( "Unable to execute HTTP request" ) ) {
                    throw e;
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
