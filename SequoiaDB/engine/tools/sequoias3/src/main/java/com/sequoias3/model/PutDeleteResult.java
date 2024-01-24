package com.sequoias3.model;

public class PutDeleteResult {
    private String  eTag;
    private String  versionId;
    private Boolean deleteMarker;

    public void seteTag(String eTag) {
        this.eTag = eTag;
    }

    public String geteTag() {
        return eTag;
    }

    public void setVersionId(String versionId) {
        this.versionId = versionId;
    }

    public String getVersionId() {
        return versionId;
    }

    public void setDeleteMarker(Boolean deleteMarker) {
        this.deleteMarker = deleteMarker;
    }

    public Boolean getDeleteMarker(){
        return this.deleteMarker;
    }
}
