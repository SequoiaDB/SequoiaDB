package com.sequoias3.core;

import com.fasterxml.jackson.annotation.JsonIgnore;
import com.fasterxml.jackson.annotation.JsonProperty;
import org.bson.BSONObject;
import org.bson.types.ObjectId;

import java.util.Map;

public class UploadMeta {
    public static final String META_KEY_NAME            = "Key";
    public static final String META_BUCKET_ID           = "BucketId";
    public static final String META_CS_NAME             = "CSName";
    public static final String META_CL_NAME             = "CLName";
    public static final String META_LOB_ID              = "LobId";
    public static final String META_UPLOAD_ID           = "UploadId";
    public static final String META_STATUS              = "UploadStatus";
    public static final String META_INIT_TIME           = "InitiatedTime";
    public static final String META_CONTENT_TYPE        = "ContentType";
    public static final String META_CONTENT_ENCODING    = "ContentEncoding";
    public static final String META_CACHE_CONTROL       = "CacheControl";
    public static final String META_CONTENT_DISPOSITION = "ContentDisposition";
    public static final String META_EXPIRES             = "Expires";
    public static final String META_CONTENT_LANGUAGE    = "ContentLanguage";
    public static final String META_LIST                = "MetaList";

    public static final int UPLOAD_INIT                 = 1;
    public static final int UPLOAD_COMPLETE             = 2;
    public static final int UPLOAD_ABORT                = 3;

    public static final String UPLOAD_INDEX             = META_BUCKET_ID + "_" + META_KEY_NAME + "_" + META_UPLOAD_ID;

    @JsonProperty("Key")
    private String key;
    @JsonIgnore
    private long bucketId;
    @JsonIgnore
    private String csName;
    @JsonIgnore
    private String clName;
    @JsonIgnore
    private ObjectId lobId;
    @JsonProperty("UploadId")
    private long uploadId;
    @JsonIgnore
    private int uploadStatus;
    @JsonIgnore
    private long lastModified;
    @JsonIgnore
    private String contentEncoding;
    @JsonIgnore
    private String contentType;
    @JsonIgnore
    private String cacheControl;
    @JsonIgnore
    private String contentDisposition;
    @JsonIgnore
    private String expires;
    @JsonIgnore
    private String contentLanguage;
    @JsonIgnore
    private Map<String, String> metaList;

    public UploadMeta(){}

    public UploadMeta(BSONObject record){
        this.bucketId     = (long) record.get(UploadMeta.META_BUCKET_ID);
        this.key          = record.get(UploadMeta.META_KEY_NAME).toString();
        this.uploadId     = (long) record.get(UploadMeta.META_UPLOAD_ID);
        this.lastModified = (long) record.get(UploadMeta.META_INIT_TIME);
        this.uploadStatus = (int) record.get(UploadMeta.META_STATUS);
        if (record.get(UploadMeta.META_CS_NAME) != null){
            this.csName = record.get(UploadMeta.META_CS_NAME).toString();
        }
        if (record.get(UploadMeta.META_CL_NAME) != null){
            this.clName = record.get(UploadMeta.META_CL_NAME).toString();
        }
        if (record.get(UploadMeta.META_LOB_ID) != null){
            this.lobId = (ObjectId) record.get(UploadMeta.META_LOB_ID);
        }
        if (record.get(UploadMeta.META_CACHE_CONTROL) != null){
            this.cacheControl = record.get(UploadMeta.META_CACHE_CONTROL).toString();
        }
        if (record.get(UploadMeta.META_CONTENT_DISPOSITION) != null){
            this.contentDisposition = record.get(UploadMeta.META_CONTENT_DISPOSITION).toString();
        }
        if (record.get(UploadMeta.META_CONTENT_ENCODING) != null){
            this.contentEncoding = record.get(UploadMeta.META_CONTENT_ENCODING).toString();
        }
        if (record.get(UploadMeta.META_CONTENT_LANGUAGE) != null){
            this.contentLanguage = record.get(UploadMeta.META_CONTENT_LANGUAGE).toString();
        }
        if (record.get(UploadMeta.META_CONTENT_TYPE) != null){
            this.contentType = record.get(UploadMeta.META_CONTENT_TYPE).toString();
        }
        if (record.get(UploadMeta.META_EXPIRES) != null){
            this.expires = record.get(UploadMeta.META_EXPIRES).toString();
        }
        if (record.get(UploadMeta.META_LIST) != null){
            this.metaList = ((BSONObject)record.get(UploadMeta.META_LIST)).toMap();
        }
    }

    public long getBucketId() {
        return bucketId;
    }

    public void setBucketId(long bucketId) {
        this.bucketId = bucketId;
    }

    public String getKey() {
        return key;
    }

    public void setKey(String key) {
        this.key = key;
    }

    public void setUploadId(long uploadId) {
        this.uploadId = uploadId;
    }

    public long getUploadId() {
        return uploadId;
    }

    public void setCsName(String csName) {
        this.csName = csName;
    }

    public String getCsName() {
        return csName;
    }

    public void setClName(String clName) {
        this.clName = clName;
    }

    public String getClName() {
        return clName;
    }

    public ObjectId getLobId() {
        return lobId;
    }

    public void setLobId(ObjectId lobId) {
        this.lobId = lobId;
    }

    public void setUploadStatus(int uploadStatus) {
        this.uploadStatus = uploadStatus;
    }

    public int getUploadStatus() {
        return uploadStatus;
    }

    public void setLastModified(long lastModifiedTime) {
        this.lastModified = lastModifiedTime;
    }

    public long getLastModified() {
        return lastModified;
    }

    public void setCacheControl(String cacheControl) {
        this.cacheControl = cacheControl;
    }

    public String getCacheControl() {
        return cacheControl;
    }

    public void setContentDisposition(String contentDisposition) {
        this.contentDisposition = contentDisposition;
    }

    public String getContentDisposition() {
        return contentDisposition;
    }

    public void setContentEncoding(String contentEncoding) {
        this.contentEncoding = contentEncoding;
    }

    public String getContentEncoding() {
        return contentEncoding;
    }

    public void setContentType(String contentType) {
        this.contentType = contentType;
    }

    public String getContentType() {
        return contentType;
    }

    public void setExpires(String expires) {
        this.expires = expires;
    }

    public String getExpires() {
        return expires;
    }

    public void setContentLanguage(String contentLanguage) {
        this.contentLanguage = contentLanguage;
    }

    public String getContentLanguage() {
        return contentLanguage;
    }

    public void setMetaList(Map<String, String> metaList) {
        this.metaList = metaList;
    }

    public Map<String, String> getMetaList() {
        return metaList;
    }

}
