package com.sequoias3.model;

import com.fasterxml.jackson.annotation.JsonIgnore;
import com.fasterxml.jackson.annotation.JsonProperty;
import com.fasterxml.jackson.dataformat.xml.annotation.JacksonXmlRootElement;

@JacksonXmlRootElement(localName = "CompleteMultipartUploadResult")
public class CompleteMultipartUploadResult {
    @JsonProperty("Location")
    private String location;

    @JsonProperty("Bucket")
    private String bucket;

    @JsonProperty("Key")
    private String Key;
    @JsonProperty("ETag")
    private String eTag;

    @JsonIgnore
    private Long versionId;

    public void seteTag(String eTag) {
        this.eTag = eTag;
    }

    public void setBucket(String bucket) {
        this.bucket = bucket;
    }

    public void setKey(String key) {
        Key = key;
    }

    public void setLocation(String location) {
        this.location = location;
    }

    public String geteTag() {
        return eTag;
    }

    public String getBucket() {
        return bucket;
    }

    public String getKey() {
        return Key;
    }

    public String getLocation() {
        return location;
    }

    public void setVersionId(Long versionId) {
        this.versionId = versionId;
    }

    public Long getVersionId() {
        return versionId;
    }
}

