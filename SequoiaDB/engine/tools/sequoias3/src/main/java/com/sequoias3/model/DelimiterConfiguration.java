package com.sequoias3.model;

import com.fasterxml.jackson.annotation.JsonProperty;
import com.fasterxml.jackson.dataformat.xml.annotation.JacksonXmlRootElement;
import com.sequoias3.exception.S3Error;
import com.sequoias3.exception.S3ServerException;

import java.io.UnsupportedEncodingException;
import java.net.URLEncoder;

@JacksonXmlRootElement(localName ="DelimiterConfiguration")
public class DelimiterConfiguration {
    @JsonProperty("Delimiter")
    private String delimiter;
    @JsonProperty("Status")
    private String status;

    public DelimiterConfiguration(){}

    public DelimiterConfiguration(String delimiter, String status, String encodingType) throws S3ServerException{
        try {
            if (encodingType != null) {
                this.delimiter = URLEncoder.encode(delimiter, "UTF-8");
            }else {
                this.delimiter = delimiter;
            }
            this.status = status;
        }catch (UnsupportedEncodingException e) {
            throw new S3ServerException(S3Error.BUCKET_DELIMITER_GET_FAILED, "encode delimiter failed", e);
        }
    }

    public String getDelimiter() {
        return delimiter;
    }

    public void setDelimiter(String delimiter) {
        this.delimiter = delimiter;
    }

    public void setStatus(String status) {
        this.status = status;
    }

    public String getStatus() {
        return status;
    }
}
