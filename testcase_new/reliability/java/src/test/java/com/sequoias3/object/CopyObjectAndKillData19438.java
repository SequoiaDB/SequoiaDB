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

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.GroupWrapper;
import com.sequoiadb.commlib.NodeWrapper;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.fault.KillNode;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;
import com.sequoias3.commlibs3.CommLibS3;
import com.sequoias3.commlibs3.S3TestBase;
import com.sequoias3.commlibs3.TestTools;
import com.sequoias3.commlibs3.s3utils.ObjectUtils;

/**
 * @Description seqDB-19438 :复制对象过程中db端节点异常
 * @author wuyan
 * @Date 2019.08.12
 * @version 1.00
 */
public class CopyObjectAndKillData19438 extends S3TestBase {
    private boolean runSuccess = false;
    private AmazonS3 s3Client = null;
    private String bucketName = "bucket19438";
    private String srcKeyName = "/SRC/test19438.tar";
    private String destKeyName = "/DEST/test19438.tar";
    private List< String > destKeyNames = new ArrayList<>();
    private List< String > copyFailDestKeyNames = Collections
            .synchronizedList( new ArrayList< String >() );
    private int destKeyNameNum = 10;
    private int fileSize = 1024 * 1024 * 50;
    private String filePath = null;
    private File localPath = null;

    @BeforeClass
    private void setUp() throws IOException {
        localPath = new File( SdbTestBase.workDir + File.separator
                + TestTools.getClassName() );
        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );
        filePath = localPath + File.separator + "localFile_" + fileSize
                + ".txt";
        TestTools.LocalFile.createFile( filePath, fileSize );
        s3Client = CommLibS3.buildS3Client();
        CommLibS3.clearBucket( s3Client, bucketName );
        s3Client.createBucket( bucketName );
        s3Client.putObject( bucketName, srcKeyName, new File( filePath ) );
    }

    @Test
    public void testCopyObject() throws Exception {
        TaskMgr mgr = new TaskMgr();

        for ( int i = 0; i < destKeyNameNum; i++ ) {
            String subDestKeyName = destKeyName + "_" + i + "_png";
            mgr.addTask( new CopyObject( subDestKeyName ) );
        }

        GroupMgr groupMgr = GroupMgr.getInstance();
        List< GroupWrapper > glist = groupMgr.getAllDataGroup();
        for ( int i = 0; i < glist.size(); i++ ) {
            String groupName = glist.get( i ).getGroupName();
            GroupWrapper group = groupMgr.getGroupByName( groupName );
            NodeWrapper node = group.getMaster();
            FaultMakeTask faultTask = KillNode.getFaultMakeTask( node, 1 );
            mgr.addTask( faultTask );
            System.out.println( "KillNode:i=" + i + "" + node.hostName() + ":"
                    + node.svcName() );
        }

        mgr.execute();
        Assert.assertTrue( mgr.isAllSuccess(), mgr.getErrorMsg() );
        Assert.assertTrue( groupMgr.checkBusinessWithLSN( 120 ),
                "node start fail!" );

        checkResult();
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
        private String destKeyName;
        private AmazonS3 s3Client1 = CommLibS3.buildS3Client();

        private CopyObject( String destKeyName ) {
            this.destKeyName = destKeyName;
        }

        @Override
        public void exec() throws Exception {
            try {
                s3Client.copyObject( bucketName, srcKeyName, bucketName,
                        destKeyName );
            } catch ( AmazonS3Exception e ) {
                copyFailDestKeyNames.add( destKeyName );
                // 200:CopyObjectFailed 500:INTERNAL_SERVER_ERROR
                if ( e.getStatusCode() != 200 && e.getStatusCode() != 500 ) {
                    throw new Exception( destKeyName, e );
                }
            } finally {
                if ( s3Client1 != null ) {
                    s3Client1.shutdown();
                }
            }
        }
    }

    private void checkResult() throws Exception {
        for ( String destKeyName : copyFailDestKeyNames ) {
            s3Client.copyObject( bucketName, srcKeyName, bucketName,
                    destKeyName );
        }

        // check the copy file
        for ( String destKeyName : destKeyNames ) {
            String downfileMd5 = ObjectUtils.getMd5OfObject( s3Client,
                    localPath, bucketName, destKeyName );
            Assert.assertEquals( downfileMd5, TestTools.getMD5( filePath ) );
        }

    }
}
