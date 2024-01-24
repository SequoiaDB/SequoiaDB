/*
 * Copyright 2022 SequoiaDB Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except
 * in compliance with the License. You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed under the License
 * is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express
 * or implied. See the License for the specific language governing permissions and limitations under
 * the License.
 */

package org.bson.types;

import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;

import java.time.Instant;
import java.time.LocalDate;
import java.time.LocalDateTime;
import java.time.ZoneId;
import java.util.Date;

/**
 * The class BSONDate represents a specific instant in time, with millisecond precision. This type
 * corresponds to the Date type of SequoiaDB database. When accessing SequoiaDB database, you need
 * to convert the date type in your own business code to BSONDate.
 *
 * </p>
 * Write scene example:
 * <pre>
 * BSONObject obj = new BasicBSONObject();
 * LocalDate localDate = LocalDate.of( 2022, 1, 1 );
 * obj.put( &quot;key&quot;, BSONDate.valueOf( localDate ) );
 * // insert this obj into SequoiaDB database
 * </pre>
 *
 * </p>
 * Read scene example:
 * <pre>
 * BSONDate date = (BSONDate) obj.get( &quot;key&quot; );
 * LocalDate localDate = date.toLocalDate();
 * // Handling this localDate in own business code
 * </pre>
 */
public class BSONDate extends Date {

    // BSONDate is a bridge between SequoiaDB date type and java date type,
    // so do not provide a construct with no arguments.

    /**
     * Construct a BSONDate.
     * @param value The milliseconds since January 1, 1970, 00:00:00 GMT.
     */
    public BSONDate( long value ) {
        super( value );
    }

    /**
     * Obtains an instance of BSONDate from an Instant object. Parts of the Instant object below
     * milliseconds are ignored.
     * @param instant The instant to convert
     * @return A BSONDate representing the same point on the time-line as the provided instant
     */
    public static BSONDate from( Instant instant ) {
        if ( instant == null ) {
            throw new BaseException( SDBError.SDB_INVALIDARG, "The instant can not be null" );
        }

        try {
            return new BSONDate( instant.toEpochMilli() );
        } catch ( ArithmeticException e ) {
            throw new BaseException( SDBError.SDB_INVALIDARG, "The instant " + instant.toString() +
                    " is too large to represent as a BSONDate", e );
        } catch ( Exception e ) {
            throw new BaseException( SDBError.SDB_INVALIDARG, "Invalid instant " + instant.toString(), e );
        }
    }

    /**
     * Obtains an instance of BSONDate from a LocalDate object.
     * @param localDate The localDate to convert
     * @return A BSONDate object
     */
    public static BSONDate valueOf( LocalDate localDate ) {
        if ( localDate == null ) {
            throw new BaseException( SDBError.SDB_INVALIDARG, "The localDate can not be null" );
        }
        Instant instant = localDate.atStartOfDay( ZoneId.systemDefault() ).toInstant();
        return BSONDate.from( instant );
    }

    /**
     * Obtains an instance of BSONDate from a LocalDateTime object. Parts of the LocalDateTime object
     * below milliseconds are ignored.
     * @param localDateTime The localDateTime to convert
     * @return A BSONDate object
     */
    public static BSONDate valueOf( LocalDateTime localDateTime ) {
        if ( localDateTime == null ) {
            throw new BaseException( SDBError.SDB_INVALIDARG, "The localDateTime can not be null" );
        }
        Instant instant = localDateTime.atZone( ZoneId.systemDefault() ).toInstant();
        return BSONDate.from( instant );
    }

    /**
     * Converts this BSONDate object to an Instant Object.
     * @return An instant representing the same point on the time-line as this BSONDate object
     */
    @Override
    public Instant toInstant() {
        return Instant.ofEpochMilli(getTime());
    }

    /**
     * Converts this BSONDate object to a LocalDate object.
     * @return A LocalDate object
     */
    public LocalDate toLocalDate() {
        return toLocalDateTime().toLocalDate();
    }

    /**
     * Converts this BSONDate object to a LocalDateTime object.
     * @return A LocalDateTime object
     */
    public LocalDateTime toLocalDateTime() {
        Instant instant = toInstant();
        return LocalDateTime.ofInstant( instant, ZoneId.systemDefault() );
    }

    @Override
    public String toString() {
        return toLocalDateTime().toString();
    }
}
