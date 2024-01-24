package com.sequoias3.core;

import com.fasterxml.jackson.annotation.JsonProperty;
import com.fasterxml.jackson.dataformat.xml.annotation.JacksonXmlElementWrapper;

import java.util.List;

public class InnerGetRegionResponse extends InnerRegion {
    @JacksonXmlElementWrapper(localName = "Buckets")
    @JsonProperty("Bucket")
    private List<InnerBucket> Buckets;

    public void setBuckets(List<InnerBucket> buckets) {
        Buckets = buckets;
    }

    public List<InnerBucket> getBuckets() {
        return Buckets;
    }

}
