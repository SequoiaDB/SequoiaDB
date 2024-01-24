package com.sequoias3.model;

import com.fasterxml.jackson.annotation.JsonProperty;
import com.fasterxml.jackson.dataformat.xml.annotation.JacksonXmlElementWrapper;
import com.fasterxml.jackson.dataformat.xml.annotation.JacksonXmlRootElement;

import java.util.List;

@JacksonXmlRootElement(localName = "Delete")
public class DeleteObjects {
    @JacksonXmlElementWrapper(localName = "Object", useWrapping = false)
    @JsonProperty("Object")
    private List<ObjectToDel> objects;

    @JsonProperty("Quiet")
    private Boolean isQuiet = false;

    public void setObjects(List<ObjectToDel> objects) {
        this.objects = objects;
    }

    public List<ObjectToDel> getObjects() {
        return objects;
    }

    public void setQuiet(Boolean quiet) {
        isQuiet = quiet;
    }

    public Boolean getQuiet() {
        return isQuiet;
    }
}
