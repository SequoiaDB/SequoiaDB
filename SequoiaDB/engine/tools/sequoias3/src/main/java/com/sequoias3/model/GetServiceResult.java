package com.sequoias3.model;

import com.fasterxml.jackson.annotation.JsonProperty;
import com.fasterxml.jackson.annotation.JsonPropertyOrder;
import com.fasterxml.jackson.dataformat.xml.annotation.JacksonXmlElementWrapper;
import com.fasterxml.jackson.dataformat.xml.annotation.JacksonXmlRootElement;
import com.sequoias3.core.Bucket;
import com.sequoias3.core.User;

import java.util.List;

@JacksonXmlRootElement(localName = "ListAllMyBucketsResult")
@JsonPropertyOrder({"owner","buckets"})
public class GetServiceResult {

    @JacksonXmlElementWrapper()
    @JsonProperty("Owner")
    private User owner;

    @JacksonXmlElementWrapper(localName = "Buckets")
    @JsonProperty("Bucket")
    private List<Bucket> buckets;

    public void setBuckets(List<Bucket> buckets) {
        this.buckets = buckets;
    }

    public List<Bucket> getBuckets() {
        return buckets;
    }

    public User getOwner() {
        return owner;
    }

    public void setOwner(User owner) {
        this.owner = owner;
    }
}
