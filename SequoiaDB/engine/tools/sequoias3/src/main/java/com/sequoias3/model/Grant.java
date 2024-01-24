package com.sequoias3.model;

import com.fasterxml.jackson.annotation.JsonProperty;
import com.fasterxml.jackson.dataformat.xml.annotation.JacksonXmlProperty;

public class Grant {
    @JsonProperty("Grantee")
    private Grantee grantee;
    @JsonProperty("Permission")
    private String permission;

    public Grantee getGrantee() {
        return grantee;
    }

    public void setGrantee(Grantee grantee) {
        this.grantee = grantee;
    }

    public String getPermission() {
        return permission;
    }

    public void setPermission(String permission) {
        this.permission = permission;
    }
}
