package com.sequoias3.commlibs3.s3utils.bean;

import java.util.List;

import com.fasterxml.jackson.annotation.JsonProperty;
import com.fasterxml.jackson.dataformat.xml.annotation.JacksonXmlElementWrapper;
import com.fasterxml.jackson.dataformat.xml.annotation.JacksonXmlRootElement;

@JacksonXmlRootElement(localName = "ListAllRegionsResult")
public class ListRegionResult {
    @JacksonXmlElementWrapper(localName = "Regions", useWrapping = false)
    @JsonProperty("Region")
    private List<String> regions;

    public void setRegions(List<String> regions) {
        this.regions = regions;
    }

    public List<String> getRegions() {
        return this.regions;
    }
}
