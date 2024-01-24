package com.sequoias3.model;

import com.fasterxml.jackson.annotation.JsonIgnore;
import com.fasterxml.jackson.annotation.JsonProperty;
import com.sequoias3.core.ObjectMeta;
import com.sequoias3.exception.S3Error;
import com.sequoias3.exception.S3ServerException;
import org.bson.BSONObject;

import java.io.UnsupportedEncodingException;
import java.net.URLEncoder;

import static com.sequoias3.utils.DataFormatUtils.formatDate;

public class Version {
    @JsonProperty("Key")
    private String key;
    @JsonProperty("VersionId")
    private String versionId;
    @JsonProperty("IsLatest")
    private Boolean isLatest;
    @JsonProperty("LastModified")
    private String lastModified;
    @JsonProperty("Owner")
    private Owner owner;
    @JsonIgnore
    private Boolean noVersionFlag;

    @JsonProperty("ETag")
    private String eTag;
    @JsonProperty("Size")
    private long   size;

    public Version(BSONObject bsonObject, String encodingType, Boolean isLatest, Owner owner)
            throws S3ServerException {
        try{
            if (null != encodingType) {
                this.key = URLEncoder.encode(bsonObject.get(ObjectMeta.META_KEY_NAME).toString(), "UTF-8");
            } else {
                this.key = bsonObject.get(ObjectMeta.META_KEY_NAME).toString();
            }
            this.lastModified = formatDate((long) bsonObject.get(ObjectMeta.META_LAST_MODIFIED));
            if ((Boolean)bsonObject.get(ObjectMeta.META_NO_VERSION_FLAG)) {
                this.versionId = ObjectMeta.NULL_VERSION_ID;
            }else{
                this.versionId = bsonObject.get(ObjectMeta.META_VERSION_ID).toString();
            }

            if (bsonObject.get(ObjectMeta.META_ETAG) != null) {
                this.eTag = bsonObject.get(ObjectMeta.META_ETAG).toString();
            }
            if (bsonObject.get(ObjectMeta.META_SIZE) != null) {
                this.size = (long) bsonObject.get(ObjectMeta.META_SIZE);
            }
            this.isLatest = isLatest;
        }catch (UnsupportedEncodingException e){
            //logger.error("Encode object name failed. e", e);
            throw new S3ServerException(S3Error.UNKNOWN_ERROR,
                    "encode object name failed."+e.getMessage(), e);
        }
    }

    public void setKey(String key) {
        this.key = key;
    }

    public String getKey() {
        return key;
    }

    public void setLastModified(String lastModified) {
        this.lastModified = lastModified;
    }

    public String getLastModified() {
        return lastModified;
    }

    public void setIsLatest(Boolean latest) {
        this.isLatest = latest;
    }

    public Boolean getIsLatest() {
        return this.isLatest;
    }

    public void setVersionId(String versionId) {
        this.versionId = versionId;
    }

    public String getVersionId() {
        return versionId;
    }

    public void setOwner(Owner owner) {
        this.owner = owner;
    }

    public Owner getOwner() {
        return owner;
    }

    public void setNoVersionFlag(Boolean noVersionFlag) {
        this.noVersionFlag = noVersionFlag;
    }

    public Boolean getNoVersionFlag() {
        return noVersionFlag;
    }

    public void seteTag(String eTag) {
        this.eTag = eTag;
    }

    public String geteTag() {
        return eTag;
    }

    public void setSize(long size) {
        this.size = size;
    }

    public long getSize() {
        return size;
    }
}
