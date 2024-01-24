package com.sequoiadb.rbac;

import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @Description seqDB-32817:创建用户使用内建角色，指定为_clusterMonitor
 * @Author liuli
 * @Date 2023.08.29
 * @UpdateAuthor liuli
 * @UpdateDate 2023.08.29
 * @version 1.10
 */
@Test(groups = "rbac")
public class Rbac32817 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String user = "user_32817";
    private String password = "passwd_32817";
    private String csName = "cs_32817";
    private String clName = "cl_32817";

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
                ( BSONObject ) JSON.parse( "{Roles:['_clusterMonitor']}" ) );
        try ( Sequoiadb userSdb = new Sequoiadb( SdbTestBase.coordUrl, user,
                password )) {
            RbacUtils.listActionSupportCommand( sdb, userSdb, csName, clName,
                    false );
            RbacUtils.snapshotActionSupportCommand( sdb, userSdb, csName,
                    clName, false );
            RbacUtils.getDetailBinActionSupportCommand( sdb, userSdb, csName,
                    clName, false );
            RbacUtils.getRoleActionSupportCommand( sdb, userSdb, csName, clName,
                    false );
            RbacUtils.getUserActionSupportCommand( sdb, userSdb, csName, clName,
                    false );
            RbacUtils.countBinActionSupportCommand( sdb, userSdb, csName,
                    clName, false );
            RbacUtils.listBinActionSupportCommand( sdb, userSdb, csName, clName,
                    false );
            RbacUtils.listRolesActionSupportCommand( sdb, userSdb, csName,
                    clName, false );
            RbacUtils.listCollectionSpacesActionSupportCommand( sdb, userSdb,
                    csName, clName, false );

            // 执行一些不支持的操作
            try {
                userSdb.dropCollectionSpace( csName );
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