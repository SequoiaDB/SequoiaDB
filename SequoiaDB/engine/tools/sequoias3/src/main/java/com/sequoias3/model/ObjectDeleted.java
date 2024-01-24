package com.sequoias3.model;

import com.fasterxml.jackson.annotation.JsonInclude;
import com.fasterxml.jackson.annotation.JsonProperty;

@JsonInclude(value = JsonInclude.Include.NON_NULL)
public class ObjectDeleted {
    @JsonProperty("Key")
    private String key;
    @JsonProperty("VersionId")
    private String versionId;
    @JsonProperty("DeleteMarker")
    private Boolean deleteMarker;
    @JsonProperty("DeleteMarkerVersionId")
    private String deleteMarkerVersion;

    public void setDeleteMarker(Boolean deleteMarker) {
        this.deleteMarker = deleteMarker;
    }

    public Boolean getDeleteMarker() {
        return deleteMarker;
    }

    public void setDeleteMarkerVersion(String deleteMarkerVersion) {
        this.deleteMarkerVersion = deleteMarkerVersion;
    }

    public String getDeleteMarkerVersion() {
        return deleteMarkerVersion;
    }

    public void setKey(String key) {
        this.key = key;
    }

    public String getKey() {
        return key;
    }

    public void setVersionId(String versionId) {
        this.versionId = versionId;
    }

    public String getVersionId() {
        return versionId;
    }
}
