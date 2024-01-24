package com.sequoias3.model;

import com.fasterxml.jackson.annotation.JsonInclude;
import com.fasterxml.jackson.annotation.JsonProperty;
import com.fasterxml.jackson.dataformat.xml.annotation.JacksonXmlElementWrapper;
import com.fasterxml.jackson.dataformat.xml.annotation.JacksonXmlRootElement;
import com.sequoias3.core.Part;
import com.sequoias3.exception.S3Error;
import com.sequoias3.exception.S3ServerException;

import java.io.UnsupportedEncodingException;
import java.net.URLEncoder;
import java.util.LinkedHashSet;

@JacksonXmlRootElement(localName = "ListPartsResult")
@JsonInclude(value = JsonInclude.Include.NON_NULL)
public class ListPartsResult {
    @JsonProperty("Bucket")
    private String bucket;
    @JsonProperty("Key")
    private String key;
    @JsonProperty("UploadId")
    private Long   uploadId;
    @JsonProperty("Initiator")
    private Owner initiator;
    @JsonProperty("Owner")
    private Owner owner;
    @JsonProperty("MaxParts")
    private Integer   maxparts;
    @JsonProperty("IsTruncated")
    private Boolean isTruncated = false;
    @JsonProperty("PartNumberMarker")
    private int partNumberMarker;
    @JsonProperty("NextPartNumberMarker")
    private int nextPartNumberMarker;
    @JacksonXmlElementWrapper(localName = "Part", useWrapping = false)
    @JsonProperty("Part")
    private LinkedHashSet<Part> partList;

    public ListPartsResult(String bucket, String key, Long uploadId, Integer maxparts,
                           Integer partNumberMarker, Owner owner, String encodingType) throws S3ServerException{
        try {
            this.bucket = bucket;
            if (encodingType != null) {
                this.key = URLEncoder.encode(key, "UTF-8");
            }else {
                this.key = key;
            }
            this.uploadId  = uploadId;
            this.maxparts  = maxparts;
            this.initiator = owner;
            this.owner     = owner;
            if (partNumberMarker != null){
                this.partNumberMarker = partNumberMarker;
            }
            this.partList  = new LinkedHashSet<>();
        }catch (UnsupportedEncodingException e) {
            throw new S3ServerException(S3Error.UNKNOWN_ERROR, "URL encode failed", e);
        }
    }

    public void setKey(String key) {
        this.key = key;
    }

    public void setBucket(String bucket) {
        this.bucket = bucket;
    }

    public void setUploadId(Long uploadId) {
        this.uploadId = uploadId;
    }

    public void setInitiator(Owner initiator) {
        this.initiator = initiator;
    }

    public void setOwner(Owner owner) {
        this.owner = owner;
    }

    public void setMaxparts(int maxparts) {
        this.maxparts = maxparts;
    }

    public void setPartList(LinkedHashSet<Part> partList) {
        this.partList = partList;
    }

    public String getKey() {
        return key;
    }

    public String getBucket() {
        return bucket;
    }

    public Long getUploadId() {
        return uploadId;
    }

    public Owner getInitiator() {
        return initiator;
    }

    public Owner getOwner() {
        return owner;
    }

    public LinkedHashSet<Part> getPartList() {
        return partList;
    }

    public int getMaxparts() {
        return maxparts;
    }

    public void setIsTruncated(Boolean truncated) {
        this.isTruncated = truncated;
    }

    public Boolean getIsTruncated() {
        return isTruncated;
    }

    public void setNextPartNumberMarker(int nextPartNumberMarker) {
        this.nextPartNumberMarker = nextPartNumberMarker;
    }

    public int getNextPartNumberMarker() {
        return nextPartNumberMarker;
    }
}
