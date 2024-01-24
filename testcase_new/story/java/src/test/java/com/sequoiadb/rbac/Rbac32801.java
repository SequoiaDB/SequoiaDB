package com.sequoiadb.rbac;

import org.bson.BSONObject;
import org.bson.util.JSON;
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
 * @Description seqDB-32801:创建角色指定继承关系，父角色和子角色存在交集
 * @Author tangtao
 * @Date 2023.08.29
 * @UpdateAuthor tangtao
 * @UpdateDate 2023.08.29
 * @version 1.0
 */
@Test(groups = "rbac")
public class Rbac32801 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String user = "user_32801";
    private String password = "passwd_32801";
    private String mainRoleName = "mainrole_32801";
    private String subRoleName = "subrole_32801";
    private String csName = "cs_32801";
    private String clName = "cl_32801";

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, SdbTestBase.rootUserName,
                SdbTestBase.rootUserPassword );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }

        RbacUtils.dropRole( sdb, mainRoleName );
        RbacUtils.dropRole( sdb, subRoleName );

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
            RbacUtils.dropRole( sdb, mainRoleName );
            RbacUtils.dropRole( sdb, subRoleName );
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    private void testAccessControl( Sequoiadb sdb ) {
        String[] actions1 = { "testCS", "testCL", "find", "insert", "update" };
        String[] actions2 = { "find", "insert", "getDetail", "alterCL",
                "createIndex", "dropIndex", "truncate" };

        // 创建父角色具有testCS和testCL权限，以及部分集合操作权限
        String action = RbacUtils.arrayToCommaSeparatedString( actions1 );
        String mainRoleStr = "{Role:'" + mainRoleName
                + "',Privileges:[{Resource:{ cs:'" + csName
                + "',cl:''}, Actions: [" + action + "] }] }";
        BSONObject mainRole = ( BSONObject ) JSON.parse( mainRoleStr );
        sdb.createRole( mainRole );

        // 创建子角色
        action = RbacUtils.arrayToCommaSeparatedString( actions2 );
        String roleStr = "{Role:'" + subRoleName
                + "',Privileges:[{Resource:{ cs:'" + csName
                + "',cl:''}, Actions: [" + action + "] }], Roles:['"
                + mainRoleName + "'] }";
        BSONObject role = ( BSONObject ) JSON.parse( roleStr );
        sdb.createRole( role );

        sdb.createUser( user, password, ( BSONObject ) JSON
                .parse( "{Roles:['" + subRoleName + "']}" ) );
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

            RbacUtils.getDetailActionSupportCommand( sdb, csName, clName,
                    userCL, false );

            RbacUtils.alterCLActionSupportCommand( sdb, csName, clName, userCL,
                    false );

            RbacUtils.createIndexActionSupportCommand( sdb, csName, clName,
                    userCL, false );

            RbacUtils.dropIndexActionSupportCommand( sdb, csName, clName,
                    userCL, false );

            userCL.truncate();

        }
    }

}