package com.sequoias3.model;

import com.fasterxml.jackson.annotation.JsonProperty;

public class DeleteError {
    @JsonProperty("Code")
    private String code;
    @JsonProperty("Key")
    private String key;
    @JsonProperty("Message")
    private String message;
    @JsonProperty("VersionId")
    private String versionId;

    public void setCode(String code) {
        this.code = code;
    }

    public String getCode() {
        return code;
    }

    public void setKey(String key) {
        this.key = key;
    }

    public String getKey() {
        return key;
    }

    public void setMessage(String message) {
        this.message = message;
    }

    public String getMessage() {
        return message;
    }

    public void setVersionId(String versionId) {
        this.versionId = versionId;
    }

    public String getVersionId() {
        return versionId;
    }
}
