/*
 * Copyright 2014 the original author or authors.
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

import static org.hamcrest.CoreMatchers.*;
import static org.junit.Assert.*;

import java.util.Arrays;

import org.bson.BSONObject;
import org.junit.Test;
import org.springframework.data.geo.Box;
import org.springframework.data.geo.Circle;
import org.springframework.data.geo.Distance;
import org.springframework.data.geo.Metrics;
import org.springframework.data.geo.Point;
import org.springframework.data.geo.Polygon;

import org.springframework.data.sequoiadb.core.convert.GeoConverters.BoxToDbObjectConverter;
import org.springframework.data.sequoiadb.core.convert.GeoConverters.CircleToDbObjectConverter;
import org.springframework.data.sequoiadb.core.convert.GeoConverters.DbObjectToBoxConverter;
import org.springframework.data.sequoiadb.core.convert.GeoConverters.DbObjectToCircleConverter;
import org.springframework.data.sequoiadb.core.convert.GeoConverters.DbObjectToPointConverter;
import org.springframework.data.sequoiadb.core.convert.GeoConverters.DbObjectToPolygonConverter;
import org.springframework.data.sequoiadb.core.convert.GeoConverters.DbObjectToSphereConverter;
import org.springframework.data.sequoiadb.core.convert.GeoConverters.GeoCommandToDbObjectConverter;
import org.springframework.data.sequoiadb.core.convert.GeoConverters.PointToDbObjectConverter;
import org.springframework.data.sequoiadb.core.convert.GeoConverters.PolygonToDbObjectConverter;
import org.springframework.data.sequoiadb.core.convert.GeoConverters.SphereToDbObjectConverter;
import org.springframework.data.sequoiadb.core.geo.Sphere;
import org.springframework.data.sequoiadb.core.query.GeoCommand;

/**
 * Unit tests for {@link GeoConverters}.
 * 


 * @since 1.5
 */
public class GeoConvertersUnitTests {

	/**
	 * @see DATA_JIRA-858
	 */
	@Test
	public void convertsBoxToDbObjectAndBackCorrectly() {

		Box box = new Box(new Point(1, 2), new Point(3, 4));

		BSONObject dbo = BoxToDbObjectConverter.INSTANCE.convert(box);
		Box result = DbObjectToBoxConverter.INSTANCE.convert(dbo);

		assertThat(result, is(box));
		assertThat(result.getClass().equals(Box.class), is(true));
	}

	/**
	 * @see DATA_JIRA-858
	 */
	@Test
	public void convertsCircleToDbObjectAndBackCorrectlyNeutralDistance() {

		Circle circle = new Circle(new Point(1, 2), 3);

		BSONObject dbo = CircleToDbObjectConverter.INSTANCE.convert(circle);
		Circle result = DbObjectToCircleConverter.INSTANCE.convert(dbo);

		assertThat(result, is(circle));
	}

	/**
	 * @see DATA_JIRA-858
	 */
	@Test
	public void convertsCircleToDbObjectAndBackCorrectlyMilesDistance() {

		Distance radius = new Distance(3, Metrics.MILES);
		Circle circle = new Circle(new Point(1, 2), radius);

		BSONObject dbo = CircleToDbObjectConverter.INSTANCE.convert(circle);
		Circle result = DbObjectToCircleConverter.INSTANCE.convert(dbo);

		assertThat(result, is(circle));
		assertThat(result.getRadius(), is(radius));
	}

	/**
	 * @see DATA_JIRA-858
	 */
	@Test
	public void convertsPolygonToDbObjectAndBackCorrectly() {

		Polygon polygon = new Polygon(new Point(1, 2), new Point(2, 3), new Point(3, 4), new Point(5, 6));

		BSONObject dbo = PolygonToDbObjectConverter.INSTANCE.convert(polygon);
		Polygon result = DbObjectToPolygonConverter.INSTANCE.convert(dbo);

		assertThat(result, is(polygon));
		assertThat(result.getClass().equals(Polygon.class), is(true));
	}

	/**
	 * @see DATA_JIRA-858
	 */
	@Test
	public void convertsSphereToDbObjectAndBackCorrectlyWithNeutralDistance() {

		Sphere sphere = new Sphere(new Point(1, 2), 3);

		BSONObject dbo = SphereToDbObjectConverter.INSTANCE.convert(sphere);
		Sphere result = DbObjectToSphereConverter.INSTANCE.convert(dbo);

		assertThat(result, is(sphere));
		assertThat(result.getClass().equals(Sphere.class), is(true));
	}

	/**
	 * @see DATA_JIRA-858
	 */
	@Test
	public void convertsSphereToDbObjectAndBackCorrectlyWithKilometerDistance() {

		Distance radius = new Distance(3, Metrics.KILOMETERS);
		Sphere sphere = new Sphere(new Point(1, 2), radius);

		BSONObject dbo = SphereToDbObjectConverter.INSTANCE.convert(sphere);
		Sphere result = DbObjectToSphereConverter.INSTANCE.convert(dbo);

		assertThat(result, is(sphere));
		assertThat(result.getRadius(), is(radius));
		assertThat(result.getClass().equals(org.springframework.data.sequoiadb.core.geo.Sphere.class), is(true));
	}

	/**
	 * @see DATA_JIRA-858
	 */
	@Test
	public void convertsPointToListAndBackCorrectly() {

		Point point = new Point(1, 2);

		BSONObject dbo = PointToDbObjectConverter.INSTANCE.convert(point);
		Point result = DbObjectToPointConverter.INSTANCE.convert(dbo);

		assertThat(result, is(point));
		assertThat(result.getClass().equals(Point.class), is(true));
	}

	/**
	 * @see DATA_JIRA-858
	 */
	@Test
	public void convertsGeoCommandToDbObjectCorrectly() {

		Box box = new Box(new double[] { 1, 2 }, new double[] { 3, 4 });
		GeoCommand cmd = new GeoCommand(box);

		BSONObject dbo = GeoCommandToDbObjectConverter.INSTANCE.convert(cmd);

		assertThat(dbo, is(notNullValue()));

		BSONObject boxObject = (BSONObject) dbo.get("$box");

		assertThat(boxObject,
				is((Object) Arrays.asList(GeoConverters.toList(box.getFirst()), GeoConverters.toList(box.getSecond()))));
	}
}
