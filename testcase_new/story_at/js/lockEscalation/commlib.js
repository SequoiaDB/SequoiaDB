import( "../lib/basic_operation/commlib.js" )
import( "../lib/main.js" )
import( "../lib/func.js" )

function checkLockEscalated(db, expected)
{
    var actual = false
    var gotSnapshot = false
    var result = db.snapshot( SDB_SNAP_TRANSACTIONS_CURRENT )
    while ( result.next() )
    {
        var record = result.current().toObj()
        gotSnapshot = true
        actual = record["IsLockEscalated"]
        break
    }
    assert.equal(gotSnapshot, true, "got snapshot failed")
    assert.equal(actual, expected, "check lock escalated failed")
}