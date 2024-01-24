/*
 * Copyright 2011-2013 the original author or authors.
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
package org.springframework.data.mongodb.core.convert;

import java.math.BigInteger;

import org.bson.types.ObjectId;
import org.springframework.beans.factory.InitializingBean;
import org.springframework.core.convert.ConversionService;
import org.springframework.core.convert.support.ConversionServiceFactory;
import org.springframework.core.convert.support.GenericConversionService;
import org.springframework.data.convert.EntityInstantiators;
import org.springframework.data.mongodb.core.convert.MongoConverters.BigIntegerToObjectIdConverter;
import org.springframework.data.mongodb.core.convert.MongoConverters.ObjectIdToBigIntegerConverter;
import org.springframework.data.mongodb.core.convert.MongoConverters.ObjectIdToStringConverter;
import org.springframework.data.mongodb.core.convert.MongoConverters.StringToObjectIdConverter;

/**
 * Base class for {@link MongoConverter} implementations. Sets up a {@link GenericConversionService} and populates basic
 * converters. Allows registering {@link CustomConversions}.
 * 
 * @author Jon Brisbin
 * @author Oliver Gierke
 */
public abstract class AbstractMongoConverter implements MongoConverter, InitializingBean {

	protected final GenericConversionService conversionService;
	protected CustomConversions conversions = new CustomConversions();
	protected EntityInstantiators instantiators = new EntityInstantiators();

	/**
	 * Creates a new {@link AbstractMongoConverter} using the given {@link GenericConversionService}.
	 * 
	 * @param conversionService
	 */
	@SuppressWarnings("deprecation")
	public AbstractMongoConverter(GenericConversionService conversionService) {
		this.conversionService = conversionService == null ? ConversionServiceFactory.createDefaultConversionService()
				: conversionService;
	}

	/**
	 * Registers the given custom conversions with the converter.
	 * 
	 * @param conversions
	 */
	public void setCustomConversions(CustomConversions conversions) {
		this.conversions = conversions;
	}

	/**
	 * Registers {@link EntityInstantiators} to customize entity instantiation.
	 * 
	 * @param instantiators
	 */
	public void setInstantiators(EntityInstantiators instantiators) {
		this.instantiators = instantiators == null ? new EntityInstantiators() : instantiators;
	}

	/**
	 * Registers additional converters that will be available when using the {@link ConversionService} directly (e.g. for
	 * id conversion). These converters are not custom conversions as they'd introduce unwanted conversions (e.g.
	 * ObjectId-to-String).
	 */
	private void initializeConverters() {

		if (!conversionService.canConvert(ObjectId.class, String.class)) {
			conversionService.addConverter(ObjectIdToStringConverter.INSTANCE);
		}
		if (!conversionService.canConvert(String.class, ObjectId.class)) {
			conversionService.addConverter(StringToObjectIdConverter.INSTANCE);
		}
		if (!conversionService.canConvert(ObjectId.class, BigInteger.class)) {
			conversionService.addConverter(ObjectIdToBigIntegerConverter.INSTANCE);
		}
		if (!conversionService.canConvert(BigInteger.class, ObjectId.class)) {
			conversionService.addConverter(BigIntegerToObjectIdConverter.INSTANCE);
		}

		conversions.registerConvertersIn(conversionService);
	}

	/* 
	 * (non-Javadoc)
	 * @see org.springframework.data.mongodb.core.convert.MongoWriter#convertToMongoType(java.lang.Object)
	 */
	public Object convertToMongoType(Object obj) {
		return convertToMongoType(obj, null);
	}

	/*
	 * (non-Javadoc)
	 * @see org.springframework.data.mongodb.core.core.convert.MongoConverter#getConversionService()
	 */
	public ConversionService getConversionService() {
		return conversionService;
	}

	/* (non-Javadoc)
	 * @see org.springframework.beans.factory.InitializingBean#afterPropertiesSet()
	 */
	public void afterPropertiesSet() {
		initializeConverters();
	}
}
