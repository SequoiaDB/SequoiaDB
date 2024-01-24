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
 * @Description seqDB-32802:创建角色指定继承关系，父角色包含子角色
 * @Author tangtao
 * @Date 2023.08.29
 * @UpdateAuthor tangtao
 * @UpdateDate 2023.08.29
 * @version 1.0
 */
@Test(groups = "rbac")
public class Rbac32802 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String user = "user_32802";
    private String password = "passwd_32802";
    private String mainRoleName = "mainrole_32802";
    private String subRoleName = "subrole_32802";
    private String csName = "cs_32802";
    private String clName = "cl_32802";

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
        String[] actions = { "find", "insert", "update", "remove", "getDetail",
                "alterCL", "createIndex", "dropIndex", "truncate", "testCS",
                "testCL" };
        String[] subActions = { "find", "insert", "update", "remove" };

        String action = RbacUtils.arrayToCommaSeparatedString( actions );
        // 创建父角色
        String mainRoleStr = "{Role:'" + mainRoleName
                + "',Privileges:[{Resource:{ cs:'" + csName
                + "',cl:''}, Actions: [" + action + "] }] }";
        BSONObject mainRole = ( BSONObject ) JSON.parse( mainRoleStr );
        sdb.createRole( mainRole );

        // 创建子角色，继承父角色
        String subAction = RbacUtils.arrayToCommaSeparatedString( subActions );
        String subRoleStr = "{Role:'" + subRoleName
                + "',Privileges:[{Resource:{ cs:'" + csName + "',cl:'" + clName
                + "'}, Actions: [" + subAction + "] }] ,Roles:['" + mainRoleName
                + "'] }";
        BSONObject subRole = ( BSONObject ) JSON.parse( subRoleStr );
        sdb.createRole( subRole );

        // 使用子角色创建用户
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

            RbacUtils.removeActionSupportCommand( sdb, csName, clName, userCL,
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