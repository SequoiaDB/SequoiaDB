package com.sequoias3.core;

import com.fasterxml.jackson.annotation.JsonInclude;
import com.fasterxml.jackson.annotation.JsonProperty;
import com.sequoias3.exception.S3ServerException;

@JsonInclude(value = JsonInclude.Include.NON_NULL)
public class Error {
    public final static String JSON_CODE = "Code";
    public final static String JSON_MESSAGE = "Message";
    public final static String JSON_RESOURCE = "Resource";
    public final static String JSON_ERRDESCRIPTION = "ErrDescription";

    @JsonProperty(JSON_CODE)
    private String code;
    @JsonProperty(JSON_MESSAGE)
    private String message;
    @JsonProperty(JSON_RESOURCE)
    private String resource;
    @JsonProperty(JSON_ERRDESCRIPTION)
    private String errDescription;

    public Error(S3ServerException e, String path) {
        this.code = e.getError().getCode();
        this.message = e.getError().getErrorMessage();
        this.resource = path;
        //this.errDescription = e.getMessage();
        //this.requestId = System.currentTimeMillis();
    }

    public Error(Exception e, String path) {
        this.code = "INTERNAL_SERVER_ERROR";
        this.message = e.getMessage();
        this.resource = path;
        //this.requestId = System.currentTimeMillis();
    }

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

    public String getErrDescription() {
        return errDescription;
    }

    public void setErrDescription(String errDescription) {
        this.errDescription = errDescription;
    }

    @Override
    public String toString() {
        return String
                .format("{\"Code\":%s,\"Message\":\"%s\",\"Resource\":\"%s\"}",
                        code, message, resource);
    }
}
