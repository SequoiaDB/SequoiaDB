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
package org.springframework.data.sequoiadb.core.convert;

import static org.hamcrest.Matchers.*;
import static org.junit.Assert.*;

import java.net.URL;
import java.text.DateFormat;
import java.text.Format;
import java.util.Arrays;
import java.util.Date;
import java.util.Locale;
import java.util.UUID;

import org.bson.types.Binary;
import org.bson.types.ObjectId;
import org.joda.time.DateTime;
import org.junit.Test;
import org.springframework.aop.framework.ProxyFactory;
import org.springframework.core.convert.converter.Converter;
import org.springframework.core.convert.support.DefaultConversionService;
import org.springframework.core.convert.support.GenericConversionService;
import org.springframework.data.sequoiadb.core.convert.SequoiadbConverters.StringToBigIntegerConverter;

import org.springframework.data.sequoiadb.assist.DBRef;

/**
 * Unit tests for {@link CustomConversions}.
 * 

 * @auhtor Christoph Strobl
 */
public class CustomConversionsUnitTests {

	@Test
	@SuppressWarnings("unchecked")
	public void findsBasicReadAndWriteConversions() {

		CustomConversions conversions = new CustomConversions(Arrays.asList(FormatToStringConverter.INSTANCE,
				StringToFormatConverter.INSTANCE));

		assertThat(conversions.getCustomWriteTarget(Format.class, null), is(typeCompatibleWith(String.class)));
		assertThat(conversions.getCustomWriteTarget(String.class, null), is(nullValue()));

		assertThat(conversions.hasCustomReadTarget(String.class, Format.class), is(true));
		assertThat(conversions.hasCustomReadTarget(String.class, Locale.class), is(false));
	}

	@Test
	@SuppressWarnings("unchecked")
	public void considersSubtypesCorrectly() {

		CustomConversions conversions = new CustomConversions(Arrays.asList(NumberToStringConverter.INSTANCE,
				StringToNumberConverter.INSTANCE));

		assertThat(conversions.getCustomWriteTarget(Long.class, null), is(typeCompatibleWith(String.class)));
		assertThat(conversions.hasCustomReadTarget(String.class, Long.class), is(true));
	}

	@Test
	public void considersTypesWeRegisteredConvertersForAsSimple() {

		CustomConversions conversions = new CustomConversions(Arrays.asList(FormatToStringConverter.INSTANCE));
		assertThat(conversions.isSimpleType(UUID.class), is(true));
	}

	/**
	 * @see DATA_JIRA-240
	 */
	@Test
	public void considersObjectIdToBeSimpleType() {

		CustomConversions conversions = new CustomConversions();
		assertThat(conversions.isSimpleType(ObjectId.class), is(true));
		assertThat(conversions.hasCustomWriteTarget(ObjectId.class), is(false));

	}

	/**
	 * @see DATA_JIRA-240
	 */
	@Test
	public void considersCustomConverterForSimpleType() {

		CustomConversions conversions = new CustomConversions(Arrays.asList(new Converter<ObjectId, String>() {
			public String convert(ObjectId source) {
				return source == null ? null : source.toString();
			}
		}));

		assertThat(conversions.isSimpleType(ObjectId.class), is(true));
		assertThat(conversions.hasCustomWriteTarget(ObjectId.class), is(true));
		assertThat(conversions.hasCustomReadTarget(ObjectId.class, String.class), is(true));
		assertThat(conversions.hasCustomReadTarget(ObjectId.class, Object.class), is(false));
	}

	@Test
	public void considersDBRefsToBeSimpleTypes() {

		CustomConversions conversions = new CustomConversions();
		assertThat(conversions.isSimpleType(DBRef.class), is(true));
	}

	@Test
	public void populatesConversionServiceCorrectly() {

		GenericConversionService conversionService = new DefaultConversionService();

		CustomConversions conversions = new CustomConversions(Arrays.asList(StringToFormatConverter.INSTANCE));
		conversions.registerConvertersIn(conversionService);

		assertThat(conversionService.canConvert(String.class, Format.class), is(true));
	}

	/**
	 * @see DATA_JIRA-259
	 */
	@Test
	public void doesNotConsiderTypeSimpleIfOnlyReadConverterIsRegistered() {
		CustomConversions conversions = new CustomConversions(Arrays.asList(StringToFormatConverter.INSTANCE));
		assertThat(conversions.isSimpleType(Format.class), is(false));
	}

	/**
	 * @see DATA_JIRA-298
	 */
	@Test
	public void discoversConvertersForSubtypesOfSequoiadbTypes() {

		CustomConversions conversions = new CustomConversions(Arrays.asList(StringToIntegerConverter.INSTANCE));
		assertThat(conversions.hasCustomReadTarget(String.class, Integer.class), is(true));
		assertThat(conversions.hasCustomWriteTarget(String.class, Integer.class), is(true));
	}

	/**
	 * @see DATA_JIRA-342
	 */
	@Test
	public void doesNotHaveConverterForStringToBigIntegerByDefault() {

		CustomConversions conversions = new CustomConversions();
		assertThat(conversions.hasCustomWriteTarget(String.class), is(false));
		assertThat(conversions.getCustomWriteTarget(String.class), is(nullValue()));

		conversions = new CustomConversions(Arrays.asList(StringToBigIntegerConverter.INSTANCE));
		assertThat(conversions.hasCustomWriteTarget(String.class), is(false));
		assertThat(conversions.getCustomWriteTarget(String.class), is(nullValue()));
	}

	/**
	 * @see DATA_JIRA-390
	 */
	@Test
	public void considersBinaryASimpleType() {

		CustomConversions conversions = new CustomConversions();
		assertThat(conversions.isSimpleType(Binary.class), is(true));
	}

	/**
	 * @see DATA_JIRA-462
	 */
	@Test
	public void hasWriteConverterForURL() {

		CustomConversions conversions = new CustomConversions();
		assertThat(conversions.hasCustomWriteTarget(URL.class), is(true));
	}

	/**
	 * @see DATA_JIRA-462
	 */
	@Test
	public void readTargetForURL() {
		CustomConversions conversions = new CustomConversions();
		assertThat(conversions.hasCustomReadTarget(String.class, URL.class), is(true));
	}

	/**
	 * @see DATA_JIRA-795
	 */
	@Test
	@SuppressWarnings("rawtypes")
	public void favorsCustomConverterForIndeterminedTargetType() {

		CustomConversions conversions = new CustomConversions(Arrays.asList(DateTimeToStringConverter.INSTANCE));
		assertThat(conversions.getCustomWriteTarget(DateTime.class, null), is(equalTo((Class) String.class)));
	}

	/**
	 * @see DATA_JIRA-881
	 */
	@Test
	public void customConverterOverridesDefault() {

		CustomConversions conversions = new CustomConversions(Arrays.asList(CustomDateTimeConverter.INSTANCE));
		GenericConversionService conversionService = new DefaultConversionService();
		conversions.registerConvertersIn(conversionService);

		assertThat(conversionService.convert(new DateTime(), Date.class), is(new Date(0)));
	}

	/**
	 * @see DATA_JIRA-1001
	 */
	@Test
	public void shouldSelectPropertCustomWriteTargetForCglibProxiedType() {

		CustomConversions conversions = new CustomConversions(Arrays.asList(FormatToStringConverter.INSTANCE));
		assertThat(conversions.getCustomWriteTarget(createProxyTypeFor(Format.class)), is(typeCompatibleWith(String.class)));
	}

	/**
	 * @see DATA_JIRA-1001
	 */
	@Test
	public void shouldSelectPropertCustomReadTargetForCglibProxiedType() {

		CustomConversions conversions = new CustomConversions(Arrays.asList(CustomObjectToStringConverter.INSTANCE));
		assertThat(conversions.hasCustomReadTarget(createProxyTypeFor(Object.class), String.class), is(true));
	}

	private static Class<?> createProxyTypeFor(Class<?> type) {

		ProxyFactory factory = new ProxyFactory();
		factory.setProxyTargetClass(true);
		factory.setTargetClass(type);

		return factory.getProxy().getClass();
	}

	enum FormatToStringConverter implements Converter<Format, String> {
		INSTANCE;

		public String convert(Format source) {
			return source.toString();
		}
	}

	enum StringToFormatConverter implements Converter<String, Format> {
		INSTANCE;
		public Format convert(String source) {
			return DateFormat.getInstance();
		}
	}

	enum NumberToStringConverter implements Converter<Number, String> {
		INSTANCE;
		public String convert(Number source) {
			return source.toString();
		}
	}

	enum StringToNumberConverter implements Converter<String, Number> {
		INSTANCE;
		public Number convert(String source) {
			return 0L;
		}
	}

	enum StringToIntegerConverter implements Converter<String, Integer> {
		INSTANCE;
		public Integer convert(String source) {
			return 0;
		}
	}

	enum DateTimeToStringConverter implements Converter<DateTime, String> {
		INSTANCE;

		@Override
		public String convert(DateTime source) {
			return "";
		}
	}

	enum CustomDateTimeConverter implements Converter<DateTime, Date> {

		INSTANCE;

		@Override
		public Date convert(DateTime source) {
			return new Date(0);
		}
	}

	enum CustomObjectToStringConverter implements Converter<Object, String> {

		INSTANCE;

		@Override
		public String convert(Object source) {
			return source != null ? source.toString() : null;
		}

	}
}
