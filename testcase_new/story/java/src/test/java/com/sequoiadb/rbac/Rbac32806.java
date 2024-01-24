package com.sequoiadb.rbac;

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

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @Description seqDB-32806:创建角色存在多层继承关系
 * @Author tangtao
 * @Date 2023.08.29
 * @UpdateAuthor tangtao
 * @UpdateDate 2023.08.29
 * @version 1.0
 */
@Test(groups = "rbac")
public class Rbac32806 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String user = "user_32806";
    private String password = "passwd_32806";
    private String hierarchicRoleName1 = "role_32806_1";
    private String hierarchicRoleName2 = "role_32806_2";
    private String hierarchicRoleName3 = "role_32806_3";
    private String csName = "cs_32806";
    private String clName = "cl_32806";

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, SdbTestBase.rootUserName,
                SdbTestBase.rootUserPassword );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }

        RbacUtils.dropRole( sdb, hierarchicRoleName1 );
        RbacUtils.dropRole( sdb, hierarchicRoleName2 );
        RbacUtils.dropRole( sdb, hierarchicRoleName3 );

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
            RbacUtils.dropRole( sdb, hierarchicRoleName1 );
            RbacUtils.dropRole( sdb, hierarchicRoleName2 );
            RbacUtils.dropRole( sdb, hierarchicRoleName3 );
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    private void testAccessControl( Sequoiadb sdb ) {
        String[] actions1 = { "testCS", "testCL" };
        String[] actions2 = { "find", "insert", "update", "remove" };
        String[] actions3 = { "find", "getDetail", "createIndex", "dropIndex" };

        // 创建一级角色
        String action = RbacUtils.arrayToCommaSeparatedString( actions1 );
        String roleStr = "{Role:'" + hierarchicRoleName1
                + "',Privileges:[{Resource:{ cs:'" + csName
                + "',cl:''}, Actions: [" + action + "] }] }";
        BSONObject role1 = ( BSONObject ) JSON.parse( roleStr );
        sdb.createRole( role1 );

        // 创建二级角色
        action = RbacUtils.arrayToCommaSeparatedString( actions2 );
        roleStr = "{Role:'" + hierarchicRoleName2
                + "',Privileges:[{Resource:{ cs:'" + csName
                + "',cl:''}, Actions: [" + action + "] }], Roles:['"
                + hierarchicRoleName1 + "'] }";
        BSONObject role2 = ( BSONObject ) JSON.parse( roleStr );
        sdb.createRole( role2 );

        // 创建三级角色
        action = RbacUtils.arrayToCommaSeparatedString( actions3 );
        roleStr = "{Role:'" + hierarchicRoleName3
                + "',Privileges:[{Resource:{ cs:'" + csName
                + "',cl:''}, Actions: [" + action + "] }], Roles:['"
                + hierarchicRoleName2 + "'] }";
        BSONObject role3 = ( BSONObject ) JSON.parse( roleStr );
        sdb.createRole( role3 );

        // 使用角色3创建用户
        sdb.createUser( user, password, ( BSONObject ) JSON
                .parse( "{Roles:['" + hierarchicRoleName3 + "']}" ) );
        try ( Sequoiadb userSdb = new Sequoiadb( SdbTestBase.coordUrl, user,
                password )) {
            DBCollection userCL = userSdb.getCollectionSpace( csName )
                    .getCollection( clName );

            RbacUtils.findActionSupportCommand( sdb, csName, clName, userCL,
                    false );

            RbacUtils.insertActionSupportCommand( sdb, csName, clName, userCL,
                    false );

            RbacUtils.updateActionSupportCommand( sdb, csName, clName, userCL,
                    false );

            RbacUtils.removeActionSupportCommand( sdb, csName, clName, userCL,
                    false );

            RbacUtils.getDetailActionSupportCommand( sdb, csName, clName,
                    userCL, false );

            try {
                userCL.alterCollection( new BasicBSONObject( "ReplSize", -1 ) );
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

            try {
                userCL.truncate();
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }
        }
    }
}