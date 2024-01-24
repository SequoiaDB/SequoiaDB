package com.sequoiadb.rbac.serial;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.rbac.RbacUtils;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @Description seqDB-32816:创建用户使用内建角色，指定为_clusterAdmin
 * @Author liuli
 * @Date 2023.08.30
 * @UpdateAuthor liuli
 * @UpdateDate 2023.08.30
 * @version 1.10
 */
@Test(groups = "rbac")
public class Rbac32816 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String user = "user_32816";
    private String password = "passwd_32816";
    private String roleName = "role_32816";
    private String csName = "cs_32816";
    private String clName = "cl_32816";

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
        RbacUtils.removeUser( sdb, user, password );
        if ( sdb != null ) {
            sdb.close();
        }
    }

    private void testAccessControl( Sequoiadb sdb ) {
        String groupName = "group_32816";
        sdb.createUser( user, password,
                ( BSONObject ) JSON.parse( "{Roles:['_clusterAdmin']}" ) );
        try ( Sequoiadb userSdb = new Sequoiadb( SdbTestBase.coordUrl, user,
                password )) {
            RbacUtils.createRGActionSupportCommand( sdb, userSdb, groupName,
                    false );
            RbacUtils.getRGActionSupportCommand( sdb, userSdb, false );
            RbacUtils.removeRGActionSupportCommand( sdb, userSdb, groupName,
                    false );
            RbacUtils.getNodeActionSupportCommand( sdb, userSdb, false );
            RbacUtils.createNodeActionSupportCommand( sdb, userSdb, false );
            RbacUtils.removeNodeActionSupportCommand( sdb, userSdb, false );
            RbacUtils.startRGActionSupportCommand( sdb, userSdb, false );
            RbacUtils.stopRGActionSupportCommand( sdb, userSdb, false );
            RbacUtils.listActionSupportCommand( sdb, userSdb, csName, clName,
                    false );
            RbacUtils.startNodeActionSupportCommand( sdb, userSdb, false );
            RbacUtils.stopNodeActionSupportCommand( sdb, userSdb, false );
            RbacUtils.snapshotActionSupportCommand( sdb, userSdb, csName,
                    clName, false );

            // 执行不支持的操作
            try {
                userSdb.deleteConfig(
                        new BasicBSONObject( "metacacheexpired", 1 ),
                        new BasicBSONObject() );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }

            try {
                userSdb.updateConfig(
                        new BasicBSONObject( "metacacheexpired", 30 ) );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }

            try {
                userSdb.getCollectionSpace( csName ).getCollection( clName );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }
        } finally {
            RbacUtils.removeUser( sdb, user, password );
            RbacUtils.dropRole( sdb, roleName );
        }
    }
}