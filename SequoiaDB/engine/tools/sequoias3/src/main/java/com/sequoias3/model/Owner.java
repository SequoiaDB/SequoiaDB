package com.sequoias3.model;

import com.fasterxml.jackson.annotation.JsonProperty;

public class Owner {
    public static final String DISPLAY_NAME = "DisplayName";
    public static final String JSON_KEY_USERID = "ID";

    @JsonProperty(DISPLAY_NAME)
    private String userName;
    @JsonProperty(JSON_KEY_USERID)
    private long userId;

    public void setUserId(long userId) {
        this.userId = userId;
    }

    public long getUserId() {
        return userId;
    }

    public void setUserName(String userName) {
        this.userName = userName;
    }

    public String getUserName() {
        return userName;
    }
}
