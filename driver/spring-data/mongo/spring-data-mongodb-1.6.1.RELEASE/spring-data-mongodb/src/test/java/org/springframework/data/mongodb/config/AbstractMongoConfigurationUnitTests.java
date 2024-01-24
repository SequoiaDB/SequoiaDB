/*
 * Copyright 2012-2013 the original author or authors.
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

import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.ExpectedException;
import org.springframework.beans.factory.NoSuchBeanDefinitionException;
import org.springframework.context.annotation.AnnotationConfigApplicationContext;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.support.AbstractApplicationContext;
import org.springframework.data.mongodb.MongoDbFactory;
import org.springframework.data.mongodb.core.convert.MappingMongoConverter;
import org.springframework.data.mongodb.core.convert.MongoTypeMapper;
import org.springframework.data.mongodb.core.mapping.BasicMongoPersistentEntity;
import org.springframework.data.mongodb.core.mapping.Document;
import org.springframework.data.mongodb.core.mapping.MongoMappingContext;
import org.springframework.expression.spel.support.StandardEvaluationContext;
import org.springframework.test.util.ReflectionTestUtils;

import org.springframework.data.mongodb.assist.Mongo;
import org.springframework.data.mongodb.assist.MongoClient;

/**
 * Unit tests for {@link AbstractMongoConfiguration}.
 * 
 * @author Oliver Gierke
 * @author Thomas Darimont
 */
public class AbstractMongoConfigurationUnitTests {

	@Rule public ExpectedException exception = ExpectedException.none();

	/**
	 * @see DATAMONGO-496
	 */
	@Test
	public void usesConfigClassPackageAsBaseMappingPackage() throws ClassNotFoundException {

		AbstractMongoConfiguration configuration = new SampleMongoConfiguration();
		assertThat(configuration.getMappingBasePackage(), is(SampleMongoConfiguration.class.getPackage().getName()));
		assertThat(configuration.getInitialEntitySet(), hasSize(1));
		assertThat(configuration.getInitialEntitySet(), hasItem(Entity.class));
	}

	/**
	 * @see DATAMONGO-496
	 */
	@Test
	public void doesNotScanPackageIfMappingPackageIsNull() throws ClassNotFoundException {

		assertScanningDisabled(null);

	}

	/**
	 * @see DATAMONGO-496
	 */
	@Test
	public void doesNotScanPackageIfMappingPackageIsEmpty() throws ClassNotFoundException {

		assertScanningDisabled("");
		assertScanningDisabled(" ");
	}

	/**
	 * @see DATAMONGO-569
	 */
	@Test
	public void containsMongoDbFactoryButNoMongoBean() {

		AbstractApplicationContext context = new AnnotationConfigApplicationContext(SampleMongoConfiguration.class);

		assertThat(context.getBean(MongoDbFactory.class), is(notNullValue()));

		exception.expect(NoSuchBeanDefinitionException.class);
		context.getBean(Mongo.class);
		context.close();
	}

	@Test
	public void returnsUninitializedMappingContext() throws Exception {

		SampleMongoConfiguration configuration = new SampleMongoConfiguration();
		MongoMappingContext context = configuration.mongoMappingContext();

		assertThat(context.getPersistentEntities(), is(emptyIterable()));
		context.initialize();
		assertThat(context.getPersistentEntities(), is(not(emptyIterable())));
	}

	/**
	 * @see DATAMONGO-717
	 */
	@Test
	public void lifecycleCallbacksAreInvokedInAppropriateOrder() {

		AbstractApplicationContext context = new AnnotationConfigApplicationContext(SampleMongoConfiguration.class);
		MongoMappingContext mappingContext = context.getBean(MongoMappingContext.class);
		BasicMongoPersistentEntity<?> entity = mappingContext.getPersistentEntity(Entity.class);
		StandardEvaluationContext spElContext = (StandardEvaluationContext) ReflectionTestUtils.getField(entity, "context");

		assertThat(spElContext.getBeanResolver(), is(notNullValue()));
		context.close();
	}

	/**
	 * @see DATAMONGO-725
	 */
	@Test
	public void shouldBeAbleToConfigureCustomTypeMapperViaJavaConfig() {

		AbstractApplicationContext context = new AnnotationConfigApplicationContext(SampleMongoConfiguration.class);
		MongoTypeMapper typeMapper = context.getBean(CustomMongoTypeMapper.class);
		MappingMongoConverter mmc = context.getBean(MappingMongoConverter.class);

		assertThat(mmc, is(notNullValue()));
		assertThat(mmc.getTypeMapper(), is(typeMapper));
		context.close();
	}

	/**
	 * @see DATAMONGO-789
	 */
	@Test
	public void authenticationDatabaseShouldDefaultToNull() {
		assertThat(new SampleMongoConfiguration().getAuthenticationDatabaseName(), is(nullValue()));
	}

	private static void assertScanningDisabled(final String value) throws ClassNotFoundException {

		AbstractMongoConfiguration configuration = new SampleMongoConfiguration() {
			@Override
			protected String getMappingBasePackage() {
				return value;
			}
		};

		assertThat(configuration.getMappingBasePackage(), is(value));
		assertThat(configuration.getInitialEntitySet(), hasSize(0));
	}

	@Configuration
	static class SampleMongoConfiguration extends AbstractMongoConfiguration {

		@Override
		protected String getDatabaseName() {
			return "database";
		}

		@Override
		public Mongo mongo() throws Exception {
			return new MongoClient();
		}

		@Bean
		@Override
		public MappingMongoConverter mappingMongoConverter() throws Exception {
			MappingMongoConverter mmc = super.mappingMongoConverter();
			mmc.setTypeMapper(typeMapper());
			return mmc;
		}

		@Bean
		public MongoTypeMapper typeMapper() {
			return new CustomMongoTypeMapper();
		}
	}

	@Document
	static class Entity {

	}
}
