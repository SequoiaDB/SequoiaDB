function _jsonEqual(a,b) {
  function __jsonEqual(a,b) {
    for(var key in a) {
      if(a.hasOwnProperty(key) && b.hasOwnProperty(key)) {
        if(a[key] !== b[key]) {
          switch(a[key].constructor) {
            case Object:
              return _jsonEqual(a[key],b[key])
            case Function:
            case Array:
              if(a[key].toString() !== b[key].toString()) {
                return false
              }
              break
            default:
              return false
          }
        }
      } else {
        return false
      }
    }
    return true
  }
  return __jsonEqual(a,b) && __jsonEqual(b,a)
}