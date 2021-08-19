package com.sequoias3;

import org.apache.catalina.connector.Connector;
import org.apache.coyote.AbstractProtocol;
import org.springframework.boot.context.embedded.tomcat.TomcatConnectorCustomizer;

public class CustomConnector implements TomcatConnectorCustomizer {
    @Override
    public void customize(Connector connector) {
        ((AbstractProtocol) connector.getProtocolHandler()).setSendReasonPhrase(true);
    }
}
