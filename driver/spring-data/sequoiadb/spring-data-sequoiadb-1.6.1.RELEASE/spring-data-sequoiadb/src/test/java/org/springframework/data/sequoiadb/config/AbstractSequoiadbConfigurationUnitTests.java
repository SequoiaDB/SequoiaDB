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
package org.springframework.data.sequoiadb.config;

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
import org.springframework.data.sequoiadb.SequoiadbFactory;
import org.springframework.data.sequoiadb.assist.Sdb;
import org.springframework.data.sequoiadb.assist.SdbClient;
import org.springframework.data.sequoiadb.core.convert.MappingSequoiadbConverter;
import org.springframework.data.sequoiadb.core.convert.SequoiadbTypeMapper;
import org.springframework.data.sequoiadb.core.mapping.BasicSequoiadbPersistentEntity;
import org.springframework.data.sequoiadb.core.mapping.Document;
import org.springframework.data.sequoiadb.core.mapping.SequoiadbMappingContext;
import org.springframework.expression.spel.support.StandardEvaluationContext;
import org.springframework.test.util.ReflectionTestUtils;

/**
 * Unit tests for {@link AbstractSequoiadbConfiguration}.
 * 


 */
public class AbstractSequoiadbConfigurationUnitTests {

	@Rule public ExpectedException exception = ExpectedException.none();

	/**
	 * @see DATA_JIRA-496
	 */
	@Test
	public void usesConfigClassPackageAsBaseMappingPackage() throws ClassNotFoundException {

		AbstractSequoiadbConfiguration configuration = new SampleSequoiadbConfiguration();
		assertThat(configuration.getMappingBasePackage(), is(SampleSequoiadbConfiguration.class.getPackage().getName()));
		assertThat(configuration.getInitialEntitySet(), hasSize(1));
		assertThat(configuration.getInitialEntitySet(), hasItem(Entity.class));
	}

	/**
	 * @see DATA_JIRA-496
	 */
	@Test
	public void doesNotScanPackageIfMappingPackageIsNull() throws ClassNotFoundException {

		assertScanningDisabled(null);

	}

	/**
	 * @see DATA_JIRA-496
	 */
	@Test
	public void doesNotScanPackageIfMappingPackageIsEmpty() throws ClassNotFoundException {

		assertScanningDisabled("");
		assertScanningDisabled(" ");
	}

	/**
	 * @see DATA_JIRA-569
	 */
	@Test
	public void containsSequoiadbFactoryButNoSequoiadbBean() {

		AbstractApplicationContext context = new AnnotationConfigApplicationContext(SampleSequoiadbConfiguration.class);

		assertThat(context.getBean(SequoiadbFactory.class), is(notNullValue()));

		exception.expect(NoSuchBeanDefinitionException.class);
		context.getBean(Sdb.class);
		context.close();
	}

	@Test
	public void returnsUninitializedMappingContext() throws Exception {

		SampleSequoiadbConfiguration configuration = new SampleSequoiadbConfiguration();
		SequoiadbMappingContext context = configuration.sequoiadbMappingContext();

		assertThat(context.getPersistentEntities(), is(emptyIterable()));
		context.initialize();
		assertThat(context.getPersistentEntities(), is(not(emptyIterable())));
	}

	/**
	 * @see DATA_JIRA-717
	 */
	@Test
	public void lifecycleCallbacksAreInvokedInAppropriateOrder() {

		AbstractApplicationContext context = new AnnotationConfigApplicationContext(SampleSequoiadbConfiguration.class);
		SequoiadbMappingContext mappingContext = context.getBean(SequoiadbMappingContext.class);
		BasicSequoiadbPersistentEntity<?> entity = mappingContext.getPersistentEntity(Entity.class);
		StandardEvaluationContext spElContext = (StandardEvaluationContext) ReflectionTestUtils.getField(entity, "context");

		assertThat(spElContext.getBeanResolver(), is(notNullValue()));
		context.close();
	}

	/**
	 * @see DATA_JIRA-725
	 */
	@Test
	public void shouldBeAbleToConfigureCustomTypeMapperViaJavaConfig() {

		AbstractApplicationContext context = new AnnotationConfigApplicationContext(SampleSequoiadbConfiguration.class);
		SequoiadbTypeMapper typeMapper = context.getBean(CustomSequoiadbTypeMapper.class);
		MappingSequoiadbConverter mmc = context.getBean(MappingSequoiadbConverter.class);

		assertThat(mmc, is(notNullValue()));
		assertThat(mmc.getTypeMapper(), is(typeMapper));
		context.close();
	}

	/**
	 * @see DATA_JIRA-789
	 */
	@Test
	public void authenticationDatabaseShouldDefaultToNull() {
		assertThat(new SampleSequoiadbConfiguration().getAuthenticationDatabaseName(), is(nullValue()));
	}

	private static void assertScanningDisabled(final String value) throws ClassNotFoundException {

		AbstractSequoiadbConfiguration configuration = new SampleSequoiadbConfiguration() {
			@Override
			protected String getMappingBasePackage() {
				return value;
			}
		};

		assertThat(configuration.getMappingBasePackage(), is(value));
		assertThat(configuration.getInitialEntitySet(), hasSize(0));
	}

	@Configuration
	static class SampleSequoiadbConfiguration extends AbstractSequoiadbConfiguration {

		@Override
		protected String getDatabaseName() {
			return "database";
		}

		@Override
		public Sdb sdb() throws Exception {
			return new SdbClient();
		}

		@Bean
		@Override
		public MappingSequoiadbConverter mappingSequoiadbConverter() throws Exception {
			MappingSequoiadbConverter mmc = super.mappingSequoiadbConverter();
			mmc.setTypeMapper(typeMapper());
			return mmc;
		}

		@Bean
		public SequoiadbTypeMapper typeMapper() {
			return new CustomSequoiadbTypeMapper();
		}
	}

	@Document
	static class Entity {

	}
}
