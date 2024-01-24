package org.springframework.data.sequoiadb.core.mapping;

import static org.hamcrest.CoreMatchers.*;
import static org.junit.Assert.*;

import java.util.Arrays;
import java.util.Collections;
import java.util.List;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.runners.MockitoJUnitRunner;
import org.springframework.data.sequoiadb.core.mapping.BasicSequoiadbPersistentEntity.SequoiadbPersistentPropertyComparator;

import static org.mockito.Mockito.*;

/**
 * Unit tests for {@link SequoiadbPersistentPropertyComparator}.
 * 

 */
@RunWith(MockitoJUnitRunner.class)
public class SequoiadbPersistentPropertyComparatorUnitTests {

	@Mock
    SequoiadbPersistentProperty firstName;

	@Mock
    SequoiadbPersistentProperty lastName;

	@Mock
    SequoiadbPersistentProperty ssn;

	@Test
	public void ordersPropertiesCorrectly() {

		when(ssn.getFieldOrder()).thenReturn(10);
		when(firstName.getFieldOrder()).thenReturn(20);
		when(lastName.getFieldOrder()).thenReturn(Integer.MAX_VALUE);

		List<SequoiadbPersistentProperty> properties = Arrays.asList(firstName, lastName, ssn);
		Collections.sort(properties, SequoiadbPersistentPropertyComparator.INSTANCE);

		assertThat(properties.get(0), is(ssn));
		assertThat(properties.get(1), is(firstName));
		assertThat(properties.get(2), is(lastName));
	}
}
