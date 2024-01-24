package com.sequoiadb.test.rbac;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;

import java.util.Objects;

public class Resource {
    private String cs;
    private String cl;
    private Boolean cluster;

    public Resource() {
    }

    public Resource(String cs, String cl, Boolean cluster) {
        this.cs = cs;
        this.cl = cl;
        this.cluster = cluster;
    }

    public BSONObject toBson() {
        BasicBSONObject bson = new BasicBSONObject();
        if (cs != null) {
            bson.put("cs", cs);
        }
        if (cl != null) {
            bson.put("cl", cl);
        }
        if (cluster != null) {
            bson.put("Cluster", cluster);
        }
        return bson;
    }

    public String getCs() {
        return cs;
    }

    public void setCs(String cs) {
        this.cs = cs;
    }

    public String getCl() {
        return cl;
    }

    public void setCl(String cl) {
        this.cl = cl;
    }

    public Boolean getCluster() {
        return cluster;
    }

    public void setCluster(Boolean cluster) {
        this.cluster = cluster;
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        if (o == null || getClass() != o.getClass()) return false;
        Resource resource = (Resource) o;
        return Objects.equals(cs, resource.cs)
                && Objects.equals(cl, resource.cl)
                && Objects.equals(cluster, resource.cluster);
    }

    @Override
    public int hashCode() {
        return Objects.hash(cs, cl, cluster);
    }

    @Override
    public String toString() {
        return "Resource{" +
                "cs='" + cs + '\'' +
                ", cl='" + cl + '\'' +
                ", cluster=" + cluster +
                '}';
    }
}
