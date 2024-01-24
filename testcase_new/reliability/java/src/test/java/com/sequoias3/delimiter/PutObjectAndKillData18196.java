package com.sequoias3.delimiter;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.concurrent.CopyOnWriteArrayList;

import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.amazonaws.SdkClientException;
import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.amazonaws.services.s3.model.BucketVersioningConfiguration;
import com.amazonaws.services.s3.model.ListObjectsV2Request;
import com.amazonaws.services.s3.model.ListObjectsV2Result;
import com.amazonaws.services.s3.model.ObjectMetadata;
import com.amazonaws.services.s3.model.PutObjectResult;
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
import com.sequoias3.commlibs3.s3utils.DelimiterUtils;

/**
 * @Description seqDB-18196 ::开启版本控制，创建对象过程中sdb端节点故障
 * @author wuyan
 * @Date 2019.01.17
 * @version 1.00
 */
public class PutObjectAndKillData18196 extends S3TestBase {
    private boolean runSuccess = false;
    private AmazonS3 s3Client = null;
    private int fileSize = 1024 * 200;
    private int objectNums = 50;
    private int versionNums = 2;
    private String filePath = null;
    private String delimiter = "?";
    private String objectNameBase = "PutObject18196";
    private List< String > objectNames = new ArrayList< String >();
    private List< String > objectNameList = new CopyOnWriteArrayList< String
            >();
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
        CommLibS3.setBucketVersioning( s3Client, bucketName,
                BucketVersioningConfiguration.ENABLED );
        for ( int i = 0; i < objectNums; i++ ) {
            objectNames.add( objectNameBase + i + "_" + delimiter );
        }
        DelimiterUtils.putBucketDelimiter( bucketName, delimiter );

    }

    @Test
    public void test() throws Exception {
        GroupMgr groupMgr = GroupMgr.getInstance();
        List< GroupWrapper > glist = groupMgr.getAllDataGroup();
        TaskMgr mgr = new TaskMgr();
        for ( int i = 0; i < glist.size(); i++ ) {
            String groupName = glist.get( i ).getGroupName();
            GroupWrapper group = groupMgr.getGroupByName( groupName );
            NodeWrapper node = group.getMaster();
            FaultMakeTask faultTask = KillNode.getFaultMakeTask( node, 1 );
            mgr.addTask( faultTask );
            System.out.println( "KillNode:i=" + i + "" + node.hostName() + ":"
                    + node.svcName() );
        }

        for ( int i = 0; i < objectNums; i++ ) {
            mgr.addTask( new PutObject( objectNames.get( i ) ) );
        }
        mgr.execute();
        Assert.assertTrue( mgr.isAllSuccess(), mgr.getErrorMsg() );
        Assert.assertTrue( groupMgr.checkBusinessWithLSN( 120 ),
                "node start fail!" );
        // put again
        List< String > expQueryCommprefixs = new ArrayList< String >(
                objectNames );
        objectNames.removeAll( objectNameList );
        for ( String objectName : objectNames ) {
            for ( int i = 0; i < versionNums; i++ ) {
                PutObjectResult obj = s3Client.putObject( bucketName,
                        objectName, new File( filePath ) );
                checkPutResult( obj, objectName );
            }
        }
        checkResultByListWithDelimiter( expQueryCommprefixs );
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

    public class PutObject extends OperateTask {
        private String objectName = null;
        private AmazonS3 s3Client1 = CommLibS3.buildS3Client();

        public PutObject( String objectName ) {
            this.objectName = objectName;
        }

        @Override
        public void exec() throws Exception {
            try {
                for ( int i = 0; i < versionNums; i++ ) {
                    s3Client1.putObject( bucketName, this.objectName,
                            new File( filePath ) );
                    objectNameList.add( this.objectName );
                }
            } catch ( AmazonS3Exception e ) {
                if ( e.getStatusCode() != 500 ) {
                    throw new Exception( objectName, e );
                }
            } catch ( SdkClientException e ) {
                if ( !e.getMessage()
                        .contains( "Unable to execute HTTP request" ) ) {
                    throw new Exception( objectName, e );
                }
            } finally {
                if ( s3Client1 != null ) {
                    s3Client1.shutdown();
                }
            }
        }
    }

    private void checkPutResult( PutObjectResult obj, String objectName )
            throws IOException {
        Assert.assertEquals( obj.getETag(), TestTools.getMD5( filePath ) );
        ObjectMetadata metadata = obj.getMetadata();
        int versionId = Integer.parseInt( metadata.getVersionId() );
        if ( versionId < 0 || versionId > versionNums ) {
            Assert.fail( "key=" + objectName + ",versionId="
                    + metadata.getVersionId() );
        }
    }

    private void checkResultByListWithDelimiter(
            List< String > expCommprefixs ) {
        // list object by delimeter.
        ListObjectsV2Request request = new ListObjectsV2Request()
                .withBucketName( bucketName ).withEncodingType( "url" );
        request.withDelimiter( delimiter );
        ListObjectsV2Result result = s3Client.listObjectsV2( request );
        List< String > commonPrefixes = result.getCommonPrefixes();
        Collections.sort( expCommprefixs );
        Assert.assertEquals( commonPrefixes, expCommprefixs );
    }
}
