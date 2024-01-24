package com.sequoias3.model;

import com.fasterxml.jackson.annotation.JsonIgnore;
import com.fasterxml.jackson.annotation.JsonProperty;
import com.fasterxml.jackson.dataformat.xml.annotation.JacksonXmlRootElement;

@JacksonXmlRootElement(localName = "CopyObjectResult")
public class CopyObjectResult {
    @JsonProperty("ETag")
    private String eTag;
    @JsonProperty("LastModified")
    private String lastModified;

    @JsonIgnore
    private Long versionId;

    @JsonIgnore
    private Long sourceVersionId;

    public void seteTag(String eTag) {
        this.eTag = eTag;
    }

    public String geteTag() {
        return eTag;
    }

    public void setLastModified(String lastModified) {
        this.lastModified = lastModified;
    }

    public String getLastModified() {
        return lastModified;
    }

    public void setVersionId(Long versionId) {
        this.versionId = versionId;
    }

    public Long getVersionId() {
        return versionId;
    }

    public void setSourceVersionId(Long sourceVersionId) {
        this.sourceVersionId = sourceVersionId;
    }

    public Long getSourceVersionId() {
        return sourceVersionId;
    }
}
