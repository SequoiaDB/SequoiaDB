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
package org.springframework.data.mongodb.config;

import static org.hamcrest.Matchers.*;
import static org.junit.Assert.*;

import java.util.Collections;
import java.util.Set;

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
import org.springframework.data.mongodb.core.convert.CustomConversions;
import org.springframework.data.mongodb.core.convert.MappingMongoConverter;
import org.springframework.data.mongodb.core.convert.MongoTypeMapper;
import org.springframework.data.mongodb.core.mapping.Account;
import org.springframework.data.mongodb.repository.Person;
import org.springframework.stereotype.Component;

import org.springframework.data.mongodb.assist.DBObject;

/**
 * Integration tests for {@link MappingMongoConverterParser}.
 * 
 * @author Oliver Gierke
 * @author Thomas Darimont
 * @author Christoph Strobl
 * @author Ryan Tenney
 */
public class MappingMongoConverterParserIntegrationTests {

	@Rule public ExpectedException exception = ExpectedException.none();

	DefaultListableBeanFactory factory;

	/**
	 * @see DATAMONGO-243
	 */
	@Test
	public void allowsDbFactoryRefAttribute() {

		loadValidConfiguration();
		factory.getBeanDefinition("converter");
		factory.getBean("converter");
	}

	/**
	 * @see DATAMONGO-725
	 */
	@Test
	public void hasCustomTypeMapper() {

		loadValidConfiguration();
		MappingMongoConverter converter = factory.getBean("converter", MappingMongoConverter.class);
		MongoTypeMapper customMongoTypeMapper = factory.getBean(CustomMongoTypeMapper.class);

		assertThat(converter.getTypeMapper(), is(customMongoTypeMapper));
	}

	/**
	 * @see DATAMONGO-301
	 */
	@Test
	public void scansForConverterAndSetsUpCustomConversionsAccordingly() {

		loadValidConfiguration();
		CustomConversions conversions = factory.getBean(CustomConversions.class);
		assertThat(conversions.hasCustomWriteTarget(Person.class), is(true));
		assertThat(conversions.hasCustomWriteTarget(Account.class), is(true));
	}

	/**
	 * @see DATAMONGO-607
	 */
	@Test
	public void activatesAbbreviatingPropertiesCorrectly() {

		loadValidConfiguration();
		BeanDefinition definition = factory.getBeanDefinition("abbreviatingConverter.mongoMappingContext");
		Object value = definition.getPropertyValues().getPropertyValue("fieldNamingStrategy").getValue();

		assertThat(value, is(instanceOf(BeanDefinition.class)));
		BeanDefinition strategy = (BeanDefinition) value;
		assertThat(strategy.getBeanClassName(), is(CamelCaseAbbreviatingFieldNamingStrategy.class.getName()));
	}

	/**
	 * @see DATAMONGO-866
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
	 * @see DATAMONGO-892
	 */
	@Test
	public void shouldThrowBeanDefinitionParsingExceptionIfConverterDefinedAsNestedBean() {

		exception.expect(BeanDefinitionParsingException.class);
		exception.expectMessage("Mongo Converter must not be defined as nested bean.");

		loadNestedBeanConfiguration();
	}

	/**
	 * @see DATAMONGO-925, DATAMONGO-928
	 */
	@Test
	public void shouldSupportCustomFieldNamingStrategy() {
		assertStrategyReferenceSetFor("mappingConverterWithCustomFieldNamingStrategy");
	}

	/**
	 * @see DATAMONGO-925, DATAMONGO-928
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

		BeanDefinition definition = reader.getRegistry().getBeanDefinition(beanId.concat(".mongoMappingContext"));
		BeanReference value = (BeanReference) definition.getPropertyValues().getPropertyValue("fieldNamingStrategy")
				.getValue();

		assertThat(value.getBeanName(), is("customFieldNamingStrategy"));
	}

	@Component
	public static class SampleConverter implements Converter<Person, DBObject> {
		public DBObject convert(Person source) {
			return null;
		}
	}

	@Component
	public static class SampleConverterFactory implements GenericConverter {

		public Set<ConvertiblePair> getConvertibleTypes() {
			return Collections.singleton(new ConvertiblePair(Account.class, DBObject.class));
		}

		public Object convert(Object source, TypeDescriptor sourceType, TypeDescriptor targetType) {
			return null;
		}
	}
}
