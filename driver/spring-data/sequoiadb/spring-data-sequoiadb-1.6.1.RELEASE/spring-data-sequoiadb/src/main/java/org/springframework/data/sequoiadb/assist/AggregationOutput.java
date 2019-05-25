package org.springframework.data.sequoiadb.assist;

import org.bson.BSONObject;

@SuppressWarnings("deprecation")
public class AggregationOutput {

    /**
     * returns an iterator to the results of the aggregation
     * @return the results of the aggregation
     */
    public Iterable<BSONObject> results() {
        return _resultSet;
    }

    /**
     * returns the command result of the aggregation
     * @return the command result
     *
     * @deprecated there is no replacement for this method
     */
    @Deprecated
    public CommandResult getCommandResult(){
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * returns the original aggregation command
     * @return the command
     *
     * @deprecated there is no replacement for this method
     */
    @Deprecated
    public BSONObject getCommand() {
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * returns the address of the server used to execute the aggregation
     * @return the server which executed the aggregation
     *
     * @deprecated there is no replacement for this method
     */
    @Deprecated
    public ServerAddress getServerUsed() {
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * Constructs a new instance
     *
     * @param command the aggregation command
     * @param commandResult the aggregation command result
     *
     * @deprecated there is no replacement for this constructor
     */
    @SuppressWarnings("unchecked")
    @Deprecated
    public AggregationOutput(BSONObject command, CommandResult commandResult) {
        throw new UnsupportedOperationException("not supported!");
    }

    AggregationOutput(Iterable<BSONObject> result) {
        _resultSet = result;
        _commandResult = null;
        _cmd = null;
    }

    /**
     * @deprecated Please use {@link #getCommandResult()} instead.
     */
    @Deprecated
    protected final CommandResult _commandResult;

    /**
     * @deprecated Please use {@link #getCommand()} instead.
     */
    @Deprecated
    protected final BSONObject _cmd;

    /**
     * @deprecated Please use {@link #results()} instead.
     */
    @Deprecated
    protected final Iterable<BSONObject> _resultSet;
}
