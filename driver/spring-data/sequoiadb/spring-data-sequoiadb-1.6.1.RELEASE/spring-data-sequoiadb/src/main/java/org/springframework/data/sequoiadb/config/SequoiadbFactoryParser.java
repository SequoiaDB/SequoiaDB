/*
 * Copyright 2011-2014 by the original author(s).
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package org.springframework.data.sequoiadb.config;

import static org.springframework.data.config.ParsingUtils.*;
import static org.springframework.data.sequoiadb.config.SequoiadbParsingUtils.*;

import org.springframework.beans.factory.BeanDefinitionStoreException;
import org.springframework.beans.factory.config.BeanDefinition;
import org.springframework.beans.factory.parsing.BeanComponentDefinition;
import org.springframework.beans.factory.support.AbstractBeanDefinition;
import org.springframework.beans.factory.support.BeanDefinitionBuilder;
import org.springframework.beans.factory.xml.AbstractBeanDefinitionParser;
import org.springframework.beans.factory.xml.BeanDefinitionParser;
import org.springframework.beans.factory.xml.ParserContext;
import org.springframework.data.authentication.UserCredentials;
import org.springframework.data.config.BeanComponentDefinitionBuilder;
import org.springframework.data.sequoiadb.assist.Sdb;
import org.springframework.data.sequoiadb.assist.SequoiadbURI;
import org.springframework.data.sequoiadb.core.SequoiadbFactoryBean;
import org.springframework.data.sequoiadb.core.SimpleSequoiadbFactory;
import org.springframework.util.StringUtils;
import org.w3c.dom.Element;

/**
 * {@link BeanDefinitionParser} to parse {@code db-factory} elements into {@link BeanDefinition}s.
 * 



 */
public class SequoiadbFactoryParser extends AbstractBeanDefinitionParser {

	/* 
	 * (non-Javadoc)
	 * @see org.springframework.beans.factory.xml.AbstractBeanDefinitionParser#resolveId(org.w3c.dom.Element, org.springframework.beans.factory.support.AbstractBeanDefinition, org.springframework.beans.factory.xml.ParserContext)
	 */
	@Override
	protected String resolveId(Element element, AbstractBeanDefinition definition, ParserContext parserContext)
			throws BeanDefinitionStoreException {

		String id = super.resolveId(element, definition, parserContext);
		return StringUtils.hasText(id) ? id : BeanNames.DB_FACTORY_BEAN_NAME;
	}

	/* 
	 * (non-Javadoc)
	 * @see org.springframework.beans.factory.xml.AbstractBeanDefinitionParser#parseInternal(org.w3c.dom.Element, org.springframework.beans.factory.xml.ParserContext)
	 */
	@Override
	protected AbstractBeanDefinition parseInternal(Element element, ParserContext parserContext) {

		Object source = parserContext.extractSource(element);

		BeanComponentDefinitionBuilder helper = new BeanComponentDefinitionBuilder(element, parserContext);

		String uri = element.getAttribute("uri");
		String sequoiadbRef = element.getAttribute("sdb-ref");
		String dbname = element.getAttribute("dbname");

		BeanDefinition userCredentials = getUserCredentialsBeanDefinition(element, parserContext);

		BeanDefinitionBuilder dbFactoryBuilder = BeanDefinitionBuilder.genericBeanDefinition(SimpleSequoiadbFactory.class);
		setPropertyValue(dbFactoryBuilder, element, "write-concern", "writeConcern");

		if (StringUtils.hasText(uri)) {
			if (StringUtils.hasText(sequoiadbRef) || StringUtils.hasText(dbname) || userCredentials != null) {
				parserContext.getReaderContext().error("Configure either Sdb URI or details individually!", source);
			}

			dbFactoryBuilder.addConstructorArgValue(getSequoiadbUri(uri));
			return getSourceBeanDefinition(dbFactoryBuilder, parserContext, element);
		}

		if (StringUtils.hasText(sequoiadbRef)) {
			dbFactoryBuilder.addConstructorArgReference(sequoiadbRef);
		} else {
			dbFactoryBuilder.addConstructorArgValue(registerSequoiadbBeanDefinition(element, parserContext));
		}

		dbFactoryBuilder.addConstructorArgValue(StringUtils.hasText(dbname) ? dbname : "db");
		dbFactoryBuilder.addConstructorArgValue(userCredentials);
		dbFactoryBuilder.addConstructorArgValue(element.getAttribute("authentication-dbname"));

		BeanDefinitionBuilder writeConcernPropertyEditorBuilder = getWriteConcernPropertyEditorBuilder();

		BeanComponentDefinition component = helper.getComponent(writeConcernPropertyEditorBuilder);
		parserContext.registerBeanComponent(component);

		return (AbstractBeanDefinition) helper.getComponentIdButFallback(dbFactoryBuilder, BeanNames.DB_FACTORY_BEAN_NAME)
				.getBeanDefinition();
	}

	/**
	 * Registers a default {@link BeanDefinition} of a {@link Sdb} instance and returns the name under which the
	 * {@link Sdb} instance was registered under.
	 * 
	 * @param element must not be {@literal null}.
	 * @param parserContext must not be {@literal null}.
	 * @return
	 */
	private BeanDefinition registerSequoiadbBeanDefinition(Element element, ParserContext parserContext) {

		BeanDefinitionBuilder sequoiadbBuilder = BeanDefinitionBuilder.genericBeanDefinition(SequoiadbFactoryBean.class);
		setPropertyValue(sequoiadbBuilder, element, "host");
		setPropertyValue(sequoiadbBuilder, element, "port");

		return getSourceBeanDefinition(sequoiadbBuilder, parserContext, element);
	}

	/**
	 * Returns a {@link BeanDefinition} for a {@link UserCredentials} object.
	 * 
	 * @param element
	 * @return the {@link BeanDefinition} or {@literal null} if neither username nor password given.
	 */
	private BeanDefinition getUserCredentialsBeanDefinition(Element element, ParserContext context) {

		String username = element.getAttribute("username");
		String password = element.getAttribute("password");

		if (!StringUtils.hasText(username) && !StringUtils.hasText(password)) {
			return null;
		}

		BeanDefinitionBuilder userCredentialsBuilder = BeanDefinitionBuilder.genericBeanDefinition(UserCredentials.class);
		userCredentialsBuilder.addConstructorArgValue(StringUtils.hasText(username) ? username : null);
		userCredentialsBuilder.addConstructorArgValue(StringUtils.hasText(password) ? password : null);

		return getSourceBeanDefinition(userCredentialsBuilder, context, element);
	}

	/**
	 * Creates a {@link BeanDefinition} for a {@link SequoiadbURI}.
	 * 
	 * @param uri
	 * @return
	 */
	private BeanDefinition getSequoiadbUri(String uri) {

		BeanDefinitionBuilder builder = BeanDefinitionBuilder.genericBeanDefinition(SequoiadbURI.class);
		builder.addConstructorArgValue(uri);

		return builder.getBeanDefinition();
	}
}
