package com.sequoiadb.driveconnection.serial;

import com.sequoiadb.auth.Util;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.base.UserConfig;
import com.sequoiadb.datasource.SequoiadbDatasource;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.testcommon.SdbTestBase;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.File;

/**
 * @descreption seqDB-27523:SequoiadbDatasource()方式设置userConfig(UserConfig
 *              userConfig)
 * @author Xu Mingxing
 * @date 2022/9/14
 * @updateUser
 * @updateDate
 * @updateRemark
 * @version 1.0
 */

public class Connection27523 extends SdbTestBase {
    private SequoiadbDatasource ds = null;
    private Sequoiadb db = null;
    private Sequoiadb sdb = null;
    private String csName = "cs_27523";
    private String userName = "user_27523";
    private String password = "password_27523";
    private String passwdFileName = "/password27523";
    private String passwordFilePath = null;
    private String token = "27523";

    @BeforeClass
    public void setUp() {
        db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
    }

    @Test
    public void test() throws Exception {
        // test a：不存在鉴权用户,不指定userConfig接口
        ds = SequoiadbDatasource.builder().serverAddress( SdbTestBase.coordUrl )
                .build();
        sdb = ds.getConnection();
        sdb.createCollectionSpace( csName );
        sdb.dropCollectionSpace( csName );

        // test b：不存在鉴权用户,指定userConfig接口
        // 默认创建
        UserConfig userConfig = new UserConfig();
        ds = SequoiadbDatasource.builder().serverAddress( SdbTestBase.coordUrl )
                .userConfig( userConfig ).build();
        sdb = ds.getConnection();
        sdb.createCollectionSpace( csName );
        sdb.dropCollectionSpace( csName );

        // test c：存在鉴权用户,用户配置信息正确
        // 使用用户名、密码创建
        db.createUser( userName, password );
        userConfig = new UserConfig( userName, password );
        ds = SequoiadbDatasource.builder().serverAddress( SdbTestBase.coordUrl )
                .userConfig( userConfig ).build();
        sdb = ds.getConnection();
        sdb.createCollectionSpace( csName );
        sdb.dropCollectionSpace( csName );

        // 问题单SEQUOIADBMAINSTREAM-6568未合入7.0
        /*
         * // 使用用户名、密码文件创建 String toolsPath = Util.getSdbInstallDir() + "/bin/";
         * Util.createPasswdFile( userName, password, passwdFileName );
         * Util.downLoadFileToLocal( SdbTestBase.workDir, toolsPath +
         * passwdFileName ); passwordFilePath = SdbTestBase.workDir +
         * passwdFileName; userConfig = new UserConfig( userName, new File(
         * passwordFilePath ) ); ds =
         * SequoiadbDatasource.builder().serverAddress( SdbTestBase.coordUrl )
         * .userConfig( userConfig ).build(); sdb = ds.getConnection();
         * sdb.createCollectionSpace( csName ); sdb.dropCollectionSpace( csName
         * ); // 使用用户名、密码文件、token创建 Util.createPasswdFile( userName, password,
         * passwdFileName, token ); Util.downLoadFileToLocal(
         * SdbTestBase.workDir, toolsPath + passwdFileName ); userConfig = new
         * UserConfig( userName, new File( passwordFilePath ), token ); ds =
         * SequoiadbDatasource.builder().serverAddress( SdbTestBase.coordUrl )
         * .userConfig( userConfig ).build(); sdb = ds.getConnection();
         * sdb.createCollectionSpace( csName ); sdb.dropCollectionSpace( csName
         * );
         */

        // test d：存在鉴权用户,用户配置信息不正确
        userConfig = new UserConfig( userName, "" );
        try {
            ds = SequoiadbDatasource.builder()
                    .serverAddress( SdbTestBase.coordUrl )
                    .userConfig( userConfig ).build();
            sdb = ds.getConnection();
            Assert.fail( "unexpect result" );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != SDBError.SDB_AUTH_AUTHORITY_FORBIDDEN
                    .getErrorCode() ) {
                throw e;
            }
        }
    }

    @AfterClass
    public void tearDown() throws Exception {
        // new File( passwordFilePath ).deleteOnExit();
        // Util.removePasswdFile(
        // Util.getSdbInstallDir() + "/bin" + passwdFileName );
        db.removeUser( userName, password );
        if ( ds != null ) {
            ds.close();
        }
        if ( db != null ) {
            db.close();
        }
        if ( sdb != null ) {
            sdb.close();
        }
    }
}
