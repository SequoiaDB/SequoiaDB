package com.sequoias3.core;

import com.fasterxml.jackson.annotation.JsonIgnore;
import com.fasterxml.jackson.annotation.JsonProperty;
import com.sequoias3.exception.S3Error;
import com.sequoias3.exception.S3ServerException;
import org.bson.BSONObject;
import org.bson.types.ObjectId;

import java.io.UnsupportedEncodingException;
import java.net.URLEncoder;

import static com.sequoias3.utils.DataFormatUtils.formatDate;

public class Part {
    public static final String UPLOADID     = "UploadId";
    public static final String PARTNUMBER   = "PartNumber";
    public static final String CSNAME       = "CSName";
    public static final String CLNAME       = "CLName";
    public static final String LOBID        = "LobId";
    public static final String SIZE         = "Size";
    public static final String ETAG         = "ETag";
    public static final String LASTMODIFIED = "LastModified";

    public static final String PART_INDEX = UPLOADID + "_" + PARTNUMBER;

    @JsonIgnore
    private Long uploadId;
    @JsonProperty("PartNumber")
    private int partNumber;
    @JsonIgnore
    private String csName;
    @JsonIgnore
    private String clName;
    @JsonIgnore
    private ObjectId lobId;
    @JsonProperty("ETag")
    private String etag;
    @JsonIgnore
    private long lastModified;
    @JsonProperty("LastModified")
    private String lastModifiedDate;
    @JsonProperty("Size")
    private long size;

    public void setLastModified(long lastModified) {
        this.lastModified = lastModified;
    }

    public long getLastModified() {
        return lastModified;
    }

    public void setSize(long size) {
        this.size = size;
    }

    public void setEtag(String etag) {
        this.etag = etag;
    }

    public Long getSize() {
        return size;
    }

    public String getEtag() {
        return etag;
    }

    public Part(){}

    public Part(BSONObject record){
        if (record.get(Part.UPLOADID) != null){
            this.uploadId = (long) record.get(Part.UPLOADID);
        }
        if (record.get(Part.PARTNUMBER) != null){
            this.partNumber = (int) record.get(Part.PARTNUMBER);
        }
        if (record.get(Part.CSNAME) != null){
            this.csName = record.get(Part.CSNAME).toString();
        }
        if (record.get(Part.CLNAME) != null){
            this.clName = record.get(Part.CLNAME).toString();
        }
        if (record.get(Part.LOBID) != null){
            this.lobId = (ObjectId) record.get(Part.LOBID);
        }
        if (record.get(Part.LASTMODIFIED) != null){
            this.lastModified = (long) record.get(Part.LASTMODIFIED);
            this.lastModifiedDate = formatDate(this.lastModified);
        }
        if (record.get(Part.ETAG) != null){
            this.etag = record.get(Part.ETAG).toString();
        }
        if (record.get(Part.SIZE) != null){
            this.size = (long) record.get(Part.SIZE);
        }
    }

    public Part(BSONObject record, String encodingType) throws S3ServerException{
        try {
            if (record.get(Part.UPLOADID) != null){
                this.uploadId = (long) record.get(Part.UPLOADID);
            }
            if (record.get(Part.PARTNUMBER) != null){
                this.partNumber = (int) record.get(Part.PARTNUMBER);
            }
            if (record.get(Part.CSNAME) != null){
                this.csName = record.get(Part.CSNAME).toString();
            }
            if (record.get(Part.CLNAME) != null){
                this.clName = record.get(Part.CLNAME).toString();
            }
            if (record.get(Part.LOBID) != null){
                this.lobId = (ObjectId) record.get(Part.LOBID);
            }
            if (record.get(Part.LASTMODIFIED) != null){
                this.lastModified = (long) record.get(Part.LASTMODIFIED);
                this.lastModifiedDate = formatDate(this.lastModified);
            }
            if (encodingType != null) {
                this.etag = URLEncoder.encode("\"" + record.get(Part.ETAG).toString() +"\"", "UTF-8");
            }else {
                this.etag = "\"" + record.get(Part.ETAG).toString() + "\"";
            }
            if (record.get(Part.SIZE) != null){
                this.size = (long) record.get(Part.SIZE);
            }
        }catch (UnsupportedEncodingException e) {
            throw new S3ServerException(S3Error.UNKNOWN_ERROR, "URL encode failed", e);
        }

    }

    public Part(long uploadId, int partNumber, String csName,
                String clName, ObjectId lobId, long size, String eTag){
        this.uploadId = uploadId;
        this.partNumber = partNumber;
        this.csName = csName;
        this.clName = clName;
        this.lobId = lobId;
        this.size = size;
        this.etag = eTag;
        this.lastModified = System.currentTimeMillis();
    }

    public void setUploadId(Long uploadId) {
        this.uploadId = uploadId;
    }

    public Long getUploadId() {
        return uploadId;
    }

    public void setLobId(ObjectId lobId) {
        this.lobId = lobId;
    }

    public String getClName() {
        return clName;
    }

    public void setClName(String clName) {
        this.clName = clName;
    }

    public void setCsName(String csName) {
        this.csName = csName;
    }

    public void setPartNumber(int partNumber) {
        this.partNumber = partNumber;
    }

    public String getCsName() {
        return csName;
    }

    public int getPartNumber() {
        return partNumber;
    }

    public ObjectId getLobId() {
        return lobId;
    }
}
