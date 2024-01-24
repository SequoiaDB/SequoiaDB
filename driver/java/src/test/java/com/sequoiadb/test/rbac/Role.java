package com.sequoiadb.test.rbac;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Objects;
import java.util.stream.Collectors;

public class Role {
    private String role;
    private List<Privilege> privileges;
    private List<String> roles;

    public BSONObject toBson() {
        BasicBSONObject bson = new BasicBSONObject();
        if (role != null) {
            bson.put("Role", role);
        }
        if (privileges != null) {
            bson.put("Privileges", privileges.stream().map(Privilege::toBson).collect(Collectors.toList()));
        }
        if (roles != null) {
            bson.put("Roles", roles);
        }
        return bson;
    }

    public static RoleBuilder builder() {
        return new RoleBuilder();
    }

    public static class RoleBuilder {
        private String role;
        private List<Privilege> privileges;
        private List<String> roles;

        public RoleBuilder role(String role) {
            this.role = role;
            return this;
        }

        public RoleBuilder privileges(List<Privilege> privileges) {
            this.privileges = privileges;
            return this;
        }

        public RoleBuilder addPrivilege(Privilege... privileges) {
            if (this.privileges == null) {
                this.privileges = new ArrayList<>();
            }
            this.privileges.addAll(Arrays.asList(privileges));
            return this;
        }

        public RoleBuilder roles(List<String> roles) {
            this.roles = roles;
            return this;
        }

        public RoleBuilder addRoles(String... roles) {
            if (this.roles == null) {
                this.roles = new ArrayList<>();
            }
            this.roles.addAll(Arrays.asList(roles));
            return this;
        }

        public Role build() {
            Role roleObj = new Role();
            roleObj.setRole(role);
            roleObj.setRoles(roles);
            roleObj.setPrivileges(privileges);
            return roleObj;
        }
    }

    public String getRole() {
        return role;
    }

    public void setRole(String role) {
        this.role = role;
    }

    public List<Privilege> getPrivileges() {
        return privileges;
    }

    public void setPrivileges(List<Privilege> privileges) {
        this.privileges = privileges;
    }

    public List<String> getRoles() {
        return roles;
    }

    public void setRoles(List<String> roles) {
        this.roles = roles;
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        if (o == null || getClass() != o.getClass()) return false;
        Role role1 = (Role) o;
        return Objects.equals(role, role1.role) && Objects.equals(privileges, role1.privileges) && Objects.equals(roles, role1.roles);
    }

    @Override
    public int hashCode() {
        return Objects.hash(role, privileges, roles);
    }

    @Override
    public String toString() {
        return "Role{" +
                "role='" + role + '\'' +
                ", privileges=" + privileges +
                ", roles=" + roles +
                '}';
    }
}
