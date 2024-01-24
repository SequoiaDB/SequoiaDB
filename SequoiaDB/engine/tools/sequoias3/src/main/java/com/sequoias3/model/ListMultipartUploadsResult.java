package com.sequoias3.model;

import com.fasterxml.jackson.annotation.JsonInclude;
import com.fasterxml.jackson.annotation.JsonProperty;
import com.fasterxml.jackson.dataformat.xml.annotation.JacksonXmlElementWrapper;
import com.fasterxml.jackson.dataformat.xml.annotation.JacksonXmlRootElement;
import com.sequoias3.model.Upload;
import com.sequoias3.exception.S3Error;
import com.sequoias3.exception.S3ServerException;

import java.io.UnsupportedEncodingException;
import java.net.URLEncoder;
import java.util.LinkedHashSet;

@JacksonXmlRootElement(localName = "ListMultipartUploadsResult")
@JsonInclude(value = JsonInclude.Include.NON_NULL)
public class ListMultipartUploadsResult {
    @JsonProperty("Bucket")
    private String bucket;

    @JsonProperty("Prefix")
    private String prefix;

    @JsonProperty("Delimiter")
    private String delimiter;

    @JsonProperty("KeyMarker")
    private String keyMarker;

    @JsonProperty("UploadIdMarker")
    private Long uploadIdMarker;

    @JsonProperty("MaxUploads")
    private int    maxUploads;

    @JsonProperty("EncodingType")
    private String encodingType;

    @JsonProperty("IsTruncated")
    private Boolean isTruncated = false;

    @JsonProperty("NextKeyMarker")
    private String nextKeyMarker;

    @JsonProperty("NextUploadIdMarker")
    private Long nextUploadIdMarker;

    @JacksonXmlElementWrapper(localName = "Upload", useWrapping = false)
    @JsonProperty("Upload")
    private LinkedHashSet<Upload> uploadList;

    @JacksonXmlElementWrapper(localName = "CommonPrefixes", useWrapping = false)
    @JsonProperty("CommonPrefixes")
    private LinkedHashSet<CommonPrefix> commonPrefixList;

    public ListMultipartUploadsResult(String bucketName, Integer maxUploads, String encodingType,
                                      String prefix, String delimiter, String keyMarker,
                                      Long uploadIdMarker)throws S3ServerException {
        try {
            this.bucket = bucketName;
            this.maxUploads = maxUploads;
            this.encodingType = encodingType;
            this.uploadIdMarker = uploadIdMarker;
            this.uploadList = new LinkedHashSet<>();
            this.commonPrefixList = new LinkedHashSet<>();
            if (null != encodingType) {
                if (null != prefix) {
                    this.prefix = URLEncoder.encode(prefix, "UTF-8");
                }
                if (null != delimiter) {
                    this.delimiter = URLEncoder.encode(delimiter, "UTF-8");
                }
                if (null != keyMarker) {
                    this.keyMarker = URLEncoder.encode(keyMarker, "UTF-8");
                }
            } else {
                this.prefix = prefix;
                this.delimiter = delimiter;
                this.keyMarker = keyMarker;
            }
        } catch (UnsupportedEncodingException e) {
            throw new S3ServerException(S3Error.UNKNOWN_ERROR, "URL encode failed", e);
        }
    }

    public void setBucket(String bucket) {
        this.bucket = bucket;
    }

    public String getBucket() {
        return bucket;
    }

    public void setDelimiter(String delimiter) {
        this.delimiter = delimiter;
    }

    public String getDelimiter() {
        return delimiter;
    }

    public void setEncodingType(String encodingType) {
        this.encodingType = encodingType;
    }

    public String getEncodingType() {
        return encodingType;
    }

    public void setPrefix(String prefix) {
        this.prefix = prefix;
    }

    public String getPrefix() {
        return prefix;
    }

    public void setKeyMarker(String keyMarker) {
        this.keyMarker = keyMarker;
    }

    public String getKeyMarker() {
        return keyMarker;
    }

    public void setUploadIdMarker(Long uploadIdMarker) {
        this.uploadIdMarker = uploadIdMarker;
    }

    public Long getUploadIdMarker() {
        return uploadIdMarker;
    }

    public void setNextKeyMarker(String nextKeyMarker) {
        this.nextKeyMarker = nextKeyMarker;
    }

    public String getNextKeyMarker() {
        return nextKeyMarker;
    }

    public void setNextUploadIdMarker(Long nextUploadIdMarker) {
        this.nextUploadIdMarker = nextUploadIdMarker;
    }

    public Long getNextUploadIdMarker() {
        return nextUploadIdMarker;
    }

    public void setMaxUploads(int maxUploads) {
        this.maxUploads = maxUploads;
    }

    public int getMaxUploads() {
        return maxUploads;
    }

    public void setCommonPrefixList(LinkedHashSet<CommonPrefix> commonPrefixList) {
        this.commonPrefixList = commonPrefixList;
    }

    public LinkedHashSet<CommonPrefix> getCommonPrefixList() {
        return commonPrefixList;
    }

    public void setUploadList(LinkedHashSet<Upload> uploadList) {
        this.uploadList = uploadList;
    }

    public LinkedHashSet<Upload> getUploadList() {
        return uploadList;
    }

    public void setIsTruncated(Boolean truncated) {
        this.isTruncated = truncated;
    }

    public Boolean getIsTruncated() {
        return isTruncated;
    }
}
