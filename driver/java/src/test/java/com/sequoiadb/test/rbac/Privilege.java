package com.sequoiadb.test.rbac;


import org.bson.BSONObject;
import org.bson.BasicBSONObject;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Objects;

public class Privilege {
    private Resource resource;
    private List<String> actions;

    public static Privilege.PrivilegeBuilder builder() {
        return new Privilege.PrivilegeBuilder();
    }

    public BSONObject toBson() {
        BasicBSONObject bson = new BasicBSONObject();
        if (resource != null) {
            bson.put("Resource", resource.toBson());
        }
        if (actions != null) {
            bson.put("Actions", actions);
        }
        return bson;
    }

    public static class PrivilegeBuilder {
        private Resource resource;
        private List<String> actions;

        public PrivilegeBuilder resource(Resource resource) {
            this.resource = resource;
            return this;
        }

        public PrivilegeBuilder resource(String cs, String cl) {
            this.resource = new Resource(cs, cl, null);
            return this;
        }

        public PrivilegeBuilder resource(Boolean cluster) {
            this.resource = new Resource(null, null, cluster);
            return this;
        }

        public PrivilegeBuilder actions(List<String> actions) {
            this.actions = actions;
            return this;
        }

        public PrivilegeBuilder addActions(String ...actions) {
            if (this.actions == null) {
                this.actions = new ArrayList<>();
            }
            this.actions.addAll(Arrays.asList(actions));
            return this;
        }

        public Privilege build() {
            Privilege privilege = new Privilege();
            privilege.setActions(actions);
            privilege.setResource(resource);
            return privilege;
        }
    }

    public Resource getResource() {
        return resource;
    }

    public void setResource(Resource resource) {
        this.resource = resource;
    }

    public List<String> getActions() {
        return actions;
    }

    public void setActions(List<String> actions) {
        this.actions = actions;
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        if (o == null || getClass() != o.getClass()) return false;
        Privilege privilege = (Privilege) o;
        return Objects.equals(resource, privilege.resource) && Objects.equals(actions, privilege.actions);
    }

    @Override
    public int hashCode() {
        return Objects.hash(resource, actions);
    }

    @Override
    public String toString() {
        return "Privilege{" +
                "resource=" + resource +
                ", actions=" + actions +
                '}';
    }
}
