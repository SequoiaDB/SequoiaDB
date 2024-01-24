package com.sequoias3.model;

import com.fasterxml.jackson.annotation.JsonProperty;
import com.fasterxml.jackson.dataformat.xml.annotation.JacksonXmlRootElement;

@JacksonXmlRootElement(localName = "CreateBucketConfiguration")
public class CreateBucketConfiguration {

    private String locationConstraint;

    public void setLocationConstraint(String locationConstraint) {
        this.locationConstraint = locationConstraint;
    }

    @JsonProperty("LocationConstraint")
    public String getLocationConstraint() {
        return locationConstraint;
    }
}
