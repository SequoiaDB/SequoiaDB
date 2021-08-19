package com.sequoias3.model;

import com.fasterxml.jackson.annotation.JsonProperty;
import com.fasterxml.jackson.dataformat.xml.annotation.JacksonXmlElementWrapper;
import com.fasterxml.jackson.dataformat.xml.annotation.JacksonXmlRootElement;
import com.sequoias3.core.Bucket;
import com.sequoias3.core.Region;

import java.util.List;

@JacksonXmlRootElement(localName = "RegionConfiguration")
public class GetRegionResult extends Region{
    public GetRegionResult(Region region){
        super(region);
    }

//    public GetRegionResult(){}

    @JacksonXmlElementWrapper(localName = "Buckets")
    @JsonProperty("Bucket")
    List<Bucket> Buckets;


    public void setBuckets(List<Bucket> buckets) {
        Buckets = buckets;
    }
}
