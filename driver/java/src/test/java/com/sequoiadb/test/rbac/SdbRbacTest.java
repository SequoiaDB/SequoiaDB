package com.sequoiadb.test.rbac;

import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.test.TestConfig;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;

import java.util.*;

import static org.junit.Assert.*;

public class SdbRbacTest {

    private Sequoiadb db;
    private final String roleName1 = "test_role_1";
    private final String roleName2 = "test_role_2";
    private final String roleName3 = "test_role_3";
    private final String username1 = "test_user_1";
    private final String username2 = "test_user_2";

    private final Privilege privilege1 = Privilege.builder()
            .resource("test", "test")
            .addActions("find", "insert")
            .build();

    private final Privilege privilege2 = Privilege.builder()
            .resource("test2", "test2")
            .addActions("update")
            .build();

    private final Role role1 = Role.builder()
            .role(roleName1)
            .addPrivilege(privilege1)
            .build();

    private final Role role2 = Role.builder()
            .role(roleName2)
            .addPrivilege(privilege2)
            .build();

    @Before
    public void setUp() {
        db = new Sequoiadb(
                TestConfig.getRbacCoordHost(),
                TestConfig.getRbacCoordPort(),
                TestConfig.getRbacRootUsername(),
                TestConfig.getRbacRootPassword()
        );

        createRoleIfNotExists(role1);
        createRoleIfNotExists(role2);
        createNewUser(username1);
    }

    @After
    public void tearDown() {
        dropRoleIfExists(roleName1);
        dropRoleIfExists(roleName2);
        dropRoleIfExists(roleName3);
        dropUserIfExists(username1);
        dropUserIfExists(username2);
        db.close();
    }

    @Test
    public void createRoleTest() {
        // create role
        Role role3 = Role.builder()
                .role(roleName3)
                .addPrivilege(privilege2)
                .build();
        db.createRole(role3.toBson());
        assertExistsRole(roleName3);

        // Invalid args
        Role roleObj3 = Role.builder()
                .role(null)
                .addPrivilege(privilege1)
                .build();
        try {
            db.createRole(roleObj3.toBson());
            fail();
        } catch (BaseException e) {
            assertEquals(SDBError.SDB_INVALIDARG.getErrorCode(), e.getErrorCode());
        }

        try {
            db.createRole(null);
            fail();
        } catch (BaseException e) {
            assertEquals(SDBError.SDB_INVALIDARG.getErrorCode(), e.getErrorCode());
        }

        try {
            db.createRole(new BasicBSONObject());
            fail();
        } catch (BaseException e) {
            assertEquals(SDBError.SDB_INVALIDARG.getErrorCode(), e.getErrorCode());
        }
    }

    @Test
    public void dropRoleTest() {
        // drop exists role
        db.dropRole(roleName1);
        assertNotExistsRole(roleName1);
        assertExistsRole(roleName2);

        // Invalid args
        try {
            db.dropRole(null);
            fail();
        } catch (BaseException e) {
            assertEquals(SDBError.SDB_INVALIDARG.getErrorCode(), e.getErrorCode());
        }
    }

    @Test
    public void getRoleTest() {
        // get exist role
        BSONObject roleBson = db.getRole(roleName1, new BasicBSONObject("ShowPrivileges", true));
        assertEquals(roleName1, roleBson.get("Role"));
        assertEquals(Collections.singletonList(privilege1.toBson()), roleBson.get("Privileges"));

        // Invalid args
        try {
            db.getRole(null, new BasicBSONObject("ShowPrivileges", true));
            fail();
        } catch (BaseException e) {
            assertEquals(SDBError.SDB_INVALIDARG.getErrorCode(), e.getErrorCode());
        }
    }

    @Test
    public void listRoleTest() {
        boolean hasRoleName1 = false;
        boolean hasRoleName2 = false;
        BasicBSONObject listRoleOptions = new BasicBSONObject();
        listRoleOptions.put("ShowPrivileges", true);
        listRoleOptions.put("ShowBuiltinRoles", true);
        try (DBCursor cursor = db.listRoles(listRoleOptions)) {
            while (cursor.hasNext()) {
                BSONObject next = cursor.getNext();
                String roleName = (String) next.get("Role");
                Object privileges = next.get("Privileges");
                if (roleName.equals(roleName1)) {
                    assertEquals(Collections.singletonList(privilege1.toBson()), privileges);
                    hasRoleName1 = true;
                } else if (roleName.equals(roleName2)) {
                    assertEquals(Collections.singletonList(privilege2.toBson()), privileges);
                    hasRoleName2 = true;
                }
            }
        }

        assertTrue(hasRoleName1);
        assertTrue(hasRoleName2);
    }

    @Test
    public void updateRoleTest() {
        Role newRole1 = Role.builder()
                .role(roleName1)
                .addPrivilege(privilege2)
                .build();
        db.updateRole(roleName1, newRole1.toBson());

        BSONObject roleBson = db.getRole(roleName1, new BasicBSONObject("ShowPrivileges", true));
        assertEquals(roleName1, roleBson.get("Role"));
        assertEquals(Collections.singletonList(privilege2.toBson()), roleBson.get("Privileges"));

        // invalid args
        try {
            db.updateRole(null, newRole1.toBson());
            fail();
        } catch (BaseException e) {
            assertEquals(SDBError.SDB_INVALIDARG.getErrorCode(), e.getErrorCode());
        }
    }

    @Test
    public void grantPrivilegesToRoleTest() {
        BasicBSONList privileges = new BasicBSONList();
        privileges.add(privilege2.toBson());
        db.grantPrivilegesToRole(roleName1, privileges);

        BSONObject roleBson = db.getRole(roleName1, new BasicBSONObject("ShowPrivileges", true));

        BasicBSONList basicBSONList = (BasicBSONList) roleBson.get("Privileges");
        assertTrue(basicBSONList.contains(privilege2.toBson()));

        // invalid args
        try {
            db.grantPrivilegesToRole(null, privileges);
            fail();
        } catch (BaseException e) {
            assertEquals(SDBError.SDB_INVALIDARG.getErrorCode(), e.getErrorCode());
        }
    }

    @Test
    public void revokePrivilegesFromRoleTest() {
        BasicBSONList privileges = new BasicBSONList();
        privileges.add(privilege1.toBson());
        db.revokePrivilegesFromRole(roleName1, privileges);

        BSONObject roleBson = db.getRole(roleName1, new BasicBSONObject("ShowPrivileges", true));

        BasicBSONList basicBSONList = (BasicBSONList) roleBson.get("Privileges");
        assertTrue(basicBSONList.isEmpty());

        // invalid args
        try {
            db.revokePrivilegesFromRole(null, privileges);
            fail();
        } catch (BaseException e) {
            assertEquals(SDBError.SDB_INVALIDARG.getErrorCode(), e.getErrorCode());
        }
    }

    @Test
    public void grantRolesToRoleTest() {
        BasicBSONList roles = new BasicBSONList();
        roles.add(roleName2);
        db.grantRolesToRole(roleName1, roles);

        BSONObject roleBson = db.getRole(roleName1, new BasicBSONObject());
        BasicBSONList basicBSONList = (BasicBSONList) roleBson.get("Roles");
        assertEquals(roles, basicBSONList);

        // invalid args
        try {
            db.grantRolesToRole(roleName1, null);
            fail();
        } catch (BaseException e) {
            assertEquals(SDBError.SDB_INVALIDARG.getErrorCode(), e.getErrorCode());
        }
    }

    @Test
    public void revokeRolesFromRoleTest() {
        BasicBSONList roles = new BasicBSONList();
        roles.add(roleName2);
        db.grantRolesToRole(roleName1, roles);

        db.revokeRolesFromRole(roleName1, roles);

        BSONObject roleBson = db.getRole(roleName1, new BasicBSONObject());
        BasicBSONList basicBSONList = (BasicBSONList) roleBson.get("Roles");
        assertTrue(basicBSONList.isEmpty());

        // invalid args
        try {
            db.revokeRolesFromRole(roleName1, null);
            fail();
        } catch (BaseException e) {
            assertEquals(SDBError.SDB_INVALIDARG.getErrorCode(), e.getErrorCode());
        }

        try {
            db.revokeRolesFromRole(null, roles);
            fail();
        } catch (BaseException e) {
            assertEquals(SDBError.SDB_INVALIDARG.getErrorCode(), e.getErrorCode());
        }
    }

    @Test
    public void createUserTest() {
        BasicBSONObject options = new BasicBSONObject();
        ArrayList<Object> roles = new ArrayList<>();
        roles.add("_root");
        roles.add(roleName1);
        options.put("Roles", roles);
        db.createUser(username2, "test", options);

        BSONObject user = db.getUser(username2, new BasicBSONObject());
        assertEquals(roles, user.get("Roles"));

        // invalid args
        try {
            db.createUser(username2, null, options);
            fail();
        } catch (BaseException e) {
            assertEquals(SDBError.SDB_INVALIDARG.getErrorCode(), e.getErrorCode());
        }

        try {
            db.createUser(null, "test", options);
            fail();
        } catch (BaseException e) {
            assertEquals(SDBError.SDB_INVALIDARG.getErrorCode(), e.getErrorCode());
        }
    }

    @Test
    public void getUserTest() {
        BasicBSONList roles = new BasicBSONList();
        roles.add(roleName1);
        db.grantRolesToUser(username1, roles);

        BSONObject user1 = db.getUser(username1, new BasicBSONObject("ShowPrivileges", true));
        assertEquals(username1, user1.get("User"));
        assertEquals(Collections.singletonList(privilege1.toBson()), user1.get("InheritedPrivileges"));

        // user not exist
        try {
            db.getUser(username2, new BasicBSONObject());
        } catch (BaseException e) {
            assertEquals(SDBError.SDB_AUTH_USER_NOT_EXIST.getErrorCode(), e.getErrorCode());
        }

        // username is null
        try {
            db.getUser(null, new BasicBSONObject());
        } catch (BaseException e) {
            assertEquals(SDBError.SDB_INVALIDARG.getErrorCode(), e.getErrorCode());
        }
    }

    @Test
    public void grantRolesToUserTest() {
        BasicBSONList roles = new BasicBSONList();
        roles.add("_root");
        roles.add(roleName1);
        db.grantRolesToUser(username1, roles);

        BSONObject user = db.getUser(username1, new BasicBSONObject());
        assertEquals(roles, user.get("Roles"));

        // invalid args
        try {
            db.grantRolesToUser(null, roles);
        } catch (BaseException e) {
            assertEquals(SDBError.SDB_INVALIDARG.getErrorCode(), e.getErrorCode());
        }

        try {
            db.grantRolesToUser(username1, null);
        } catch (BaseException e) {
            assertEquals(SDBError.SDB_INVALIDARG.getErrorCode(), e.getErrorCode());
        }

        try {
            db.grantRolesToUser(username1, new BasicBSONObject());
        } catch (BaseException e) {
            assertEquals(SDBError.SDB_INVALIDARG.getErrorCode(), e.getErrorCode());
        }
    }

    @Test
    public void revokeRolesFromUserTest() {
        BasicBSONList roles = new BasicBSONList();
        roles.add("_root");
        roles.add(roleName1);
        db.grantRolesToUser(username1, roles);

        BasicBSONList revokeRoles = new BasicBSONList();
        revokeRoles.add(roleName1);
        db.revokeRolesFromUser(username1, revokeRoles);

        BSONObject user = db.getUser(username1, new BasicBSONObject());
        List<String> userRoles = new ArrayList<>();
        userRoles.add("_root");
        assertEquals(userRoles, user.get("Roles"));

        // invalid args
        try {
            db.revokeRolesFromUser(null, roles);
        } catch (BaseException e) {
            assertEquals(SDBError.SDB_INVALIDARG.getErrorCode(), e.getErrorCode());
        }

        try {
            db.revokeRolesFromUser(username1, null);
        } catch (BaseException e) {
            assertEquals(SDBError.SDB_INVALIDARG.getErrorCode(), e.getErrorCode());
        }

        try {
            db.revokeRolesFromUser(username1, new BasicBSONObject());
        } catch (BaseException e) {
            assertEquals(SDBError.SDB_INVALIDARG.getErrorCode(), e.getErrorCode());
        }
    }

    @Test
    public void invalidateUserCacheTest() {
        // Clear all user caches on all nodes
        db.invalidateUserCache();
        db.invalidateUserCache("", null);
        db.invalidateUserCache("", new BasicBSONObject());
        db.invalidateUserCache(null, new BasicBSONObject());

        // Invalidate user cache by group
        db.invalidateUserCache(username1, new BasicBSONObject("Group", TestConfig.getDataGroupName()));
    }

    private void createRoleIfNotExists(Role role) {
        try {
            db.createRole(role.toBson());
        } catch (BaseException e) {
        }
    }

    private void dropRoleIfExists(String roleName) {
        try {
            db.dropRole(roleName);
        } catch (BaseException e) {
        }
    }

    private void createNewUser(String username) {
        try {
            db.createUser(username, "test", new BasicBSONObject());
        } catch (BaseException e) {
            if (e.getErrorCode() == SDBError.SDB_AUTH_USER_ALREADY_EXIST.getErrorCode()) {
                db.removeUser(username, "test");
                db.createUser(username, "test", new BasicBSONObject());
                return;
            }
            throw e;
        }
    }

    private void dropUserIfExists(String username) {
        try {
            db.removeUser(username, "test");
        } catch (BaseException e) {
            if (e.getErrorCode() != SDBError.SDB_AUTH_USER_NOT_EXIST.getErrorCode()) {
                throw e;
            }
        }
    }

    private void assertExistsRole(String roleName) {
        assertTrue(listRoleName().contains(roleName));
    }

    private void assertNotExistsRole(String roleName) {
        assertFalse(listRoleName().contains(roleName));
    }

    private Set<String> listRoleName() {
        HashSet<String> set = new HashSet<>();
        BasicBSONObject listRoleOptions = new BasicBSONObject();
        listRoleOptions.put("ShowPrivileges", true);
        listRoleOptions.put("ShowBuiltinRoles", true);
        try (DBCursor cursor = db.listRoles(listRoleOptions)) {
            while (cursor.hasNext()) {
                BSONObject next = cursor.getNext();
                set.add((String) next.get("Role"));
            }
        }
        return set;
    }

}
