package com.sequoiadb.rbac;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.util.JSON;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @Description seqDB-33061:角色Resource为集群，Actions指定trans
 *              seqDB-33065:角色AnyResource为集群，Actions指定trans
 * @Author liuli
 * @Date 2023.08.30
 * @UpdateAuthor liuli
 * @UpdateDate 2023.08.30
 * @version 1.10
 */
@Test(groups = "rbac")
public class Rbac33061_33065 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String user = "user_33061";
    private String password = "passwd_33061";
    private String roleName = "role_33061";
    private String csName = "cs_33061";
    private String clName = "cl_33061";

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
        RbacUtils.dropRole( sdb, roleName );

        CollectionSpace cs = sdb.createCollectionSpace( csName );
        cs.createCollection( clName );
    }

    @Test
    public void test() throws Exception {
        // seqDB-33061:角色Resource为集群，Actions指定trans
        testAccessControl( sdb, "Cluster" );
        // seqDB-33065:角色AnyResource为集群，Actions指定trans
        testAccessControl( sdb, "AnyResource" );
    }

    @AfterClass
    public void tearDown() {
        try {
            RbacUtils.removeUser( sdb, user, password );
            if ( sdb.isCollectionSpaceExist( csName ) ) {
                sdb.dropCollectionSpace( csName );
            }
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    private void testAccessControl( Sequoiadb sdb, String resource ) {
        String roleStr = "{Role:'" + roleName + "',Privileges:[{Resource:{ "
                + resource + ":true}, Actions: ['trans'] },{Resource:{cs:'"
                + csName + "',cl:''},Actions:['insert','testCS','testCL']}] }";
        BSONObject role = ( BSONObject ) JSON.parse( roleStr );
        sdb.createRole( role );
        sdb.createUser( user, password,
                ( BSONObject ) JSON.parse( "{Roles:['" + roleName + "']}" ) );
        try ( Sequoiadb userSdb = new Sequoiadb( SdbTestBase.coordUrl, user,
                password )) {
            DBCollection userCL = userSdb.getCollectionSpace( csName )
                    .getCollection( clName );
            userSdb.beginTransaction();
            userCL.insertRecord( new BasicBSONObject( "a", 1 ) );
            userSdb.commit();
            userSdb.beginTransaction();
            userCL.insertRecord( new BasicBSONObject( "a", 2 ) );
            userSdb.rollback();
        } finally {
            RbacUtils.removeUser( sdb, user, password );
            RbacUtils.dropRole( sdb, roleName );
        }
    }
}