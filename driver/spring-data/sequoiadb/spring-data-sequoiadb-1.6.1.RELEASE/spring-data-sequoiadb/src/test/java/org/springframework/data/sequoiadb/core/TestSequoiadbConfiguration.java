package org.springframework.data.sequoiadb.core;

import java.util.ArrayList;
import java.util.List;

import org.springframework.context.annotation.Bean;
import org.springframework.core.convert.converter.Converter;
import org.springframework.data.sequoiadb.assist.Sdb;
import org.springframework.data.sequoiadb.assist.SdbClient;
import org.springframework.data.sequoiadb.config.AbstractSequoiadbConfiguration;
import org.springframework.data.sequoiadb.core.convert.CustomConversions;
import org.springframework.data.sequoiadb.core.mapping.SequoiadbMappingContext;

public class TestSequoiadbConfiguration extends AbstractSequoiadbConfiguration {

	@Override
	public String getDatabaseName() {
		return "database";
	}

	@Override
	@Bean
	public Sdb sdb() throws Exception {
		return new SdbClient("127.0.0.1", 11810);
	}

	@Override
	public String getMappingBasePackage() {
		return SequoiadbMappingContext.class.getPackage().getName();
	}

	@Override
	public CustomConversions customConversions() {

		List<Converter<?, ?>> converters = new ArrayList<Converter<?, ?>>();
		converters.add(new org.springframework.data.sequoiadb.core.PersonReadConverter());
		converters.add(new org.springframework.data.sequoiadb.core.PersonWriteConverter());
		return new CustomConversions(converters);
	}
}
