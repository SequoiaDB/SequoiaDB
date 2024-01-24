package com.sequoias3.core;

public class Dir {
    public static final String DIR_BUCKETID    = "BucketId";
    public static final String DIR_DELIMITER   = "Delimiter";
    public static final String DIR_NAME        = "Name";
    public static final String DIR_ID          = "ID";

    public static final String DIR_INDEX       = "dirIndex";

    private Long bucketId;
    private String delimiter;
    private String name;
    private Long ID;
//    private Boolean deleteMarker;

    public Dir(Long bucketId, String delimiter, String name, Long ID){
        this.bucketId     = bucketId;
        this.delimiter    = delimiter;
        this.name         = name;
        this.ID           = ID;
//        this.deleteMarker = deleteMarker;
    }

    public void setBucketId(Long bucketId) {
        this.bucketId = bucketId;
    }

    public Long getBucketId() {
        return bucketId;
    }

    public void setDelimiter(String delimiter) {
        this.delimiter = delimiter;
    }

    public String getDelimiter() {
        return delimiter;
    }

    public void setName(String name) {
        this.name = name;
    }

    public String getName() {
        return name;
    }

    public void setID(Long ID) {
        this.ID = ID;
    }

    public Long getID() {
        return ID;
    }

//    public void setDeleteMarker(Boolean deleteMarker) {
//        this.deleteMarker = deleteMarker;
//    }

//    public Boolean getDeleteMarker() {
//        return deleteMarker;
//    }
}
