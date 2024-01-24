package com.sequoiadb.rbac;

import com.sequoiadb.base.Domain;
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

import java.util.ArrayList;
import java.util.List;

/**
 * @Description seqDB-33060:角色Resource为集群，Actions指定domain对象支持操作
 *              seqDB-33064:角色AnyResource为集群，Actions指定domain对象支持操作
 * @Author liuli
 * @Date 2023.08.30
 * @UpdateAuthor liuli
 * @UpdateDate 2023.08.30
 * @version 1.10
 */
@Test(groups = "rbac")
public class Rbac33060_33064 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String user = "user_33060";
    private String password = "passwd_33060";
    private String roleName = "role_33060";
    private String domainName = "domain_33060";
    private List< String > groupNames = new ArrayList<>();

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, SdbTestBase.rootUserName,
                SdbTestBase.rootUserPassword );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }
        groupNames = CommLib.getDataGroupNames( sdb );
        if ( groupNames.size() < 3 ) {
            throw new SkipException( "no group skip testcase" );
        }
        if ( sdb.isDomainExist( domainName ) ) {
            sdb.dropDomain( domainName );
        }
        RbacUtils.dropRole( sdb, roleName );

        sdb.createDomain( domainName,
                new BasicBSONObject( "Groups", groupNames ) );
    }

    @Test
    public void test() throws Exception {
        // seqDB-33060:角色Resource为集群，Actions指定seq对象支持操作
        testAccessControl( sdb, "Cluster" );
        // seqDB-33063:角色AnyResource为集群，Actions指定seq对象支持操作
        testAccessControl( sdb, "AnyResource" );
    }

    @AfterClass
    public void tearDown() {
        try {
            RbacUtils.removeUser( sdb, user, password );
            if ( sdb.isDomainExist( domainName ) ) {
                sdb.dropDomain( domainName );
            }
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    private void testAccessControl( Sequoiadb sdb, String resource ) {
        String roleStr = "{Role:'" + roleName + "',Privileges:[{Resource:{ "
                + resource
                + ":true}, Actions: ['alterDomain','getDomain'] }] }";
        BSONObject role = ( BSONObject ) JSON.parse( roleStr );
        sdb.createRole( role );
        sdb.createUser( user, password,
                ( BSONObject ) JSON.parse( "{Roles:['" + roleName + "']}" ) );
        try ( Sequoiadb userSdb = new Sequoiadb( SdbTestBase.coordUrl, user,
                password )) {
            Domain domain = userSdb.getDomain( domainName );
            RbacUtils.alterDomainActionSupportCommand( sdb, userSdb, domain,
                    groupNames, true );
        } finally {
            RbacUtils.removeUser( sdb, user, password );
            RbacUtils.dropRole( sdb, roleName );
        }
    }
}