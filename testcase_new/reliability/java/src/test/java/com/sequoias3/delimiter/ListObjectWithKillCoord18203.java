package com.sequoias3.delimiter;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.amazonaws.services.s3.model.BucketVersioningConfiguration;
import com.amazonaws.services.s3.model.ListVersionsRequest;
import com.amazonaws.services.s3.model.VersionListing;
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
import org.springframework.util.LinkedMultiValueMap;
import org.springframework.util.MultiValueMap;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.File;
import java.io.IOException;
import java.util.List;

/**
 * test content: 获取对象版本列表过程中db端节点异常 testlink-case: seqDB-18203
 * 
 * @author wangkexin
 * @Date 2019.05.23
 * @version 1.00
 */
public class ListObjectWithKillCoord18203 extends S3TestBase {
    private String userName = "user18200";
    private String bucketName = "bucket18203";
    private String objectName = "/aa/bb/object18203";
    private String delimiter = "?";
    private int objectNums = 100;
    private int versionNum = 3;
    private String[] objectNames = new String[ objectNums ];
    private AmazonS3 s3Client = null;
    private int fileSize = 1024 * 1;
    private File localPath = null;
    private String filePath = null;
    private String roleName = "normal";
    private GroupMgr groupMgr = null;
    private GroupWrapper coordGroup = null;
    private String[] accessKeys = null;
    private boolean runSuccess = false;

    @BeforeClass
    private void setUp() throws IOException, ReliabilityException {
        groupMgr = GroupMgr.getInstance();
        coordGroup = groupMgr.getGroupByName( "SYSCoord" );

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
        CommLibS3.setBucketVersioning( s3Client, bucketName,
                BucketVersioningConfiguration.ENABLED );
        DelimiterUtils.putBucketDelimiter( bucketName, delimiter,
                accessKeys[ 0 ] );
        for ( int i = 0; i < objectNums / 2; i++ ) {
            String currObjectName = objectName + "_" + i + delimiter
                    + "test.txt";
            for ( int v = 0; v < versionNum; v++ ) {
                s3Client.putObject( bucketName, currObjectName,
                        new File( filePath ) );
            }
            objectNames[ i ] = currObjectName;
        }
        for ( int i = objectNums / 2; i < objectNums; i++ ) {
            String objectNameWithoutDelimiter = objectName + "_" + i + ".txt";
            for ( int v = 0; v < versionNum; v++ ) {
                s3Client.putObject( bucketName, objectNameWithoutDelimiter,
                        new File( filePath ) );
            }
            objectNames[ i ] = objectNameWithoutDelimiter;
        }
    }

    @Test
    public void test() throws ReliabilityException, IOException {
        // kill coord when list object versions
        TaskMgr mgr = new TaskMgr();
        for ( NodeWrapper node : coordGroup.getNodes() ) {
            FaultMakeTask faultTask = KillNode.getFaultMakeTask( node, 0 );
            mgr.addTask( faultTask );
        }
        for ( int i = 0; i < 20; i++ ) {
            ListObject listTask = new ListObject();
            mgr.addTask( listTask );
        }
        mgr.execute();
        Assert.assertTrue( mgr.isAllSuccess(), mgr.getErrorMsg() );
        // list objects again
        listVersionsAndCheck();
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
                listVersionsAndCheck();
            } catch ( AmazonS3Exception e ) {
                if ( !e.getErrorCode().equals( "GetDBConnectFail" ) ) {
                    throw e;
                }
            }
        }
    }

    private void listVersionsAndCheck() throws IOException {
        List< String > expCommprefixes = ObjectUtils
                .getCommPrefixes( objectNames, "", delimiter );
        MultiValueMap< String, String > expMap = new LinkedMultiValueMap< String, String >();
        for ( int i = objectNums / 2; i < objectNums; i++ ) {
            for ( int j = versionNum - 1; j >= 0; j-- ) {
                expMap.add( objectNames[ i ], String.valueOf( j ) );
            }
        }

        AmazonS3 s3Client = CommLibS3.buildS3Client( accessKeys[ 0 ],
                accessKeys[ 1 ] );
        try {
            VersionListing verList = s3Client.listVersions(
                    new ListVersionsRequest().withBucketName( bucketName )
                            .withDelimiter( delimiter ) );
            ObjectUtils.checkListVSResults( verList, expCommprefixes, expMap );
        } finally {
            if ( s3Client != null ) {
                s3Client.shutdown();
            }
        }
    }
}