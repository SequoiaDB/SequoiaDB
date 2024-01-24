package com.sequoiadb.rbac.serial;

import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.rbac.RbacUtils;
import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @Description seqDB-32818:创建用户使用内建角色，指定为_backup
 * @Author liuli
 * @Date 2023.08.29
 * @UpdateAuthor liuli
 * @UpdateDate 2023.08.29
 * @version 1.10
 */
@Test(groups = "rbac")
public class Rbac32818 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String user = "user_32818";
    private String password = "passwd_32818";
    private String csName = "cs_32818";
    private String clName = "cl_32818";

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, SdbTestBase.rootUserName,
                SdbTestBase.rootUserPassword );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }
        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }

        CollectionSpace cs = sdb.createCollectionSpace( csName );
        cs.createCollection( clName );
    }

    @Test
    public void test() throws Exception {
        testAccessControl( sdb );
    }

    @AfterClass
    public void tearDown() {
        try {
            RbacUtils.removeUser( sdb, user, password );
            sdb.dropCollectionSpace( csName );
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    private void testAccessControl( Sequoiadb sdb ) {
        sdb.createUser( user, password,
                ( BSONObject ) JSON.parse( "{Roles:['_backup']}" ) );
        try ( Sequoiadb userSdb = new Sequoiadb( SdbTestBase.coordUrl, user,
                password )) {
            RbacUtils.backupActionSupportCommand( sdb, userSdb, csName, clName,
                    false );
            RbacUtils.listBackupActionSupportCommand( sdb, userSdb, csName,
                    clName, false );
            RbacUtils.removeBackupActionSupportCommand( sdb, userSdb, csName,
                    clName, false );

            // 执行一些不支持的操作
            try {
                userSdb.getCollectionSpace( csName ).getCollection( clName );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }

            try {
                userSdb.getList( Sequoiadb.SDB_LIST_USERS, null, null, null );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }
        } finally {
            sdb.removeUser( user, password );
        }
    }
}