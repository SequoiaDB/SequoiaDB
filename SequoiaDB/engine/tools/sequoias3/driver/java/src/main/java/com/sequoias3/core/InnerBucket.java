package com.sequoias3.core;

import com.fasterxml.jackson.annotation.JsonProperty;

public class InnerBucket {
    public static final String BUCKET_NAME                 = "Name";
    public static final String BUCKET_CREATETIME           = "CreationDate";

    @JsonProperty(BUCKET_NAME)
    private String bucketName;

    @JsonProperty(BUCKET_CREATETIME)
    private String formatDate;

    public void setBucketName(String bucketName){
        this.bucketName = bucketName;
    }

    public String getBucketName(){
        return this.bucketName;
    }

    public void setFormatDate(String creationDate){
        this.formatDate = creationDate;
    }

    public String getFormatDate(){
        return this.formatDate;
    }
}
