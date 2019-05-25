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
package org.springframework.data.sequoiadb.config;

import static org.hamcrest.Matchers.*;
import static org.junit.Assert.*;

import org.junit.Before;
import org.junit.Test;
import org.springframework.beans.factory.NoSuchBeanDefinitionException;
import org.springframework.beans.factory.support.BeanDefinitionReader;
import org.springframework.beans.factory.support.DefaultListableBeanFactory;
import org.springframework.beans.factory.xml.XmlBeanDefinitionReader;
import org.springframework.core.io.ClassPathResource;
import org.springframework.data.sequoiadb.core.mapping.event.ValidatingSequoiadbEventListener;

/**
 * Integration test for creation of instance of
 * {@link ValidatingSequoiadbEventListener} by defining
 * {@code <sdb:mapping-converter />} in context XML.
 * 
 * @see DATA_JIRA-36



 */
public class MappingSdbConverterParserValidationIntegrationTests {

	DefaultListableBeanFactory factory;
	BeanDefinitionReader reader;

	@Before
	public void setUp() {
		factory = new DefaultListableBeanFactory();
		reader = new XmlBeanDefinitionReader(factory);
	}

	/**
	 * @see DATA_JIRA-36
	 */
	@Test
	public void validatingEventListenerCreatedWithDefaultConfig() {

		reader.loadBeanDefinitions(new ClassPathResource("namespace/converter-default.xml"));
		assertThat(factory.getBean(BeanNames.VALIDATING_EVENT_LISTENER_BEAN_NAME), is(not(nullValue())));
	}

	/**
	 * @see DATA_JIRA-36
	 */
	@Test
	public void validatingEventListenerCreatedWhenValidationEnabled() {

		reader.loadBeanDefinitions(new ClassPathResource("namespace/converter-validation-enabled.xml"));
		assertThat(factory.getBean(BeanNames.VALIDATING_EVENT_LISTENER_BEAN_NAME), is(not(nullValue())));
	}

	/**
	 * @see DATA_JIRA-36
	 */
	@Test(expected = NoSuchBeanDefinitionException.class)
	public void validatingEventListenersIsNotCreatedWhenDisabled() {

		reader.loadBeanDefinitions(new ClassPathResource("namespace/converter-validation-disabled.xml"));
		factory.getBean(BeanNames.VALIDATING_EVENT_LISTENER_BEAN_NAME);
	}

	/**
	 * @see DATA_JIRA-36
	 */
	@Test
	public void validatingEventListenerCreatedWithCustomTypeMapperConfig() {

		reader.loadBeanDefinitions(new ClassPathResource("namespace/converter-custom-typeMapper.xml"));
		assertThat(factory.getBean(BeanNames.VALIDATING_EVENT_LISTENER_BEAN_NAME), is(not(nullValue())));
	}
}
