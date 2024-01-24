package com.sequoiadb.test.rg;

import com.sequoiadb.base.ReplicaGroup;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.test.TestConfig;
import com.sequoiadb.test.rbac.Privilege;
import com.sequoiadb.test.rbac.Role;
import org.bson.BasicBSONObject;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;

import java.util.Collections;

import static com.sequoiadb.test.AssertUtil.*;
import static org.junit.Assert.*;

public class RbacRgTest {

    private Sequoiadb adminDb;
    private Sequoiadb userDb;

    private final String roleName = "test_role";

    private final Role role1 = Role.builder()
            .role(roleName)
            .addPrivilege(
                    Privilege.builder()
                            .resource(true)
                            .addActions("createRG")
                            .build()
            )
            .build();

    private final Role role3 = Role.builder()
            .role(roleName)
            .addPrivilege(
                    Privilege.builder()
                            .resource(true)
                            .addActions("createRG", "list", "createNode", "removeNode")
                            .build()
            )
            .build();

    private final String username = "test_user";

    private final String password = "test";

    @Before
    public void setUp() {
        adminDb = new Sequoiadb(
                TestConfig.getRbacCoordHost(),
                TestConfig.getRbacCoordPort(),
                TestConfig.getRbacRootUsername(),
                TestConfig.getRbacRootPassword()
        );

        adminDb.createRole(role1.toBson());

        BasicBSONObject options = new BasicBSONObject("Roles", Collections.singleton(role1.getRole()));
        adminDb.createUser(username, password, options);

        userDb = new Sequoiadb(
                TestConfig.getRbacCoordHost(),
                TestConfig.getRbacCoordPort(),
                username,
                password
        );
    }

    @After
    public void tearDown() {
        adminDb.removeUser(username, password);
        adminDb.dropRole(role1.getRole());
        userDb.close();
        adminDb.close();
    }

    @Test
    public void onlyNeedCreateRGPrivilegesTest() {
        String newGroupName = "group_test";
        try {
            ReplicaGroup replicaGroup = userDb.createReplicaGroup(newGroupName);

            assertNotThrows(replicaGroup::getId);
            assertEquals(newGroupName, replicaGroup.getGroupName());
            assertFalse(replicaGroup.isCatalog());
        } finally {
            adminDb.removeReplicaGroup(newGroupName);
        }
    }

    @Test
    public void needListPrivilegesTest() {
        String newGroupName = "group_test";
        try {
            ReplicaGroup replicaGroup = userDb.createReplicaGroup(newGroupName);

            assertSDBError(SDBError.SDB_NO_PRIVILEGES, replicaGroup::getDetail);
            assertSDBError(SDBError.SDB_NO_PRIVILEGES, replicaGroup::getMaster);
            assertSDBError(SDBError.SDB_NO_PRIVILEGES, replicaGroup::getSlave);
            assertSDBError(SDBError.SDB_NO_PRIVILEGES, () -> replicaGroup.getNode(TestConfig.getRbacCoordHost(), 12910));

            adminDb.updateRole(roleName, role3.toBson());
            replicaGroup.createNode(
                    TestConfig.getRbacCoordHost(),
                    12930,
                    TestConfig.getRbacNewNodeDbPathPrefix() + "/12930"
            );

            assertNotThrows(replicaGroup::getDetail);
            assertNotSDBError(SDBError.SDB_NO_PRIVILEGES, replicaGroup::getMaster);
            assertNotThrows(replicaGroup::getSlave);
            assertNotThrows(() -> replicaGroup.getNode(TestConfig.getRbacCoordHost(), 12930));
        } finally {
            adminDb.removeReplicaGroup(newGroupName);
        }
    }

    @Test
    public void needNodePrivilegesTest() {
        String newGroupName = "group_test";
        try {
            ReplicaGroup replicaGroup = userDb.createReplicaGroup(newGroupName);

            assertSDBError(SDBError.SDB_NO_PRIVILEGES, () -> replicaGroup.createNode(
                    TestConfig.getRbacCoordHost(),
                    13920,
                    TestConfig.getRbacNewNodeDbPathPrefix() + "/13920"
            ));
            assertSDBError(SDBError.SDB_NO_PRIVILEGES, () -> replicaGroup.detachNode(
                    TestConfig.getRbacCoordHost(),
                    13920,
                    new BasicBSONObject()
            ));
            assertSDBError(SDBError.SDB_NO_PRIVILEGES, () -> replicaGroup.attachNode(
                    TestConfig.getRbacCoordHost(),
                    13920,
                    new BasicBSONObject()
            ));

            adminDb.updateRole(roleName, role3.toBson());
            replicaGroup.createNode(
                    TestConfig.getRbacCoordHost(),
                    13920,
                    TestConfig.getRbacNewNodeDbPathPrefix() + "/13920"
            );
            assertNotSDBError(SDBError.SDB_NO_PRIVILEGES, () -> replicaGroup.detachNode(
                    TestConfig.getRbacCoordHost(),
                    13920,
                    new BasicBSONObject("KeepData", false)
            ));
            assertNotSDBError(SDBError.SDB_NO_PRIVILEGES, () -> replicaGroup.attachNode(
                    TestConfig.getRbacCoordHost(),
                    13920,
                    new BasicBSONObject("KeepData", false)
            ));
        } finally {
            adminDb.removeReplicaGroup(newGroupName);
        }
    }

}
