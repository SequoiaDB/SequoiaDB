package com.sequoias3.core.serial;

import com.fasterxml.jackson.core.JsonGenerator;
import com.fasterxml.jackson.databind.SerializerProvider;
import com.fasterxml.jackson.databind.ser.std.StdSerializer;
import com.sequoias3.core.Error;

import java.io.IOException;

public class ErrorSerializer extends StdSerializer<Error> {
    private static final long serialVersionUID = 5087465916766420390L;

    public ErrorSerializer() {
        super(Error.class);
    }

    @Override
    public void serialize(Error error, JsonGenerator gen, SerializerProvider provider)
            throws IOException {
        gen.writeStartObject();
        gen.writeStringField(Error.JSON_CODE, error.getCode());
        gen.writeStringField(Error.JSON_MESSAGE, error.getMessage());
        gen.writeStringField(Error.JSON_RESOURCE, error.getResource());
        //gen.writeNumberField(Error.JSON_REQUESTID, error.getRequestId());
        gen.writeEndObject();
    }
}
