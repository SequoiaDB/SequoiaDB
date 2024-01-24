package com.sequoiadb.rbac.serial;

import com.sequoiadb.base.Node;
import com.sequoiadb.base.ReplicaGroup;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.util.JSON;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.rbac.RbacUtils;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;

import java.util.List;

/**
 * @version 1.10
 * @Description seqDB-33345:创建角色指定Resource为cluster，Actions分别指定为createRG、createNode
 *              、alterRG、alterNode
 * @Author wangxingming
 * @Date 2023.09.12
 * @UpdateAuthor wangxingming
 * @UpdateDate 2023.09.12
 */
@Test(groups = "rbac")
public class Rbac33345 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String user = "user_33345";
    private String password = "passwd_33345";
    private String roleName = "role_33345";
    private String groupName = "group_33345";

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, SdbTestBase.rootUserName,
                SdbTestBase.rootUserPassword );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }

        RbacUtils.removeUser( sdb, user, password );
        RbacUtils.dropRole( sdb, roleName );
    }

    @Test
    public void test() throws Exception {
        testAccessCreateNode( sdb );
        testAccessSetLocation( sdb );
        testAccessSetActiveLocation( sdb );
    }

    @AfterClass
    public void tearDown() {
        RbacUtils.removeUser( sdb, user, password );
        RbacUtils.dropRole( sdb, roleName );
        if ( sdb != null ) {
            sdb.close();
        }
    }

    private void testAccessCreateNode( Sequoiadb sdb ) {
        List< String > groupNames = CommLib.getDataGroupNames( sdb );
        ReplicaGroup rootGroup = sdb.getReplicaGroup( groupNames.get( 0 ) );
        String hostName = rootGroup.getMaster().getHostName();
        int port = SdbTestBase.reservedPortBegin + 10;
        String dataPath = SdbTestBase.reservedDir + "/data/" + port;
        String roleStr = "{Role:'" + roleName
                + "',Privileges:[{Resource:{Cluster:true}, Actions: ['createRG', 'createNode'] }] }";
        BSONObject role = ( BSONObject ) JSON.parse( roleStr );
        sdb.createRole( role );
        sdb.createUser( user, password,
                ( BSONObject ) JSON.parse( "{Roles:['" + roleName + "']}" ) );
        try ( Sequoiadb userSdb = new Sequoiadb( SdbTestBase.coordUrl, user,
                password )) {
            ReplicaGroup replicaGroup = userSdb.createReplicaGroup( groupName );
            replicaGroup.createNode( hostName, port, dataPath );
        } finally {
            RbacUtils.removeUser( sdb, user, password );
            RbacUtils.dropRole( sdb, roleName );
            sdb.getReplicaGroup( groupName ).removeNode( hostName, port,
                    new BasicBSONObject( "Enforced", true ) );
            sdb.removeReplicaGroup( groupName );
        }
    }

    private void testAccessSetLocation( Sequoiadb sdb ) {
        List< String > groupNames = CommLib.getDataGroupNames( sdb );
        ReplicaGroup rootGroup = sdb.getReplicaGroup( groupNames.get( 0 ) );
        String hostName = rootGroup.getMaster().getHostName();
        int port = SdbTestBase.reservedPortBegin + 10;
        String dataPath = SdbTestBase.reservedDir + "/data/" + port;
        String roleStr = "{Role:'" + roleName
                + "',Privileges:[{Resource:{Cluster:true}, Actions: ['createRG', 'createNode', 'alterNode'] }] }";
        BSONObject role = ( BSONObject ) JSON.parse( roleStr );
        sdb.createRole( role );
        sdb.createUser( user, password,
                ( BSONObject ) JSON.parse( "{Roles:['" + roleName + "']}" ) );
        try ( Sequoiadb userSdb = new Sequoiadb( SdbTestBase.coordUrl, user,
                password )) {
            ReplicaGroup replicaGroup = userSdb.createReplicaGroup( groupName );
            Node node = replicaGroup.createNode( hostName, port, dataPath );
            node.setLocation( "GuangZhou" );
            node.setLocation( "" );
        } finally {
            RbacUtils.removeUser( sdb, user, password );
            RbacUtils.dropRole( sdb, roleName );
            sdb.getReplicaGroup( groupName ).removeNode( hostName, port,
                    new BasicBSONObject( "Enforced", true ) );
            sdb.removeReplicaGroup( groupName );
        }
    }

    private void testAccessSetActiveLocation( Sequoiadb sdb ) {
        List< String > groupNames = CommLib.getDataGroupNames( sdb );
        ReplicaGroup rootGroup = sdb.getReplicaGroup( groupNames.get( 0 ) );
        String hostName = rootGroup.getMaster().getHostName();
        int port = SdbTestBase.reservedPortBegin + 10;
        String dataPath = SdbTestBase.reservedDir + "/data/" + port;
        String roleStr = "{Role:'" + roleName
                + "',Privileges:[{Resource:{Cluster:true}, Actions: ['createRG', 'createNode', 'alterNode', 'alterRG'] }] }";
        BSONObject role = ( BSONObject ) JSON.parse( roleStr );
        sdb.createRole( role );
        sdb.createUser( user, password,
                ( BSONObject ) JSON.parse( "{Roles:['" + roleName + "']}" ) );
        try ( Sequoiadb userSdb = new Sequoiadb( SdbTestBase.coordUrl, user,
                password )) {
            ReplicaGroup replicaGroup = userSdb.createReplicaGroup( groupName );
            Node node = replicaGroup.createNode( hostName, port, dataPath );
            node.setLocation( "GuangZhou" );
            replicaGroup.setActiveLocation( "GuangZhou" );
            replicaGroup.setActiveLocation( "" );
        } finally {
            RbacUtils.removeUser( sdb, user, password );
            RbacUtils.dropRole( sdb, roleName );
            sdb.getReplicaGroup( groupName ).removeNode( hostName, port,
                    new BasicBSONObject( "Enforced", true ) );
            sdb.removeReplicaGroup( groupName );
        }
    }
}
