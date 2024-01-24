package com.sequoias3.delimiter;

import com.amazonaws.SdkClientException;
import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.amazonaws.services.s3.model.ListObjectsV2Request;
import com.amazonaws.services.s3.model.ListObjectsV2Result;
import com.amazonaws.services.s3.model.S3Object;
import com.amazonaws.services.s3.model.S3ObjectInputStream;
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
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.File;
import java.util.ArrayList;
import java.util.List;
import java.util.Random;
import java.util.concurrent.CopyOnWriteArrayList;

/**
 * @Description seqDB-18198 :: 增加对象过程中db端节点异常
 * @author fanyu
 * @version 1.00
 * @Date 2019.05.23
 */
public class PutObjectWithKillCoord18198 extends S3TestBase {
    private boolean runSuccess = false;
    private AmazonS3 s3Client = null;
    private int fileSize = 1024 * new Random().nextInt( 1025 );
    private int objectNums = 100;
    private String filePath = null;
    private String bucketName = "bucket18198";
    private String objectNameBase = "object18198?";
    private String delimiter = "?";
    private List< String > objectNames = new ArrayList< String >();
    private List< String > objectNameList = new CopyOnWriteArrayList< String
            >();
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
        DelimiterUtils.putBucketDelimiter( bucketName, delimiter );
        DelimiterUtils.checkCurrentDelimiteInfo( bucketName, delimiter );
        for ( int i = 0; i < objectNums; i++ ) {
            objectNames.add( objectNameBase + "_" + i + "?_"
                    + TestTools.getRandomString( 1 ) );
        }
    }

    @Test
    public void test() throws Exception {
        // kill coord
        TaskMgr mgr = new TaskMgr();
        for ( NodeWrapper node : coordGroup.getNodes() ) {
            FaultMakeTask faultTask = KillNode.getFaultMakeTask( node, 2 );
            mgr.addTask( faultTask );
        }
        for ( int i = 0; i < objectNums; i++ ) {
            mgr.addTask( new PutObject( objectNames.get( i ) ) );
        }
        mgr.execute();
        Assert.assertEquals( mgr.isAllSuccess(), true, mgr.getErrorMsg() );
        // put again
        objectNames.removeAll( objectNameList );
        for ( String objectName : objectNames ) {
            s3Client.putObject( bucketName, objectName, new File( filePath ) );
            objectNameList.add( objectName );
        }
        Assert.assertEquals( objectNameList.size(), objectNums,
                "objectNameList = " + objectNameList.toString() );
        for ( String objectName : objectNameList ) {
            checkResult( objectName );
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
            try {
                s3Client.putObject( bucketName, this.objectName,
                        new File( filePath ) );
                objectNameList.add( this.objectName );
            } catch ( AmazonS3Exception e ) {
                if ( e.getStatusCode() != 500 ) {
                    throw e;
                }
            } catch ( SdkClientException e ) {
                if ( !e.getMessage()
                        .contains( "Unable to execute HTTP request" ) ) {
                    throw new Exception( objectName, e );
                }
            } catch ( Exception e ) {
                if ( !e.getMessage().contains( "I/O error on POST request" ) ) {
                    throw e;
                }
            }
        }
    }

    private void checkResult( String objectName ) throws Exception {
        S3Object obj = s3Client.getObject( bucketName, objectName );
        S3ObjectInputStream s3is = obj.getObjectContent();
        String downloadPath = TestTools.LocalFile.initDownloadPath( localPath,
                TestTools.getMethodName(), Thread.currentThread().getId() );
        ObjectUtils.inputStream2File( s3is, downloadPath );
        s3is.close();
        String actMd5 = TestTools.getMD5( downloadPath );
        String expMd5 = TestTools.getMD5( filePath );
        Assert.assertEquals( obj.getKey(), objectName );
        Assert.assertEquals( actMd5, expMd5 );
        // 通过携带delimiter查询对象列表的对外映射场景检测目录表是否生成新目录，对象元数据表和目录表中数据通过连接db手工校验
        ListObjectsV2Request request = new ListObjectsV2Request()
                .withBucketName( bucketName ).withEncodingType( "url" );
        request.withDelimiter( delimiter );
        ListObjectsV2Result result = s3Client.listObjectsV2( request );
        List< String > commonPrefixes = result.getCommonPrefixes();
        Assert.assertEquals( commonPrefixes.size(), 1 );
        Assert.assertEquals( result.getObjectSummaries().size(), 0 );
    }
}
