package com.sequoias3.model;

import com.fasterxml.jackson.annotation.JsonInclude;
import com.fasterxml.jackson.annotation.JsonProperty;
import com.fasterxml.jackson.dataformat.xml.annotation.JacksonXmlProperty;

@JsonInclude(value = JsonInclude.Include.NON_NULL)
public class Grantee {
    @JacksonXmlProperty(isAttribute = true, localName = "xmlns:xsi")
    private String xsi = "http://www.w3.org/2001/XMLSchema-instance";

    @JacksonXmlProperty(isAttribute = true, localName = "xsi:type")
    private String xsiType;

    @JacksonXmlProperty(isAttribute = true)
    private String type;

    @JsonProperty("ID")
    private Long id;
    @JsonProperty("DisplayName")
    private String displayName;
    @JsonProperty("URI")
    private String uri;
    @JsonProperty("EmailAddress")
    private String emailAddress;

    public Grantee(){}

    public Grantee(String type, Long id, String displayName, String uri, String emailAddress){
        this.type = type;
        this.xsiType = type;
        this.id = id;
        this.displayName = displayName;
        this.uri = uri;
        this.emailAddress = emailAddress;
    }

    public void setId(Long id) {
        this.id = id;
    }

    public void setDisplayName(String displayName) {
        this.displayName = displayName;
    }

    public void setEmailAddress(String emailAddress) {
        this.emailAddress = emailAddress;
    }

    public void setUri(String uri) {
        this.uri = uri;
    }

    public String getDisplayName() {
        return displayName;
    }

    public String getEmailAddress() {
        return emailAddress;
    }

    public Long getId() {
        return id;
    }

    public String getUri() {
        return uri;
    }

    public void setType(String type) {
        this.type = type;
    }

    public String getType() {
        return type;
    }

    public void setXsiType(String xsiType) {
        this.xsiType = xsiType;
    }

    public String getXsiType() {
        return xsiType;
    }
}
