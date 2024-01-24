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

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
import org.springframework.core.convert.converter.Converter;
import org.springframework.data.convert.ReadingConverter;
import org.springframework.data.convert.WritingConverter;
import org.springframework.data.geo.Box;
import org.springframework.data.geo.Circle;
import org.springframework.data.geo.Distance;
import org.springframework.data.geo.Metrics;
import org.springframework.data.geo.Point;
import org.springframework.data.geo.Polygon;
import org.springframework.data.geo.Shape;
import org.springframework.data.sequoiadb.core.geo.Sphere;
import org.springframework.data.sequoiadb.core.query.GeoCommand;
import org.springframework.util.Assert;





/**
 * Wrapper class to contain useful geo structure converters for the usage with Sdb.
 * 


 * @since 1.5
 */
abstract class GeoConverters {

	/**
	 * Private constructor to prevent instantiation.
	 */
	private GeoConverters() {}

	/**
	 * Returns the geo converters to be registered.
	 * 
	 * @return
	 */
	public static Collection<? extends Object> getConvertersToRegister() {
		return Arrays.asList( //
				BoxToDbObjectConverter.INSTANCE //
				, PolygonToDbObjectConverter.INSTANCE //
				, CircleToDbObjectConverter.INSTANCE //
				, SphereToDbObjectConverter.INSTANCE //
				, DbObjectToBoxConverter.INSTANCE //
				, DbObjectToPolygonConverter.INSTANCE //
				, DbObjectToCircleConverter.INSTANCE //
				, DbObjectToSphereConverter.INSTANCE //
				, DbObjectToPointConverter.INSTANCE //
				, PointToDbObjectConverter.INSTANCE //
				, GeoCommandToDbObjectConverter.INSTANCE);
	}

	/**
	 * Converts a {@link List} of {@link Double}s into a {@link Point}.
	 * 

	 * @since 1.5
	 */
	@ReadingConverter
	public static enum DbObjectToPointConverter implements Converter<BSONObject, Point> {

		INSTANCE;

		/*
		 * (non-Javadoc)
		 * @see org.springframework.core.convert.converter.Converter#convert(java.lang.Object)
		 */
		@Override
		public Point convert(BSONObject source) {

			Assert.isTrue(source.keySet().size() == 2, "Source must contain 2 elements");

			return source == null ? null : new Point((Double) source.get("x"), (Double) source.get("y"));
		}
	}

	/**
	 * Converts a {@link Point} into a {@link List} of {@link Double}s.
	 * 

	 * @since 1.5
	 */
	public static enum PointToDbObjectConverter implements Converter<Point, BSONObject> {

		INSTANCE;

		/*
		 * (non-Javadoc)
		 * @see org.springframework.core.convert.converter.Converter#convert(java.lang.Object)
		 */
		@Override
		public BSONObject convert(Point source) {
			return null;
		}
	}

	/**
	 * Converts a {@link Box} into a {@link BasicBSONList}.
	 * 

	 * @since 1.5
	 */
	@WritingConverter
	public static enum BoxToDbObjectConverter implements Converter<Box, BSONObject> {

		INSTANCE;

		/*
		 * (non-Javadoc)
		 * @see org.springframework.core.convert.converter.Converter#convert(java.lang.Object)
		 */
		@Override
		public BSONObject convert(Box source) {

			if (source == null) {
				return null;
			}

			BasicBSONObject result = new BasicBSONObject();
			result.put("first", PointToDbObjectConverter.INSTANCE.convert(source.getFirst()));
			result.put("second", PointToDbObjectConverter.INSTANCE.convert(source.getSecond()));
			return result;
		}
	}

	/**
	 * Converts a {@link BasicBSONList} into a {@link Box}.
	 * 

	 * @since 1.5
	 */
	@ReadingConverter
	public static enum DbObjectToBoxConverter implements Converter<BSONObject, Box> {

		INSTANCE;

		/*
		 * (non-Javadoc)
		 * @see org.springframework.core.convert.converter.Converter#convert(java.lang.Object)
		 */
		@Override
		public Box convert(BSONObject source) {

			if (source == null) {
				return null;
			}

			Point first = DbObjectToPointConverter.INSTANCE.convert((BSONObject) source.get("first"));
			Point second = DbObjectToPointConverter.INSTANCE.convert((BSONObject) source.get("second"));

			return new Box(first, second);
		}
	}

	/**
	 * Converts a {@link Circle} into a {@link BasicBSONList}.
	 * 

	 * @since 1.5
	 */
	public static enum CircleToDbObjectConverter implements Converter<Circle, BSONObject> {

		INSTANCE;

		/*
		 * (non-Javadoc)
		 * @see org.springframework.core.convert.converter.Converter#convert(java.lang.Object)
		 */
		@Override
		public BSONObject convert(Circle source) {

			if (source == null) {
				return null;
			}

			BSONObject result = new BasicBSONObject();
			result.put("center", PointToDbObjectConverter.INSTANCE.convert(source.getCenter()));
			result.put("radius", source.getRadius().getNormalizedValue());
			result.put("metric", source.getRadius().getMetric().toString());
			return result;
		}
	}

	/**
	 * Converts a {@link BSONObject} into a {@link Circle}.
	 * 

	 * @since 1.5
	 */
	@ReadingConverter
	public static enum DbObjectToCircleConverter implements Converter<BSONObject, Circle> {

		INSTANCE;

		/*
		 * (non-Javadoc)
		 * @see org.springframework.core.convert.converter.Converter#convert(java.lang.Object)
		 */
		@Override
		public Circle convert(BSONObject source) {

			if (source == null) {
				return null;
			}

			BSONObject center = (BSONObject) source.get("center");
			Double radius = (Double) source.get("radius");

			Distance distance = new Distance(radius);

			if (source.containsField("metric")) {

				String metricString = (String) source.get("metric");
				Assert.notNull(metricString, "Metric must not be null!");

				distance = distance.in(Metrics.valueOf(metricString));
			}

			Assert.notNull(center, "Center must not be null!");
			Assert.notNull(radius, "Radius must not be null!");

			return new Circle(DbObjectToPointConverter.INSTANCE.convert(center), distance);
		}
	}

	/**
	 * Converts a {@link Sphere} into a {@link BasicBSONList}.
	 * 

	 * @since 1.5
	 */
	public static enum SphereToDbObjectConverter implements Converter<Sphere, BSONObject> {

		INSTANCE;

		/*
		 * (non-Javadoc)
		 * @see org.springframework.core.convert.converter.Converter#convert(java.lang.Object)
		 */
		@Override
		public BSONObject convert(Sphere source) {

			if (source == null) {
				return null;
			}

			BSONObject result = new BasicBSONObject();
			result.put("center", PointToDbObjectConverter.INSTANCE.convert(source.getCenter()));
			result.put("radius", source.getRadius().getNormalizedValue());
			result.put("metric", source.getRadius().getMetric().toString());
			return result;
		}
	}

	/**
	 * Converts a {@link BasicBSONList} into a {@link Sphere}.
	 * 

	 * @since 1.5
	 */
	@ReadingConverter
	public static enum DbObjectToSphereConverter implements Converter<BSONObject, Sphere> {

		INSTANCE;

		/*
		 * (non-Javadoc)
		 * @see org.springframework.core.convert.converter.Converter#convert(java.lang.Object)
		 */
		@Override
		public Sphere convert(BSONObject source) {

			if (source == null) {
				return null;
			}

			BSONObject center = (BSONObject) source.get("center");
			Double radius = (Double) source.get("radius");

			Distance distance = new Distance(radius);

			if (source.containsField("metric")) {

				String metricString = (String) source.get("metric");
				Assert.notNull(metricString, "Metric must not be null!");

				distance = distance.in(Metrics.valueOf(metricString));
			}

			Assert.notNull(center, "Center must not be null!");
			Assert.notNull(radius, "Radius must not be null!");

			return new Sphere(DbObjectToPointConverter.INSTANCE.convert(center), distance);
		}
	}

	/**
	 * Converts a {@link Polygon} into a {@link BasicBSONList}.
	 * 

	 * @since 1.5
	 */
	public static enum PolygonToDbObjectConverter implements Converter<Polygon, BSONObject> {

		INSTANCE;

		/*
		 * (non-Javadoc)
		 * @see org.springframework.core.convert.converter.Converter#convert(java.lang.Object)
		 */
		@Override
		public BSONObject convert(Polygon source) {

			if (source == null) {
				return null;
			}

			List<Point> points = source.getPoints();
			List<BSONObject> pointTuples = new ArrayList<BSONObject>(points.size());

			for (Point point : points) {
				pointTuples.add(PointToDbObjectConverter.INSTANCE.convert(point));
			}

			BSONObject result = new BasicBSONObject();
			result.put("points", pointTuples);
			return result;
		}
	}

	/**
	 * Converts a {@link BasicBSONList} into a {@link Polygon}.
	 * 

	 * @since 1.5
	 */
	@ReadingConverter
	public static enum DbObjectToPolygonConverter implements Converter<BSONObject, Polygon> {

		INSTANCE;

		/*
		 * (non-Javadoc)
		 * @see org.springframework.core.convert.converter.Converter#convert(java.lang.Object)
		 */
		@Override
		@SuppressWarnings({ "unchecked" })
		public Polygon convert(BSONObject source) {

			if (source == null) {
				return null;
			}

			List<BSONObject> points = (List<BSONObject>) source.get("points");
			List<Point> newPoints = new ArrayList<Point>(points.size());

			for (BSONObject element : points) {

				Assert.notNull(element, "Point elements of polygon must not be null!");
				newPoints.add(DbObjectToPointConverter.INSTANCE.convert(element));
			}

			return new Polygon(newPoints);
		}
	}

	/**
	 * Converts a {@link Sphere} into a {@link BasicBSONList}.
	 * 

	 * @since 1.5
	 */
	public static enum GeoCommandToDbObjectConverter implements Converter<GeoCommand, BSONObject> {

		INSTANCE;

		/*
		 * (non-Javadoc)
		 * @see org.springframework.core.convert.converter.Converter#convert(java.lang.Object)
		 */
		@Override
		public BSONObject convert(GeoCommand source) {

			if (source == null) {
				return null;
			}

			BasicBSONList argument = new BasicBSONList();

			Shape shape = source.getShape();

			if (shape instanceof Box) {

				argument.add(toList(((Box) shape).getFirst()));
				argument.add(toList(((Box) shape).getSecond()));

			} else if (shape instanceof Circle) {

				argument.add(toList(((Circle) shape).getCenter()));
				argument.add(((Circle) shape).getRadius().getNormalizedValue());

			} else if (shape instanceof Circle) {

				argument.add(toList(((Circle) shape).getCenter()));
				argument.add(((Circle) shape).getRadius());

			} else if (shape instanceof Polygon) {

				for (Point point : ((Polygon) shape).getPoints()) {
					argument.add(toList(point));
				}

			} else if (shape instanceof Sphere) {

				argument.add(toList(((Sphere) shape).getCenter()));
				argument.add(((Sphere) shape).getRadius().getNormalizedValue());
			}

			return new BasicBSONObject(source.getCommand(), argument);
		}
	}

	static List<Double> toList(Point point) {
		return Arrays.asList(point.getX(), point.getY());
	}
}
