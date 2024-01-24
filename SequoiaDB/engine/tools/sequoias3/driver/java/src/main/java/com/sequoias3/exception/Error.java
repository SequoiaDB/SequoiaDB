package com.sequoias3.exception;

import com.fasterxml.jackson.annotation.JsonProperty;
import com.fasterxml.jackson.dataformat.xml.annotation.JacksonXmlRootElement;

@JacksonXmlRootElement(localName = "Error")
public class Error {
    public final static String JSON_CODE = "Code";
    public final static String JSON_MESSAGE = "Message";
    public final static String JSON_RESOURCE = "Resource";

    @JsonProperty(JSON_CODE)
    private String code;
    @JsonProperty(JSON_MESSAGE)
    private String message;
    @JsonProperty(JSON_RESOURCE)
    private String resource;

    public String getCode() {
        return code;
    }

    public void setCode(String code) {
        this.code = code;
    }

    public String getMessage() {
        return message;
    }

    public void setMessage(String message) {
        this.message = message;
    }

    public String getResource() {
        return resource;
    }

    public void setResource(String resource) {
        this.resource = resource;
    }

    @Override
    public String toString() {
        return String
                .format("{\"Code\":%s,\"Message\":\"%s\",\"Resource\":\"%s\"}",
                        code, message, resource);
    }
}
