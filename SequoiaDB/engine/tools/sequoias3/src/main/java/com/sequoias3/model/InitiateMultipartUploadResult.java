package com.sequoias3.model;

import com.fasterxml.jackson.annotation.JsonProperty;
import com.fasterxml.jackson.annotation.JsonPropertyOrder;
import com.fasterxml.jackson.dataformat.xml.annotation.JacksonXmlRootElement;

@JacksonXmlRootElement(localName = "InitiateMultipartUploadResult")
public class InitiateMultipartUploadResult {
    @JsonProperty("Bucket")
    private String bucket;
    @JsonProperty("Key")
    private String key;
    @JsonProperty("UploadId")
    private Long uploadId;

    public InitiateMultipartUploadResult(String bucket, String key, Long uploadId){
        this.bucket   = bucket;
        this.key      = key;
        this.uploadId = uploadId;
    }

    public void setKey(String key) {
        this.key = key;
    }

    public String getKey() {
        return key;
    }

    public void setBucket(String bucket) {
        this.bucket = bucket;
    }

    public String getBucket() {
        return bucket;
    }

    public void setUploadId(long uploadId) {
        this.uploadId = uploadId;
    }

    public long getUploadId() {
        return uploadId;
    }
}
