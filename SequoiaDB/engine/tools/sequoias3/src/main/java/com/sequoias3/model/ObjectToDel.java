package com.sequoias3.model;

import com.fasterxml.jackson.annotation.JsonProperty;

public class ObjectToDel {
    @JsonProperty("Key")
    private String key;
    @JsonProperty("VersionId")
    private String versionId;

    public void setKey(String key) {
        this.key = key;
    }

    public String getKey() {
        return key;
    }

    public void setVersionId(String versionId) {
        this.versionId = versionId;
    }

    public String getVersionId() {
        return versionId;
    }
}
