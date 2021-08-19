package com.sequoias3.model;

import com.fasterxml.jackson.annotation.JsonInclude;
import com.fasterxml.jackson.annotation.JsonProperty;
import com.fasterxml.jackson.dataformat.xml.annotation.JacksonXmlElementWrapper;
import com.fasterxml.jackson.dataformat.xml.annotation.JacksonXmlRootElement;
import com.sequoias3.exception.S3Error;
import com.sequoias3.exception.S3ServerException;

import java.io.UnsupportedEncodingException;
import java.net.URLEncoder;
import java.util.LinkedHashSet;

@JacksonXmlRootElement(localName = "ListBucketResult")
@JsonInclude(value = JsonInclude.Include.NON_NULL)
public class ListObjectsResultV1 {
    @JsonProperty("Name")
    private String name;

    @JsonProperty("Prefix")
    private String prefix;

    @JsonProperty("Marker")
    private String startAfter;

    @JsonProperty("MaxKeys")
    private int    maxKeys;

    @JsonProperty("Delimiter")
    private String delimiter;

    @JsonProperty("IsTruncated")
    private Boolean isTruncated = false;

    @JsonProperty("NextMarker")
    private String nextMarker;

    @JsonProperty("EncodingType")
    private String encodingType;

    @JacksonXmlElementWrapper(localName = "Contents", useWrapping = false)
    @JsonProperty("Contents")
    private LinkedHashSet<Content> contentList;

    @JacksonXmlElementWrapper(localName = "CommonPrefixes", useWrapping = false)
    @JsonProperty("CommonPrefixes")
    private LinkedHashSet<CommonPrefix> commonPrefixList;

    public ListObjectsResultV1(String bucketName, Integer maxKeys, String encodingType,
                             String prefix, String startAfter, String delimiter) throws S3ServerException {
        try {
            this.name = bucketName;
            this.maxKeys = maxKeys;
            this.encodingType = encodingType;
            this.contentList = new LinkedHashSet<>();
            this.commonPrefixList = new LinkedHashSet<>();
            if (null != encodingType) {
                if (null != prefix) {
                    this.prefix = URLEncoder.encode(prefix, "UTF-8");
                }
                if (null != delimiter) {
                    this.delimiter = URLEncoder.encode(delimiter, "UTF-8");
                }
                if (null != startAfter) {
                    this.startAfter = URLEncoder.encode(startAfter, "UTF-8");
                }
            } else {
                this.prefix = prefix;
                this.delimiter = delimiter;
                this.startAfter = startAfter;
            }
        }catch (UnsupportedEncodingException e){
            throw new S3ServerException(S3Error.UNKNOWN_ERROR, "URL encode failed", e);
        }
    }

    public void setName(String name) {
        this.name = name;
    }

    public String getName() {
        return name;
    }

    public void setPrefix(String prefix) {
        this.prefix = prefix;
    }

    public String getPrefix() {
        return prefix;
    }

    public void setStartAfter(String startAfter) {
        this.startAfter = startAfter;
    }

    public String getStartAfter() {
        return startAfter;
    }

    public void setDelimiter(String delimiter) {
        this.delimiter = delimiter;
    }

    public String getDelimiter() {
        return delimiter;
    }

    public void setIsTruncated(Boolean isTruncated) {
        this.isTruncated = isTruncated;
    }

    public Boolean getIsTruncated() {
        return isTruncated;
    }

    public void setEncodingType(String encodingType) {
        this.encodingType = encodingType;
    }

    public String getEncodingType() {
        return encodingType;
    }

    public void setMaxKeys(int maxKeys) {
        this.maxKeys = maxKeys;
    }

    public int getMaxKeys() {
        return maxKeys;
    }

    public void setNextMarker(String nextMarker) {
        this.nextMarker = nextMarker;
    }

    public String getNextMarker() {
        return nextMarker;
    }

    public void setContentList(LinkedHashSet<Content> contentList) {
        this.contentList = contentList;
    }

    public LinkedHashSet<Content> getContentList() {
        return contentList;
    }

    public void setCommonPrefixList(LinkedHashSet<CommonPrefix> commonPrefixList) {
        this.commonPrefixList = commonPrefixList;
    }

    public LinkedHashSet<CommonPrefix> getCommonPrefixList() {
            return commonPrefixList;
        }
}
