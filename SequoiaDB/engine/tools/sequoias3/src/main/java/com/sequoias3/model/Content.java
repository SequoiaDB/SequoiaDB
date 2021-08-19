package com.sequoias3.model;

import com.fasterxml.jackson.annotation.JsonInclude;
import com.fasterxml.jackson.annotation.JsonProperty;
import com.sequoias3.core.ObjectMeta;
import com.sequoias3.exception.S3Error;
import com.sequoias3.exception.S3ServerException;
import org.bson.BSONObject;

import java.io.UnsupportedEncodingException;
import java.net.URLEncoder;

import static com.sequoias3.utils.DataFormatUtils.formatDate;

@JsonInclude(value = JsonInclude.Include.NON_NULL)
public class Content {
    @JsonProperty("Key")
    private String key;
    @JsonProperty("LastModified")
    private String lastModified;
    @JsonProperty("ETag")
    private String eTag;
    @JsonProperty("Size")
    private long   size;
    @JsonProperty("Owner")
    private Owner owner;

    public Content(){}

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

    public void setOwner(Owner owner) {
        this.owner = owner;
    }

    public Owner getOwner() {
        return owner;
    }

    public Content (BSONObject bsonObject, String encodingType)
            throws S3ServerException {
        try {
            if (null != encodingType) {
                this.key = URLEncoder.encode(bsonObject.get(ObjectMeta.META_KEY_NAME).toString(), "UTF-8");
            } else {
                this.key = bsonObject.get(ObjectMeta.META_KEY_NAME).toString();
            }
            this.lastModified = formatDate((long) bsonObject.get(ObjectMeta.META_LAST_MODIFIED));
            this.eTag         = bsonObject.get(ObjectMeta.META_ETAG).toString();
            this.size         = (long) bsonObject.get(ObjectMeta.META_SIZE);
        }catch (UnsupportedEncodingException e){
            //logger.error("Encode object name failed. e", e);
            throw new S3ServerException(S3Error.UNKNOWN_ERROR,
                    "encode object name failed."+e.getMessage(), e);
        }
    }
}
