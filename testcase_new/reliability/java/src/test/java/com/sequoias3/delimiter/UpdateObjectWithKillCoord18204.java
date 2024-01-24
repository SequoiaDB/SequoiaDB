package com.sequoias3.delimiter;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.*;
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

/**
 * @Description seqDB-18204 ::设置分隔符过程中db端节点异常
 * @author fanyu
 * @version 1.00
 * @Date 2019.05.23
 */
public class UpdateObjectWithKillCoord18204 extends S3TestBase {
    private boolean runSuccess = false;
    private AmazonS3 s3Client = null;
    private int fileSize = 1024 * new Random().nextInt( 1025 );
    private String filePath = null;
    private String bucketName = "bucket18204";
    private String objectNameBase = "PutObject18204?!";
    private String oldDelimiter = "?";
    private String newDelimiter = "!";
    private int objectNums = 100;
    private List< String > objectNameList = new ArrayList<>();
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
        DelimiterUtils.putBucketDelimiter( bucketName, oldDelimiter );
        DelimiterUtils.checkCurrentDelimiteInfo( bucketName, oldDelimiter );
        for ( int i = 0; i < objectNums; i++ ) {
            String objectName = objectNameBase + "_" + i + "?!_"
                    + TestTools.getRandomString( 1 );
            objectNameList.add( objectName );
            s3Client.putObject( bucketName, objectName, new File( filePath ) );
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
        mgr.addTask( new UpdateDelimiter() );
        mgr.execute();
        Assert.assertTrue( mgr.isAllSuccess(), mgr.getErrorMsg() );
        // 如果故障时，更新分隔符失败，故障恢复后，再次更新分割符
        if ( !DelimiterUtils.getDelimiter( bucketName ).getDelimiter()
                .equals( newDelimiter ) ) {
            DelimiterUtils.updateDelimiterSuccessAgain( bucketName,
                    newDelimiter );
        }
        for ( String objectName : objectNameList ) {
            checkObjectResult( objectName );
        }
        checkListV2Result();
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

    public class UpdateDelimiter extends OperateTask {

        @Override
        public void exec() throws Exception {
            try {
                DelimiterUtils.updateDelimiterSuccessAgain( bucketName,
                        newDelimiter );
            } catch ( AmazonS3Exception e ) {
                if ( e.getStatusCode() != 500 ) {
                    throw e;
                }
            }
        }
    }

    private void checkObjectResult( String objectName ) throws Exception {
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
    }

    private void checkListV2Result() {
        // 通过携带delimiter查询对象列表的对外映射场景检测目录表是否生成新目录，对象元数据表和目录表中数据通过连接db手工校验
        ListObjectsV2Request request = new ListObjectsV2Request()
                .withBucketName( bucketName ).withEncodingType( "url" );
        request.withDelimiter( newDelimiter );
        ListObjectsV2Result result = s3Client.listObjectsV2( request );
        List< String > commonPrefixes = result.getCommonPrefixes();
        Assert.assertEquals( commonPrefixes.size(), 1,
                commonPrefixes.toString() );
        Assert.assertEquals( result.getObjectSummaries().size(), 0,
                "bucketName = " + bucketName + ",delimiter = " + newDelimiter );
    }
}
