package com.sequoias3.model;

import com.fasterxml.jackson.annotation.JsonProperty;
import com.fasterxml.jackson.annotation.JsonPropertyOrder;
import com.fasterxml.jackson.dataformat.xml.annotation.JacksonXmlRootElement;

@JacksonXmlRootElement(localName = "AccessKeys")
@JsonPropertyOrder({"accessKeyID","secretAccessKey"})
public class AccessKeys {
    public static final String JSON_ACCESSKEY_ID = "AccessKeyID";
    public static final String JSON_SECRET_ASSCESS_KEY = "SecretAccessKey";

    @JsonProperty(JSON_ACCESSKEY_ID)
    private String accessKeyID;
    @JsonProperty(JSON_SECRET_ASSCESS_KEY)
    private String secretAccessKey;

    public AccessKeys(String accessKeyID, String secretAccessKey) {
        this.accessKeyID = accessKeyID;
        this.secretAccessKey = secretAccessKey;
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
