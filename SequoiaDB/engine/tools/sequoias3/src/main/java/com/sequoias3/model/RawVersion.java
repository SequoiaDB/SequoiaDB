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

public class RawVersion {
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

    public RawVersion(BSONObject bsonObject, String encodingType, Boolean isLatest, Owner owner)
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

            this.isLatest = isLatest;
        }catch (UnsupportedEncodingException e){
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
}
