import("../lib/basic_operation/commlib.js");
import("../lib/main.js");

function ensurePrivilegeCheckEnabled(testPara, f) {
  var cursor = db.snapshot(
    SDB_SNAP_CONFIGS,
    { SvcName: COORDSVCNAME },
    { privilegecheck: 1 }
  );
  var privilegeCheckEnabled = JSON.parse(cursor.next()).privilegecheck;
  if (privilegeCheckEnabled == "FALSE") {
    try {
      db.updateConf({ privilegecheck: true }, { SvcName: COORDSVCNAME });
    } catch (e) {
      if (e != SDB_RTN_CONF_NOT_TAKE_EFFECT) {
        throw e;
      } else {
        var oma = Oma();
        oma.stopNode(COORDSVCNAME);
        oma.startNode(COORDSVCNAME);
        db = new Sdb(COORDHOSTNAME, COORDSVCNAME);
      }
    }
  }
  try {
    f(testPara);
  } finally {
    if (privilegeCheckEnabled == "FALSE") {
      try {
        db.updateConf({ privilegecheck: false }, { SvcName: COORDSVCNAME });
      } catch (e) {
        if (e != SDB_RTN_CONF_NOT_TAKE_EFFECT) {
          throw e;
        } else {
          var oma = Oma();
          oma.stopNode(COORDSVCNAME);
          oma.startNode(COORDSVCNAME);
          db = new Sdb(COORDHOSTNAME, COORDSVCNAME);
        }
      }
    }
  }
}

function ignoreError(f) {
  try {
    f();
  } catch (e) {}
}

var test_role_ac_name = "test_role_ac";
var test_role_common_name = "test_role_common";
var test_su_name = "su";
var test_user_name = "test_user";
var pwd = "123";

function cleanTestAccessControl(db) {
  ignoreError(function () {
    db.dropUsr(test_user_name, pwd);
  });
  ignoreError(function () {
    db.dropRole(test_role_common_name);
  });
  ignoreError(function () {
    db.dropRole(test_role_ac_name);
  });
  ignoreError(function () {
    db.dropUsr(test_su_name, pwd);
  });
}

function prepareTestAccessControl(db, test_privileges, cmd_privileges) {
  cleanTestAccessControl(db);
  db.createUsr(test_su_name, pwd, { Roles: ["_root"] });
  db.createRole({
    Role: test_role_common_name,
    Privileges: test_privileges,
  });
  db.createRole({ Role: test_role_ac_name, Privileges: cmd_privileges });
  db.createUsr(test_user_name, pwd, { Roles: [test_role_common_name] });
  return new Sdb(COORDHOSTNAME, COORDSVCNAME, test_user_name, pwd);
}

function checkAccessControl(db, test_privileges, cmd_privileges, f) {
  var user = prepareTestAccessControl(db, test_privileges, cmd_privileges);
  assert.tryThrow(SDB_NO_PRIVILEGES, function () {
    f(user);
  });
  db.grantRolesToUser(test_user_name, [test_role_ac_name]);
  db.invalidateUserCache(test_user_name);
  f(user);
  cleanTestAccessControl(db);
}

var test_role1_name = "test_role1";
var test_role2_name = "test_role2";
var test_role3_name = "test_role3";

var test_role1 = {
  Role: test_role1_name,
  Privileges: [
    {
      Resource: { cs: "test_rbac", cl: "test_rbac" },
      Actions: ["find"],
    },
  ],
  Roles: [],
};

var test_role2 = {
  Role: test_role2_name,
  Privileges: [
    {
      Resource: { cs: "test_rbac", cl: "test_rbac" },
      Actions: ["insert", "update"],
    },
  ],
  Roles: [test_role1_name],
};

var test_role3 = {
  Role: test_role3_name,
  Privileges: [
    {
      Resource: { cs: "test_rbac", cl: "test_rbac" },
      Actions: ["remove"],
    },
  ],
  Roles: [test_role2_name],
};

function cleanTestRoles(db) {
  try {
    db.dropRole(test_role1_name);
  } catch (e) {}
  try {
    db.dropRole(test_role2_name);
  } catch (e) {}
  try {
    db.dropRole(test_role3_name);
  } catch (e) {}
}

function prepareTestRoles(db) {
  cleanTestRoles(db);
  db.createRole(test_role1);
  db.createRole(test_role2);
  db.createRole(test_role3);
}

function checkRolesOfRole(role, expectedRoles, expectedInheritedRoles) {
  assert.equal(role.Roles.sort(), expectedRoles.sort());
  assert.equal(role.InheritedRoles.sort(), expectedInheritedRoles.sort());
}

function checkRolesOfRoleByName(
  db,
  roleName,
  expectedRoles,
  expectedInheritedRoles
) {
  checkRolesOfRole(
    JSON.parse(db.getRole(roleName)),
    expectedRoles,
    expectedInheritedRoles
  );
}

function checkPrivilegesOfRole(role, expPrivileges, expInheritedPrivileges) {
  assert.equal(role.Privileges.sort(), expPrivileges.sort());
  assert.equal(role.InheritedPrivileges.sort(), expInheritedPrivileges.sort());
}

function checkPrivilegesOfRoleByName(
  db,
  roleName,
  expPrivileges,
  expInheritedPrivileges
) {
  checkPrivilegesOfRole(
    JSON.parse(db.getRole(roleName, { ShowPrivileges: true })),
    expPrivileges,
    expInheritedPrivileges
  );
}
