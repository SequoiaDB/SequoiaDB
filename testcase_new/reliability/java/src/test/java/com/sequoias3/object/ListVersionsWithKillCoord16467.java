package com.sequoias3.object;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.amazonaws.services.s3.model.BucketVersioningConfiguration;
import com.amazonaws.services.s3.model.ListVersionsRequest;
import com.amazonaws.services.s3.model.VersionListing;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.GroupWrapper;
import com.sequoiadb.commlib.NodeWrapper;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.fault.KillNode;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;
import com.sequoias3.commlibs3.CommLibS3;
import com.sequoias3.commlibs3.S3TestBase;
import com.sequoias3.commlibs3.s3utils.ObjectUtils;
import org.springframework.util.LinkedMultiValueMap;
import org.springframework.util.MultiValueMap;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.IOException;
import java.util.ArrayList;
import java.util.UUID;

/**
 * @Description seqDB-16467 :: 获取对象版本列表过程中db端节点异常
 * @author fanyu
 * @Date 2019.01.17
 * @version 1.00
 */
public class ListVersionsWithKillCoord16467 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket16467";
    private String objectName = "object16467";
    private AmazonS3 s3Client = null;
    private int versionNum = 2000;
    private GroupMgr groupMgr = null;
    private GroupWrapper coordGroup = null;

    @BeforeClass
    private void setUp() throws IOException, ReliabilityException {
        groupMgr = GroupMgr.getInstance();
        coordGroup = groupMgr.getGroupByName( "SYSCoord" );

        s3Client = CommLibS3.buildS3Client();
        CommLibS3.clearBucket( s3Client, bucketName );
        s3Client.createBucket( bucketName );
        CommLibS3.setBucketVersioning( s3Client, bucketName,
                BucketVersioningConfiguration.ENABLED );
        for ( int i = 0; i < versionNum; i++ ) {
            s3Client.putObject( bucketName, objectName,
                    String.valueOf( UUID.randomUUID() ) );
        }
    }

    @Test
    public void test() throws ReliabilityException, IOException {
        // kill coord when list objects
        TaskMgr mgr = new TaskMgr();
        for ( NodeWrapper node : coordGroup.getNodes() ) {
            FaultMakeTask faultTask = KillNode.getFaultMakeTask( node, 2 );
            mgr.addTask( faultTask );
        }
        ListVersions listTask = new ListVersions();
        mgr.addTask( listTask );
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
                CommLibS3.clearBucket( s3Client, bucketName );
            }
        } finally {
            s3Client.shutdown();
        }
    }

    public class ListVersions extends OperateTask {
        @Override
        public void exec() throws Exception {
            try {
                listVersionsAndCheck();
            } catch ( AmazonS3Exception e ) {
                if ( e.getStatusCode() != 500 ) {
                    throw e;
                }
            }
        }
    }

    private void listVersionsAndCheck() throws IOException {
        String keyMarker = objectName;
        String versionIdMarker = String.valueOf( versionNum );
        VersionListing vsList;
        int i = 0;
        do {
            // list by prefix/keyMarker/versionIdMarker
            vsList = s3Client.listVersions( new ListVersionsRequest()
                    .withBucketName( bucketName ).withKeyMarker( keyMarker )
                    .withVersionIdMarker( versionIdMarker ) );
            // check
            MultiValueMap< String, String > expMap = new LinkedMultiValueMap< String, String >();
            for ( int j = versionNum - i * 1000 - 1; j >= vsList.getMaxKeys()
                    - i * 1000; j-- ) {
                expMap.add( objectName, String.valueOf( j ) );
            }
            ObjectUtils.checkListVSResults( vsList, new ArrayList< String >(),
                    expMap );
            i++;
            // next keyMark and versionIdMrker
            keyMarker = vsList.getNextKeyMarker();
            versionIdMarker = vsList.getNextVersionIdMarker();
        } while ( vsList.isTruncated() );
    }
}
