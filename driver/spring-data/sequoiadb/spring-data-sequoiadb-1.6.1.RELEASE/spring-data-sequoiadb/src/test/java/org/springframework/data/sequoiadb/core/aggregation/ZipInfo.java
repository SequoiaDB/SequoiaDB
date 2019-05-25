package org.springframework.data.sequoiadb.core.aggregation;

import java.util.Arrays;

import org.springframework.data.sequoiadb.core.mapping.Field;

/**
 * Data model from sequoiadb reference data set
 * 
 * @see http://docs.sequoiadb.org/manual/tutorial/aggregation-examples/
 * @see http://media.sequoiadb.org/zips.json
 */
class ZipInfo {

	String id;
	String city;
	String state;
	@Field("pop") int population;
	@Field("loc") double[] location;

	public String toString() {
		return "ZipInfo [id=" + id + ", city=" + city + ", state=" + state + ", population=" + population + ", location="
				+ Arrays.toString(location) + "]";
	}
}
