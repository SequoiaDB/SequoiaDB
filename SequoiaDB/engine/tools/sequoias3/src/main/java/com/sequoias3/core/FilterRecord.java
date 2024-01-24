package com.sequoias3.core;

import com.sequoias3.model.*;

public class FilterRecord {
    public static final int FILTER_DIR          = 1;
    public static final int FILTER_DELIMITER    = 2;
    public static final int FILTER_NO_DELIMITER = 3;

    public static final int CONTENT      = 1;
    public static final int VERSION      = 2;
    public static final int DELETEMARKER = 3;
    public static final int COMMONPREFIX = 4;
    public static final int UPLOAD       = 5;

    private int recordType;
    private CommonPrefix commonPrefix;
    private Version version;
    private RawVersion deleteMarker;
    private Content content;
    private Upload upload;

    public void setRecordType(int recordType) {
        this.recordType = recordType;
    }

    public int getRecordType() {
        return recordType;
    }

    public void setDeleteMarker(RawVersion deleteMarker) {
        this.deleteMarker = deleteMarker;
    }

    public RawVersion getDeleteMarker() {
        return deleteMarker;
    }

    public void setCommonPrefix(CommonPrefix commonPrefix) {
        this.commonPrefix = commonPrefix;
    }

    public CommonPrefix getCommonPrefix() {
        return commonPrefix;
    }

    public void setVersion(Version version) {
        this.version = version;
    }

    public Version getVersion() {
        return version;
    }

    public void setContent(Content content) {
        this.content = content;
    }

    public Content getContent() {
        return content;
    }

    public void setUpload(Upload upload) {
        this.upload = upload;
    }

    public Upload getUpload() {
        return upload;
    }
}
