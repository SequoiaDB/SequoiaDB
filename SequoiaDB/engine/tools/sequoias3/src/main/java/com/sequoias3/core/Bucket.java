package com.sequoias3.core;

import com.fasterxml.jackson.annotation.JsonIgnore;
import com.fasterxml.jackson.annotation.JsonProperty;
import com.sequoias3.common.DelimiterStatus;

public class Bucket {
    public static final String BUCKET_ID                   = "ID";
    public static final String BUCKET_NAME                 = "Name";
    public static final String BUCKET_OWNERID              = "OwnerID";
    public static final String BUCKET_CREATETIME           = "CreationDate";
    public static final String BUCKET_VERSIONINGSTATUS     = "VersioningStatus";
    public static final String BUCKET_REGION               = "Region";
    public static final String BUCKET_DELIMITER            = "Delimiter";
    public static final String BUCKET_DELIMITER1           = "Delimiter1";
    public static final String BUCKET_DELIMITER1STATUS     = "Delimiter1Status";
    public static final String BUCKET_DELIMITER1CREATETIME = "Delimiter1CreateTime";
    public static final String BUCKET_DELIMITER1MODTIME    = "Delimiter1ModTime";
    public static final String BUCKET_DELIMITER2           = "Delimiter2";
    public static final String BUCKET_DELIMITER2STATUS     = "Delimiter2Status";
    public static final String BUCKET_DELIMITER2CREATETIME = "Delimiter2CreateTime";
    public static final String BUCKET_DELIMITER2MODTIME    = "Delimiter2ModTime";
    public static final String BUCKET_TASKID               = "TaskID";
    public static final String BUCKET_ACLID                = "AclID";
    public static final String BUCKET_PRIVATE              = "IsPrivate";

    public static final String NAME_INDEX = "nameIndex";
    public static final String ID_INDEX   = "idIndex";

    @JsonIgnore
    private long    bucketId;

    @JsonProperty(BUCKET_NAME)
    private String bucketName;

    @JsonIgnore
    private long    ownerId;

    @JsonProperty(BUCKET_CREATETIME)
    private String formatDate;

    @JsonIgnore
    private long timeMillis;

    @JsonIgnore
    private String versioningStatus;

    @JsonIgnore
    private Integer delimiter;

    @JsonIgnore
    private String region;
    @JsonIgnore
    private String delimiter1;
    @JsonIgnore
    private String delimiter1Status;
    @JsonIgnore
    private Long delimiter1CreateTime;
    @JsonIgnore
    private Long delimiter1ModTime;
    @JsonIgnore
    private String delimiter2;
    @JsonIgnore
    private String delimiter2Status;
    @JsonIgnore
    private Long delimiter2CreateTime;
    @JsonIgnore
    private Long delimiter2ModTime;
    @JsonIgnore
    private Long taskID;
    @JsonIgnore
    private Boolean isPrivate;
    @JsonIgnore
    private Long aclId;

    public void setBucketId(long bucketId){
        this.bucketId = bucketId;
    }

    public long getBucketId(){
        return this.bucketId;
    }

    public void setBucketName(String bucketName){
        this.bucketName = bucketName;
    }

    public String getBucketName(){
        return this.bucketName;
    }

    public void setOwnerId(long ownerId){
        this.ownerId = ownerId;
    }

    public long getOwnerId(){
        return this.ownerId;
    }

    public void setFormatDate(String creationDate){
        this.formatDate = creationDate;
    }

    public String getFormatDate(){
        return this.formatDate;
    }

    public void setTimeMillis(long timeMillis) {
        this.timeMillis = timeMillis;
    }

    public long getTimeMillis() {
        return timeMillis;
    }

    public void setVersioningStatus(String versioningStatus){
        this.versioningStatus = versioningStatus;
    }

    public String getVersioningStatus(){
        return this.versioningStatus;
    }

    public void setRegion(String region) {
        this.region = region;
    }

    public String getRegion() {
        return region;
    }

    public void setDelimiter(Integer delimiter){
        this.delimiter = delimiter;
    }

    public Integer getDelimiter(){
        return this.delimiter;
    }

    public void setDelimiter1(String delimiter1) {
        this.delimiter1 = delimiter1;
    }

    public String getDelimiter1() {
        return delimiter1;
    }

    public void setDelimiter1Status(String delimiter1Status) {
        this.delimiter1Status = delimiter1Status;
    }

    public String getDelimiter1Status() {
        return delimiter1Status;
    }

    public void setDelimiter1CreateTime(Long delimiter1CreateTime) {
        this.delimiter1CreateTime = delimiter1CreateTime;
    }

    public Long getDelimiter1CreateTime() {
        return delimiter1CreateTime;
    }

    public void setDelimiter1ModTime(Long delimiter1ModTime) {
        this.delimiter1ModTime = delimiter1ModTime;
    }

    public Long getDelimiter1ModTime() {
        return delimiter1ModTime;
    }

    public void setDelimiter2(String delimiter2) {
        this.delimiter2 = delimiter2;
    }

    public String getDelimiter2() {
        return delimiter2;
    }

    public void setDelimiter2Status(String delimiter2Status) {
        this.delimiter2Status = delimiter2Status;
    }

    public String getDelimiter2Status() {
        return delimiter2Status;
    }

    public void setDelimiter2CreateTime(Long delimiter2CreateTime) {
        this.delimiter2CreateTime = delimiter2CreateTime;
    }

    public Long getDelimiter2CreateTime() {
        return delimiter2CreateTime;
    }

    public void setDelimiter2ModTime(Long delimiter2ModTime) {
        this.delimiter2ModTime = delimiter2ModTime;
    }

    public Long getDelimiter2ModTime() {
        return delimiter2ModTime;
    }

    public void setTaskID(Long taskID) {
        this.taskID = taskID;
    }

    public Long getTaskID() {
        return taskID;
    }

    public void setAclId(Long aclId) {
        this.aclId = aclId;
    }

    public Long getAclId() {
        return aclId;
    }

    @JsonIgnore
    public void setPrivate(boolean aPrivate) {
        isPrivate = aPrivate;
    }

    @JsonIgnore
    public Boolean isPrivate() {
        return isPrivate;
    }
}
