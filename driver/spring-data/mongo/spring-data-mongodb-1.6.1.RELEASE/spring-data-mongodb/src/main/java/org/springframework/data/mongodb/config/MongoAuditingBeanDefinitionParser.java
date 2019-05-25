/*
 * Copyright 2012-2014 the original author or authors.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package org.springframework.data.mongodb.config;

import static org.springframework.data.config.ParsingUtils.*;
import static org.springframework.data.mongodb.config.BeanNames.*;

import org.springframework.beans.factory.support.BeanDefinitionBuilder;
import org.springframework.beans.factory.support.BeanDefinitionRegistry;
import org.springframework.beans.factory.support.RootBeanDefinition;
import org.springframework.beans.factory.xml.AbstractSingleBeanDefinitionParser;
import org.springframework.beans.factory.xml.BeanDefinitionParser;
import org.springframework.beans.factory.xml.ParserContext;
import org.springframework.data.auditing.config.IsNewAwareAuditingHandlerBeanDefinitionParser;
import org.springframework.data.mongodb.core.mapping.MongoMappingContext;
import org.springframework.data.mongodb.core.mapping.event.AuditingEventListener;
import org.springframework.util.StringUtils;
import org.w3c.dom.Element;

/**
 * {@link BeanDefinitionParser} to register a {@link AuditingEventListener} to transparently set auditing information on
 * an entity.
 * 
 * @author Oliver Gierke
 */
public class MongoAuditingBeanDefinitionParser extends AbstractSingleBeanDefinitionParser {

	/* 
	 * (non-Javadoc)
	 * @see org.springframework.beans.factory.xml.AbstractSingleBeanDefinitionParser#getBeanClass(org.w3c.dom.Element)
	 */
	@Override
	protected Class<?> getBeanClass(Element element) {
		return AuditingEventListener.class;
	}

	/* 
	 * (non-Javadoc)
	 * @see org.springframework.beans.factory.xml.AbstractBeanDefinitionParser#shouldGenerateId()
	 */
	@Override
	protected boolean shouldGenerateId() {
		return true;
	}

	/* 
	 * (non-Javadoc)
	 * @see org.springframework.beans.factory.xml.AbstractSingleBeanDefinitionParser#doParse(org.w3c.dom.Element, org.springframework.beans.factory.xml.ParserContext, org.springframework.beans.factory.support.BeanDefinitionBuilder)
	 */
	@Override
	protected void doParse(Element element, ParserContext parserContext, BeanDefinitionBuilder builder) {

		String mappingContextRef = element.getAttribute("mapping-context-ref");

		if (!StringUtils.hasText(mappingContextRef)) {

			BeanDefinitionRegistry registry = parserContext.getRegistry();

			if (!registry.containsBeanDefinition(MAPPING_CONTEXT_BEAN_NAME)) {
				registry.registerBeanDefinition(MAPPING_CONTEXT_BEAN_NAME, new RootBeanDefinition(MongoMappingContext.class));
			}

			mappingContextRef = MAPPING_CONTEXT_BEAN_NAME;
		}

		IsNewAwareAuditingHandlerBeanDefinitionParser parser = new IsNewAwareAuditingHandlerBeanDefinitionParser(
				mappingContextRef);
		parser.parse(element, parserContext);

		builder.addConstructorArgValue(getObjectFactoryBeanDefinition(parser.getResolvedBeanName(),
				parserContext.extractSource(element)));
	}
}
