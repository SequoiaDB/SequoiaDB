/*
 * Copyright 2011-2014 the original author or authors.
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

import java.util.Collections;
import java.util.Set;

import org.bson.BSONObject;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.ExpectedException;
import org.springframework.beans.factory.config.BeanDefinition;
import org.springframework.beans.factory.config.BeanReference;
import org.springframework.beans.factory.parsing.BeanDefinitionParsingException;
import org.springframework.beans.factory.support.BeanDefinitionRegistry;
import org.springframework.beans.factory.support.DefaultListableBeanFactory;
import org.springframework.beans.factory.xml.XmlBeanDefinitionReader;
import org.springframework.core.convert.TypeDescriptor;
import org.springframework.core.convert.converter.Converter;
import org.springframework.core.convert.converter.GenericConverter;
import org.springframework.core.io.ClassPathResource;
import org.springframework.data.mapping.model.CamelCaseAbbreviatingFieldNamingStrategy;
import org.springframework.data.sequoiadb.core.convert.CustomConversions;
import org.springframework.data.sequoiadb.core.convert.MappingSequoiadbConverter;
import org.springframework.data.sequoiadb.core.convert.SequoiadbTypeMapper;
import org.springframework.data.sequoiadb.core.mapping.Account;
import org.springframework.data.sequoiadb.repository.Person;
import org.springframework.stereotype.Component;



/**
 * Integration tests for {@link MappingSequoiadbConverterParser}.
 * 




 */
public class MappingSdbConverterParserIntegrationTests {

	@Rule public ExpectedException exception = ExpectedException.none();

	DefaultListableBeanFactory factory;

	/**
	 * @see DATA_JIRA-243
	 */
	@Test
	public void allowsDbFactoryRefAttribute() {

		loadValidConfiguration();
		factory.getBeanDefinition("converter");
		factory.getBean("converter");
	}

	/**
	 * @see DATA_JIRA-725
	 */
	@Test
	public void hasCustomTypeMapper() {

		loadValidConfiguration();
		MappingSequoiadbConverter converter = factory.getBean("converter", MappingSequoiadbConverter.class);
		SequoiadbTypeMapper customSequoiadbTypeMapper = factory.getBean(CustomSequoiadbTypeMapper.class);

		assertThat(converter.getTypeMapper(), is(customSequoiadbTypeMapper));
	}

	/**
	 * @see DATA_JIRA-301
	 */
	@Test
	public void scansForConverterAndSetsUpCustomConversionsAccordingly() {

		loadValidConfiguration();
		CustomConversions conversions = factory.getBean(CustomConversions.class);
		assertThat(conversions.hasCustomWriteTarget(Person.class), is(true));
		assertThat(conversions.hasCustomWriteTarget(Account.class), is(true));
	}

	/**
	 * @see DATA_JIRA-607
	 */
	@Test
	public void activatesAbbreviatingPropertiesCorrectly() {

		loadValidConfiguration();
		BeanDefinition definition = factory.getBeanDefinition("abbreviatingConverter.sequoiadbMappingContext");
		Object value = definition.getPropertyValues().getPropertyValue("fieldNamingStrategy").getValue();

		assertThat(value, is(instanceOf(BeanDefinition.class)));
		BeanDefinition strategy = (BeanDefinition) value;
		assertThat(strategy.getBeanClassName(), is(CamelCaseAbbreviatingFieldNamingStrategy.class.getName()));
	}

	/**
	 * @see DATA_JIRA-866
	 */
	@Test
	public void rejectsInvalidFieldNamingStrategyConfiguration() {

		exception.expect(BeanDefinitionParsingException.class);
		exception.expectMessage("abbreviation");
		exception.expectMessage("field-naming-strategy-ref");

		BeanDefinitionRegistry factory = new DefaultListableBeanFactory();
		XmlBeanDefinitionReader reader = new XmlBeanDefinitionReader(factory);
		reader.loadBeanDefinitions(new ClassPathResource("namespace/converter-invalid.xml"));
	}

	/**
	 * @see DATA_JIRA-892
	 */
	@Test
	public void shouldThrowBeanDefinitionParsingExceptionIfConverterDefinedAsNestedBean() {

		exception.expect(BeanDefinitionParsingException.class);
		exception.expectMessage("Sdb Converter must not be defined as nested bean.");

		loadNestedBeanConfiguration();
	}

	/**
	 * @see DATA_JIRA-925, DATA_JIRA-928
	 */
	@Test
	public void shouldSupportCustomFieldNamingStrategy() {
		assertStrategyReferenceSetFor("mappingConverterWithCustomFieldNamingStrategy");
	}

	/**
	 * @see DATA_JIRA-925, DATA_JIRA-928
	 */
	@Test
	public void shouldNotFailLoadingConfigIfAbbreviationIsDisabledAndStrategySet() {
		assertStrategyReferenceSetFor("mappingConverterWithCustomFieldNamingStrategyAndAbbreviationDisabled");
	}

	private void loadValidConfiguration() {
		this.loadConfiguration("namespace/converter.xml");
	}

	private void loadNestedBeanConfiguration() {
		this.loadConfiguration("namespace/converter-nested-bean-definition.xml");
	}

	private void loadConfiguration(String configLocation) {
		factory = new DefaultListableBeanFactory();
		XmlBeanDefinitionReader reader = new XmlBeanDefinitionReader(factory);
		reader.loadBeanDefinitions(new ClassPathResource(configLocation));
	}

	private static void assertStrategyReferenceSetFor(String beanId) {

		BeanDefinitionRegistry factory = new DefaultListableBeanFactory();
		XmlBeanDefinitionReader reader = new XmlBeanDefinitionReader(factory);
		reader.loadBeanDefinitions(new ClassPathResource("namespace/converter-custom-fieldnamingstrategy.xml"));

		BeanDefinition definition = reader.getRegistry().getBeanDefinition(beanId.concat(".sequoiadbMappingContext"));
		BeanReference value = (BeanReference) definition.getPropertyValues().getPropertyValue("fieldNamingStrategy")
				.getValue();

		assertThat(value.getBeanName(), is("customFieldNamingStrategy"));
	}

	@Component
	public static class SampleConverter implements Converter<Person, BSONObject> {
		public BSONObject convert(Person source) {
			return null;
		}
	}

	@Component
	public static class SampleConverterFactory implements GenericConverter {

		public Set<ConvertiblePair> getConvertibleTypes() {
			return Collections.singleton(new ConvertiblePair(Account.class, BSONObject.class));
		}

		public Object convert(Object source, TypeDescriptor sourceType, TypeDescriptor targetType) {
			return null;
		}
	}
}
