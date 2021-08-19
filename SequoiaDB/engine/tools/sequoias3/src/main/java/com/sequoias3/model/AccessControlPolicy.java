package com.sequoias3.model;

import com.fasterxml.jackson.annotation.JsonProperty;
import com.fasterxml.jackson.dataformat.xml.annotation.JacksonXmlElementWrapper;
import com.fasterxml.jackson.dataformat.xml.annotation.JacksonXmlRootElement;

import java.util.ArrayList;
import java.util.List;

@JacksonXmlRootElement(localName = "AccessControlPolicy")
public class AccessControlPolicy {
    @JsonProperty("Owner")
    private Owner owner;

    @JacksonXmlElementWrapper(localName = "AccessControlList")
    @JsonProperty("Grant")
    List<Grant> grants;

    public AccessControlPolicy(){
        grants = new ArrayList<>();
    }

    public void setOwner(Owner owner) {
        this.owner = owner;
    }

    public void setGrants(List<Grant> grants) {
        this.grants = grants;
    }

    public Owner getOwner() {
        return owner;
    }

    public List<Grant> getGrants() {
        return grants;
    }
}
