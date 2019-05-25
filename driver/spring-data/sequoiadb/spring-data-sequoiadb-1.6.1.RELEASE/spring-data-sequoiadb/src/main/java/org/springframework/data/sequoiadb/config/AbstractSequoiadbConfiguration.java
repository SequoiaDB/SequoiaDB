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

import java.util.Collections;
import java.util.HashSet;
import java.util.Set;

import org.springframework.beans.factory.config.BeanDefinition;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.ClassPathScanningCandidateComponentProvider;
import org.springframework.context.annotation.Configuration;
import org.springframework.core.convert.converter.Converter;
import org.springframework.core.type.filter.AnnotationTypeFilter;
import org.springframework.data.annotation.Persistent;
import org.springframework.data.authentication.UserCredentials;
import org.springframework.data.mapping.context.MappingContextIsNewStrategyFactory;
import org.springframework.data.mapping.model.CamelCaseAbbreviatingFieldNamingStrategy;
import org.springframework.data.mapping.model.FieldNamingStrategy;
import org.springframework.data.mapping.model.PropertyNameFieldNamingStrategy;
import org.springframework.data.sequoiadb.SequoiadbFactory;
import org.springframework.data.sequoiadb.assist.Sdb;
import org.springframework.data.sequoiadb.core.SequoiadbTemplate;
import org.springframework.data.sequoiadb.core.SimpleSequoiadbFactory;
import org.springframework.data.sequoiadb.core.convert.CustomConversions;
import org.springframework.data.sequoiadb.core.convert.DbRefResolver;
import org.springframework.data.sequoiadb.core.convert.DefaultDbRefResolver;
import org.springframework.data.sequoiadb.core.convert.MappingSequoiadbConverter;
import org.springframework.data.sequoiadb.core.mapping.Document;
import org.springframework.data.sequoiadb.core.mapping.SequoiadbMappingContext;
import org.springframework.data.support.CachingIsNewStrategyFactory;
import org.springframework.data.support.IsNewStrategyFactory;
import org.springframework.util.ClassUtils;
import org.springframework.util.StringUtils;

/**
 * Base class for Spring Data SequoiaDB configuration using JavaConfig.
 *
 */
@Configuration
public abstract class AbstractSequoiadbConfiguration {

	/**
	 * Return the name of the database to connect to.
	 * 
	 * @return must not be {@literal null}.
	 */
	protected abstract String getDatabaseName();

	/**
	 * Return the name of the authentication database to use. Defaults to {@literal null} and will turn into the value
	 * returned by {@link #getDatabaseName()} later on effectively.
	 * 
	 * @return
	 */
	protected String getAuthenticationDatabaseName() {
		return null;
	}

	/**
	 * Return the {@link Sdb} instance to connect to. Annotate with {@link Bean} in case you want to expose a
	 * {@link Sdb} instance to the {@link org.springframework.context.ApplicationContext}.
	 * 
	 * @return
	 * @throws Exception
	 */
	public abstract Sdb sdb() throws Exception;

	/**
	 * Creates a {@link SequoiadbTemplate}.
	 * 
	 * @return
	 * @throws Exception
	 */
	@Bean
	public SequoiadbTemplate sequoiadbTemplate() throws Exception {
		return new SequoiadbTemplate(sequoiadbFactory(), mappingSequoiadbConverter());
	}

	/**
	 * Creates a {@link SimpleSequoiadbFactory} to be used by the {@link SequoiadbTemplate}. Will use the {@link Sdb} instance
	 * configured in {@link #sdb()}.
	 * 
	 * @see #sdb()
	 * @see #sequoiadbTemplate()
	 * @return
	 * @throws Exception
	 */
	@Bean
	public SequoiadbFactory sequoiadbFactory() throws Exception {
		return new SimpleSequoiadbFactory(sdb(), getDatabaseName(), getUserCredentials(), getAuthenticationDatabaseName());
	}

	/**
	 * Return the base package to scan for mapped {@link Document}s. Will return the package name of the configuration
	 * class' (the concrete class, not this one here) by default. So if you have a {@code com.acme.AppConfig} extending
	 * {@link AbstractSequoiadbConfiguration} the base package will be considered {@code com.acme} unless the method is
	 * overriden to implement alternate behaviour.
	 * 
	 * @return the base package to scan for mapped {@link Document} classes or {@literal null} to not enable scanning for
	 *         entities.
	 */
	protected String getMappingBasePackage() {

		Package mappingBasePackage = getClass().getPackage();
		return mappingBasePackage == null ? null : mappingBasePackage.getName();
	}

	/**
	 * Return {@link UserCredentials} to be used when connecting to the SequoiaDB instance or {@literal null} if none shall
	 * be used.
	 * 
	 * @return
	 */
	protected UserCredentials getUserCredentials() {
		return null;
	}

	/**
	 * Creates a {@link SequoiadbMappingContext} equipped with entity classes scanned from the mapping base package.
	 * 
	 * @see #getMappingBasePackage()
	 * @return
	 * @throws ClassNotFoundException
	 */
	@Bean
	public SequoiadbMappingContext sequoiadbMappingContext() throws ClassNotFoundException {

		SequoiadbMappingContext mappingContext = new SequoiadbMappingContext();
		mappingContext.setInitialEntitySet(getInitialEntitySet());
		mappingContext.setSimpleTypeHolder(customConversions().getSimpleTypeHolder());
		mappingContext.setFieldNamingStrategy(fieldNamingStrategy());

		return mappingContext;
	}

	/**
	 * Returns a {@link MappingContextIsNewStrategyFactory} wrapped into a {@link CachingIsNewStrategyFactory}.
	 * 
	 * @return
	 * @throws ClassNotFoundException
	 */
	@Bean
	public IsNewStrategyFactory isNewStrategyFactory() throws ClassNotFoundException {
		return new CachingIsNewStrategyFactory(new MappingContextIsNewStrategyFactory(sequoiadbMappingContext()));
	}

	/**
	 * Register custom {@link Converter}s in a {@link CustomConversions} object if required. These
	 * {@link CustomConversions} will be registered with the {@link #mappingSequoiadbConverter()} and
	 * {@link #sequoiadbMappingContext()}. Returns an empty {@link CustomConversions} instance by default.
	 * 
	 * @return must not be {@literal null}.
	 */
	@Bean
	public CustomConversions customConversions() {
		return new CustomConversions(Collections.emptyList());
	}

	/**
	 * Creates a {@link MappingSequoiadbConverter} using the configured {@link #sequoiadbFactory()} and
	 * {@link #sequoiadbMappingContext()}. Will get {@link #customConversions()} applied.
	 * 
	 * @see #customConversions()
	 * @see #sequoiadbMappingContext()
	 * @see #sequoiadbFactory()
	 * @return
	 * @throws Exception
	 */
	@Bean
	public MappingSequoiadbConverter mappingSequoiadbConverter() throws Exception {

		DbRefResolver dbRefResolver = new DefaultDbRefResolver(sequoiadbFactory());
		MappingSequoiadbConverter converter = new MappingSequoiadbConverter(dbRefResolver, sequoiadbMappingContext());
		converter.setCustomConversions(customConversions());

		return converter;
	}

	/**
	 * Scans the mapping base package for classes annotated with {@link Document}.
	 * 
	 * @see #getMappingBasePackage()
	 * @return
	 * @throws ClassNotFoundException
	 */
	protected Set<Class<?>> getInitialEntitySet() throws ClassNotFoundException {

		String basePackage = getMappingBasePackage();
		Set<Class<?>> initialEntitySet = new HashSet<Class<?>>();

		if (StringUtils.hasText(basePackage)) {
			ClassPathScanningCandidateComponentProvider componentProvider = new ClassPathScanningCandidateComponentProvider(
					false);
			componentProvider.addIncludeFilter(new AnnotationTypeFilter(Document.class));
			componentProvider.addIncludeFilter(new AnnotationTypeFilter(Persistent.class));

			for (BeanDefinition candidate : componentProvider.findCandidateComponents(basePackage)) {
				initialEntitySet.add(ClassUtils.forName(candidate.getBeanClassName(),
						AbstractSequoiadbConfiguration.class.getClassLoader()));
			}
		}

		return initialEntitySet;
	}

	/**
	 * Configures whether to abbreviate field names for domain objects by configuring a
	 * {@link CamelCaseAbbreviatingFieldNamingStrategy} on the {@link SequoiadbMappingContext} instance created. For advanced
	 * customization needs, consider overriding {@link #mappingSequoiadbConverter()}.
	 * 
	 * @return
	 */
	protected boolean abbreviateFieldNames() {
		return false;
	}

	/**
	 * Configures a {@link FieldNamingStrategy} on the {@link SequoiadbMappingContext} instance created.
	 * 
	 * @return
	 * @since 1.5
	 */
	protected FieldNamingStrategy fieldNamingStrategy() {
		return abbreviateFieldNames() ? new CamelCaseAbbreviatingFieldNamingStrategy()
				: PropertyNameFieldNamingStrategy.INSTANCE;
	}
}
