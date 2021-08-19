package com.sequoias3.model;

import com.fasterxml.jackson.annotation.JsonProperty;
import com.fasterxml.jackson.dataformat.xml.annotation.JacksonXmlElementWrapper;
import com.fasterxml.jackson.dataformat.xml.annotation.JacksonXmlRootElement;
import com.sequoias3.core.Part;

import java.util.List;

@JacksonXmlRootElement(localName = "CompleteMultipartUpload")
public class CompleteMultipartUpload {
    @JacksonXmlElementWrapper(localName = "Part", useWrapping = false)
    @JsonProperty("Part")
    private List<Part> part;

    public void setPart(List<Part> part) {
        this.part = part;
    }

    public List<Part> getPart() {
        return part;
    }
}
