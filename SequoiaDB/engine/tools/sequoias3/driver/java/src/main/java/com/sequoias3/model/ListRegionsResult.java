package com.sequoias3.model;

import com.fasterxml.jackson.annotation.JsonProperty;
import com.fasterxml.jackson.dataformat.xml.annotation.JacksonXmlElementWrapper;
import com.fasterxml.jackson.dataformat.xml.annotation.JacksonXmlRootElement;

import java.util.List;

@JacksonXmlRootElement(localName = "ListAllRegionsResult")
public class ListRegionsResult extends Object{
    @JacksonXmlElementWrapper(localName = "Regions", useWrapping = false)
    @JsonProperty("Region")
    private List<String> regions;

    void setRegions(List<String> regions) {
        this.regions = regions;
    }

    /**
     *
     * @return A list of region name
     */
    public List<String> getRegions() {
        return regions;
    }
}
