package com.sequoias3.core;

import com.fasterxml.jackson.annotation.JsonIgnore;
import com.fasterxml.jackson.annotation.JsonProperty;

public class User {
    public static final String JSON_KEY_USERNAME = "Name";
    public static final String JSON_KEY_USERID = "ID";
    public static final String JSON_KEY_ROLE = "Role";
    public static final String JSON_KEY_ACCESS_KEY_ID = "AccessKeyID";
    public static final String JSON_KEY_SECRET_ACCESS_KEY = "SecretAccessKey";
    public static final String DISPLAY_NAME = "DisplayName";

    @JsonProperty(DISPLAY_NAME)
    private String userName;
    @JsonProperty(JSON_KEY_USERID)
    private long userId;
    @JsonIgnore
    private String role;
    @JsonIgnore
    private String accessKeyID;
    @JsonIgnore
    private String secretAccessKey;

    public String getUserName() {
        return userName;
    }

    public void setUserName(String username) {
        this.userName = username;
    }

    public long getUserId() {
        return userId;
    }

    public void setUserId(long userId) {
        this.userId = userId;
    }

    public String getRole() {
        return role;
    }

    public void setRole(String role) {
        this.role = role;
    }

    public String getAccessKeyID() {
        return accessKeyID;
    }

    public void setAccessKeyID(String accessKeyID) {
        this.accessKeyID = accessKeyID;
    }

    public String getSecretAccessKey() {
        return secretAccessKey;
    }

    public void setSecretAccessKey(String secretAccessKey) {
        this.secretAccessKey = secretAccessKey;
    }
}
