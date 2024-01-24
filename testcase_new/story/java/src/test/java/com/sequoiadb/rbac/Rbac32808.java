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
 * @Description seqDB-32808:创建用户指定多个角色，包含"_root"角色和其他角色
 * @Author tangtao
 * @Date 2023.08.29
 * @UpdateAuthor tangtao
 * @UpdateDate 2023.08.29
 * @version 1.0
 */
@Test(groups = "rbac")
public class Rbac32808 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String user = "user_32808";
    private String password = "passwd_32808";
    private String roleName = "role_32808";
    private String csName = "cs_32808";
    private String clName = "cl_32808";

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
        String[] actions = { "find", "insert", "update", "remove" };
        String action = RbacUtils.arrayToCommaSeparatedString( actions );
        // 创建角色
        String roleStr = "{Role:'" + roleName + "',Privileges:[{Resource:{ cs:'"
                + csName + "',cl:''}, Actions: [" + action + "] }] }";
        BSONObject role = ( BSONObject ) JSON.parse( roleStr );
        sdb.createRole( role );

        // 使用_root角色与自建角色创建用户
        sdb.createUser( user, password, ( BSONObject ) JSON
                .parse( "{Roles:['_root', '" + roleName + "']}" ) );
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