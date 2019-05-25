/*
 * Copyright 2013 the original author or authors.
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
package org.springframework.data.sequoiadb.core.aggregation;

import static org.hamcrest.CoreMatchers.*;
import static org.junit.Assert.*;

import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.ExpectedException;
import org.junit.runner.RunWith;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.data.mapping.model.MappingException;
import org.springframework.data.sequoiadb.SequoiadbFactory;
import org.springframework.data.sequoiadb.core.convert.DbRefResolver;
import org.springframework.data.sequoiadb.core.convert.DefaultDbRefResolver;
import org.springframework.data.sequoiadb.core.convert.MappingSequoiadbConverter;
import org.springframework.data.sequoiadb.core.convert.QueryMapper;
import org.springframework.data.sequoiadb.core.mapping.SequoiadbMappingContext;
import org.springframework.test.context.ContextConfiguration;
import org.springframework.test.context.junit4.SpringJUnit4ClassRunner;

/**
 * Integration tests for {@link SpelExpressionTransformer}.
 * 
 * @see DATA_JIRA-774

 */
@RunWith(SpringJUnit4ClassRunner.class)
@ContextConfiguration("classpath:infrastructure.xml")
public class SpelExpressionTransformerIntegrationTests {

	@Autowired
	SequoiadbFactory sequoiadbFactory;

	@Rule public ExpectedException exception = ExpectedException.none();

	SpelExpressionTransformer transformer;
	DbRefResolver dbRefResolver;

	@Before
	public void setUp() {
		this.transformer = new SpelExpressionTransformer();
		this.dbRefResolver = new DefaultDbRefResolver(sequoiadbFactory);
	}

	@Test
	public void shouldConvertCompoundExpressionToPropertyPath() {

		MappingSequoiadbConverter converter = new MappingSequoiadbConverter(dbRefResolver, new SequoiadbMappingContext());
		TypeBasedAggregationOperationContext ctxt = new TypeBasedAggregationOperationContext(Data.class,
				new SequoiadbMappingContext(), new QueryMapper(converter));
		assertThat(transformer.transform("item.primitiveIntValue", ctxt, new Object[0]).toString(),
				is("$item.primitiveIntValue"));
	}

	@Test
	public void shouldThrowExceptionIfNestedPropertyCannotBeFound() {

		exception.expect(MappingException.class);
		exception.expectMessage("value2");

		MappingSequoiadbConverter converter = new MappingSequoiadbConverter(dbRefResolver, new SequoiadbMappingContext());
		TypeBasedAggregationOperationContext ctxt = new TypeBasedAggregationOperationContext(Data.class,
				new SequoiadbMappingContext(), new QueryMapper(converter));
		assertThat(transformer.transform("item.value2", ctxt, new Object[0]).toString(), is("$item.value2"));
	}
}
