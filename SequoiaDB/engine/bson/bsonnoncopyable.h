#ifndef BSON_NONCOPYABLE__
#define BSON_NONCOPYABLE__
namespace bson
{
class bsonnoncopyable
  {
   protected:
      bsonnoncopyable() {}
      ~bsonnoncopyable() {}
   private:  // emphasize the following members are private
      bsonnoncopyable( const bsonnoncopyable& );
      const bsonnoncopyable& operator=( const bsonnoncopyable& );
  };
}
#endif
