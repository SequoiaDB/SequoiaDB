package com.sequoias3.delimiter;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.GroupWrapper;
import com.sequoiadb.commlib.NodeWrapper;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.fault.KillNode;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;
import com.sequoias3.commlibs3.CommLibS3;
import com.sequoias3.commlibs3.S3TestBase;
import com.sequoias3.commlibs3.TestTools;
import com.sequoias3.commlibs3.s3utils.DelimiterUtils;
import com.sequoias3.commlibs3.s3utils.ObjectUtils;
import com.sequoias3.commlibs3.s3utils.UserUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

/**
 * test content: 获取对象列表过程中db端节点异常 testlink-case: seqDB-18202
 * 
 * @author wangkexin
 * @Date 2019.05.23
 * @version 1.00
 */
public class ListObjectsWithKillData18202 extends S3TestBase {
    private String userName = "user18200";
    private String bucketName = "bucket18202";
    private String objectName = "/aa/bb/object18202";
    private String delimiter = "?";
    private int objectNums = 100;
    private String[] objectNames = new String[ objectNums ];
    private AmazonS3 s3Client = null;
    private int fileSize = 1024 * 1;
    private File localPath = null;
    private String filePath = null;
    private String roleName = "normal";
    private String[] accessKeys = null;
    private boolean runSuccess = false;

    @BeforeClass
    private void setUp() throws IOException, ReliabilityException {
        localPath = new File( SdbTestBase.workDir + File.separator
                + TestTools.getClassName() );
        filePath = localPath + File.separator + "localFile_" + fileSize
                + ".txt";

        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );
        TestTools.LocalFile.createFile( filePath, fileSize );

        CommLibS3.clearUser( userName );
        accessKeys = UserUtils.createUser( userName, roleName );
        s3Client = CommLibS3.buildS3Client( accessKeys[ 0 ], accessKeys[ 1 ] );
        s3Client.createBucket( bucketName );
        DelimiterUtils.putBucketDelimiter( bucketName, delimiter,
                accessKeys[ 0 ] );

        for ( int i = 0; i < objectNums; i++ ) {
            String currObjectName = objectName + "_" + i + delimiter
                    + "test.txt";
            s3Client.putObject( bucketName, currObjectName,
                    new File( filePath ) );
            objectNames[ i ] = currObjectName;
        }

        s3Client.putObject( bucketName, "test18202a", new File( filePath ) );
        s3Client.putObject( bucketName, "test18202b", new File( filePath ) );
    }

    @Test
    public void test() throws ReliabilityException, IOException {
        TaskMgr mgr = new TaskMgr();
        GroupMgr groupMgr = GroupMgr.getInstance();
        List< GroupWrapper > dataGroups = groupMgr.getAllDataGroup();

        for ( int i = 0; i < dataGroups.size(); i++ ) {
            String groupName = dataGroups.get( i ).getGroupName();
            GroupWrapper group = groupMgr.getGroupByName( groupName );
            NodeWrapper node = group.getMaster();
            FaultMakeTask faultTask = KillNode.getFaultMakeTask( node, 1 );
            mgr.addTask( faultTask );
        }

        for ( int i = 0; i < 20; i++ ) {
            ListObject listTask = new ListObject();
            mgr.addTask( listTask );
        }

        mgr.execute();
        Assert.assertTrue( mgr.isAllSuccess(), mgr.getErrorMsg() );

        // check whether the cluster is normal and lsn consistency ,the longest
        // waiting time is 600S
        Assert.assertEquals( groupMgr.checkBusinessWithLSN( 600 ), true,
                "checkBusinessWithLSN() occurs timeout" );

        // list objects again
        listObjectsAndCheck();
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( runSuccess ) {
                UserUtils.deleteUser( userName );
                TestTools.LocalFile.removeFile( localPath );
            }
        } finally {
            s3Client.shutdown();
        }
    }

    private class ListObject extends OperateTask {
        @Override
        public void exec() throws IOException {
            try {
                listObjectsAndCheck();
            } catch ( AmazonS3Exception e ) {
                if ( e.getStatusCode() != 500 ) {
                    throw e;
                }
            }
        }
    }

    private void listObjectsAndCheck() throws IOException {
        List< String > expCommprefixes = ObjectUtils
                .getCommPrefixes( objectNames, "", delimiter );
        List< String > matchContentsList = new ArrayList<>();
        matchContentsList.add( "test18202a" );
        matchContentsList.add( "test18202b" );
        AmazonS3 s3Client = CommLibS3.buildS3Client( accessKeys[ 0 ],
                accessKeys[ 1 ] );
        try {
            DelimiterUtils.listObjectsWithDelimiter( s3Client, bucketName,
                    delimiter, expCommprefixes, matchContentsList );
        } finally {
            if ( s3Client != null ) {
                s3Client.shutdown();
            }
        }
    }
}