package com.sequoias3.delimiter;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
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
import com.amazonaws.services.s3.model.ObjectMetadata;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;
import com.sequoias3.commlibs3.CommLibS3;
import com.sequoias3.commlibs3.S3TestBase;
import com.sequoias3.commlibs3.TestTools;
import com.sequoias3.commlibs3.s3utils.DelimiterUtils;
import com.sequoias3.commlibs3.s3utils.ObjectUtils;
import com.sequoias3.commlibs3.s3utils.S3NodeRestart;
import com.sequoias3.commlibs3.s3utils.UserUtils;
import com.sequoias3.commlibs3.s3utils.bean.S3NodeWrapper;

/**
 * @Description seqDB-18205：开启版本控制，创建对象过程中s3节点异常*
 * @author wangkexin
 * @Date 2019.05.23
 * @version 1.00
 */
public class CreateObjectWithReStartS3N18205 extends S3TestBase {
    private String userName = "user18205";
    private String bucketName = "bucket18205";
    private int objectVersionNums = 3;
    private int objectNums = 50;
    private String delimiter = "#";
    private String objectNameBase = "#object18205_";
    private List< String > objectNames = new ArrayList< String >();
    private List< String > putSuccessObjectNames = new CopyOnWriteArrayList< String >();
    private String roleName = "normal";
    private String[] accessKeys = null;
    private AmazonS3 s3Client = null;
    private String filePath = null;
    private File localPath = null;
    private int fileSize = 5;
    private boolean runSuccess = false;

    @BeforeClass
    private void setUp() throws IOException {
        localPath = new File( SdbTestBase.workDir + File.separator
                + TestTools.getClassName() );
        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );
        filePath = localPath + File.separator + "localFile_" + fileSize
                + ".txt";
        TestTools.LocalFile.createFile( filePath, fileSize );
        CommLibS3.clearUser( userName );
        accessKeys = UserUtils.createUser( userName, roleName );
        s3Client = CommLibS3.buildS3Client( accessKeys[ 0 ], accessKeys[ 1 ] );
        s3Client.createBucket( bucketName );
        CommLibS3.setBucketVersioning( s3Client, bucketName,
                BucketVersioningConfiguration.ENABLED );

        for ( int i = 0; i < objectNums; i++ ) {
            objectNames.add( objectNameBase + i + "_" + delimiter );
        }
        DelimiterUtils.putBucketDelimiter( bucketName, delimiter,
                accessKeys[ 0 ] );
    }

    @Test
    public void test() throws Exception {
        // 并发多线程上传对象A,使对象A存在多个版本
        FaultMakeTask faultMakeTask = S3NodeRestart
                .getFaultMakeTask( new S3NodeWrapper(), 15, 10 );
        TaskMgr mgr = new TaskMgr( faultMakeTask );
        for ( int i = 0; i < objectNums; i++ ) {
            mgr.addTask( new PutObject( objectNames.get( i ) ) );
        }
        mgr.execute();
        Assert.assertTrue( mgr.isAllSuccess(), mgr.getErrorMsg() );
        // 继续上传对象A
        s3Client = CommLibS3.buildS3Client( accessKeys[ 0 ], accessKeys[ 1 ] );
        checkResult();
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() throws Exception {
        try {
            if ( runSuccess ) {
                UserUtils.deleteUser( userName );
                TestTools.LocalFile.removeFile( localPath );
            }
        } finally {
            if ( s3Client != null ) {
                s3Client.shutdown();
            }
        }
    }

    private class PutObject extends OperateTask {
        private String objectName = null;

        public PutObject( String objectName ) {
            this.objectName = objectName;
        }

        @Override
        public void exec() throws Exception {
            AmazonS3 s3Client = CommLibS3.buildS3Client( accessKeys[ 0 ],
                    accessKeys[ 1 ] );
            try {
                for ( int i = 0; i < objectVersionNums; i++ ) {
                    s3Client.putObject( bucketName, this.objectName,
                            new File( filePath ) );
                    putSuccessObjectNames.add( this.objectName );
                }

            } catch ( AmazonS3Exception e ) {
                if ( e.getStatusCode() != 500 ) {
                    throw new Exception( objectName + ":" + objectName, e );
                }
            } catch ( SdkClientException e ) {
                if ( !e.getMessage()
                        .contains( "Unable to execute HTTP request" ) ) {
                    throw e;
                }
            } catch ( Exception e ) {
                if ( !e.getMessage().contains( "I/O error on POST request" ) ) {
                    throw e;
                }
            } finally {
                if ( s3Client != null ) {
                    s3Client.shutdown();
                }
            }
        }
    }

    private void checkResult() throws Exception {
        // 故障前创建成功的对象，检查对象信息正确
        String expFileMd5 = TestTools.getMD5( filePath );
        if ( !putSuccessObjectNames.isEmpty() )
            for ( String keyName : putSuccessObjectNames ) {
                ObjectMetadata objectMetaInfo = s3Client
                        .getObjectMetadata( bucketName, keyName );
                String curVersionId = objectMetaInfo.getVersionId();
                // 如果故障前所有版本对象都创建成功，则检查所有版本对象内容
                if ( curVersionId.equals( objectVersionNums + "" ) ) {
                    for ( int i = 0; i < objectVersionNums; i++ ) {
                        String versionId = i + "";
                        String downfileMd5 = ObjectUtils.getMd5OfObject(
                                s3Client, localPath, bucketName, keyName,
                                versionId );
                        Assert.assertEquals( downfileMd5, expFileMd5,
                                "---check object=" + keyName + ", versionId="
                                        + versionId );
                    }

                } else {
                    String downfileMd5 = ObjectUtils.getMd5OfObject( s3Client,
                            localPath, bucketName, keyName, curVersionId );
                    Assert.assertEquals( downfileMd5, expFileMd5,
                            "---check object=" + keyName + ", versionId="
                                    + curVersionId );
                    // 再次插入新版本对象，检查结果
                    s3Client.putObject( bucketName, keyName,
                            new File( filePath ) );
                    String downfileMd51 = ObjectUtils.getMd5OfObject( s3Client,
                            localPath, bucketName, keyName );
                    Assert.assertEquals( downfileMd51, expFileMd5,
                            "---check reWrite object=" + keyName );
                }

            }
        // 故障后重新创建对象,忽略故障时创建返回失败实际创建成功的对象
        objectNames.removeAll( putSuccessObjectNames );
        for ( String keyName : objectNames ) {
            for ( int i = 0; i < objectVersionNums; i++ ) {
                String versionId = i + "";
                s3Client.putObject( bucketName, keyName, new File( filePath ) );
                String downfileMd5 = ObjectUtils.getMd5OfObject( s3Client,
                        localPath, bucketName, keyName, versionId );
                Assert.assertEquals( downfileMd5, expFileMd5, "---check object="
                        + keyName + ", versionId=" + versionId );
            }
        }
    }

}