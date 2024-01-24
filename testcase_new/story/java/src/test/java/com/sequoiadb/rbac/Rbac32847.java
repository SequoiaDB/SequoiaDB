package com.sequoiadb.rbac;

import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @Description seqDB-32847:角色撤销多个权限
 * @Author tangtao
 * @Date 2023.08.30
 * @UpdateAuthor tangtao
 * @UpdateDate 2023.08.30
 * @version 1.0
 */
@Test(groups = "rbac")
public class Rbac32847 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String user = "user_32847";
    private String password = "passwd_32847";
    private String roleName = "role_32847";
    private String csName = "cs_32847";
    private String clName = "cl_32847";

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, SdbTestBase.rootUserName,
                SdbTestBase.rootUserPassword );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }

        RbacUtils.dropRole( sdb, roleName );

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
            sdb.dropCollectionSpace( csName );
            sdb.removeUser( user, password );
            RbacUtils.dropRole( sdb, roleName );
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    private void testAccessControl( Sequoiadb sdb ) {
        String[] actions1 = { "testCS", "testCL", "find", "insert", "update",
                "remove" };
        String[] grantActions = { "createIndex", "dropIndex", "truncate" };
        String[] revokeActions = { "insert", "update", "remove" };

        // 创建角色
        String action = RbacUtils.arrayToCommaSeparatedString( actions1 );
        String roleStr = "{Role:'" + roleName + "',Privileges:[{Resource:{ cs:'"
                + csName + "',cl:''}, Actions: [" + action + "] }] }";
        BSONObject role1 = ( BSONObject ) JSON.parse( roleStr );
        sdb.createRole( role1 );

        // 使用角色创建用户
        sdb.createUser( user, password,
                ( BSONObject ) JSON.parse( "{Roles:['" + roleName + "']}" ) );
        try ( Sequoiadb userSdb = new Sequoiadb( SdbTestBase.coordUrl, user,
                password )) {

            CollectionSpace userCS = userSdb.getCollectionSpace( csName );
            DBCollection userCL = userCS.getCollection( clName );

            RbacUtils.findActionSupportCommand( sdb, csName, clName, userCL,
                    false );
            RbacUtils.insertActionSupportCommand( sdb, csName, clName, userCL,
                    false );

            try {
                RbacUtils.createIndexActionSupportCommand( sdb, csName, clName,
                        userCL, false );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }

            // 赋予创建/删除索引权限
            action = RbacUtils.arrayToCommaSeparatedString( grantActions );
            BasicBSONList privilegeList = new BasicBSONList();
            String priStr = "{ Resource:{ cs:'" + csName
                    + "',cl:''}, Actions: [" + action + "] }";
            BSONObject grantAct = ( BSONObject ) JSON.parse( priStr );
            privilegeList.add( grantAct );
            sdb.grantPrivilegesToRole( roleName, privilegeList );

            // 回收增删改操作权限
            action = RbacUtils.arrayToCommaSeparatedString( revokeActions );
            privilegeList = new BasicBSONList();
            priStr = "{ Resource:{ cs:'" + csName + "',cl:''}, Actions: ["
                    + action + "] }";
            BSONObject revokeAct = ( BSONObject ) JSON.parse( priStr );
            privilegeList.add( revokeAct );
            sdb.revokePrivilegesFromRole( roleName, privilegeList );

            RbacUtils.findActionSupportCommand( sdb, csName, clName, userCL,
                    false );
            try {
                RbacUtils.insertActionSupportCommand( sdb, csName, clName,
                        userCL, false );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }

            RbacUtils.createIndexActionSupportCommand( sdb, csName, clName,
                    userCL, false );

            RbacUtils.dropIndexActionSupportCommand( sdb, csName, clName,
                    userCL, false );

        }
    }
}