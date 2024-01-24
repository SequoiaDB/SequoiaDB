package com.sequoias3.model;

import com.fasterxml.jackson.annotation.JsonInclude;
import com.fasterxml.jackson.annotation.JsonProperty;
import com.fasterxml.jackson.dataformat.xml.annotation.JacksonXmlElementWrapper;
import com.fasterxml.jackson.dataformat.xml.annotation.JacksonXmlRootElement;

import java.util.ArrayList;
import java.util.List;

@JacksonXmlRootElement(localName = "DeleteResult")
public class DeleteObjectsResult {
    @JacksonXmlElementWrapper(localName = "Deleted", useWrapping = false)
    @JsonProperty("Deleted")
    private List<ObjectDeleted> deletedObjects;

    @JacksonXmlElementWrapper(localName = "Error", useWrapping = false)
    @JsonProperty("Error")
    private List<DeleteError> errors;

    public DeleteObjectsResult(){
        deletedObjects = new ArrayList<ObjectDeleted>();
        errors = new ArrayList<DeleteError>();
    }

    public void setDeletedObjects(List<ObjectDeleted> deletedObjects) {
        this.deletedObjects = deletedObjects;
    }

    public List<ObjectDeleted> getDeletedObjects() {
        return deletedObjects;
    }

    public void setErrors(List<DeleteError> errors) {
        this.errors = errors;
    }

    public List<DeleteError> getErrors() {
        return errors;
    }
}
