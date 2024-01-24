/* Here are a few defines that you might have to comment out */
#define HAS_LONG_LONG_TYPE 1
#define HAS_LONG_DOUBLE_TYPE 1
#define HAS_ENUM_BITFIELDS 1

#include <limits.h>
#include <float.h>
#include <stddef.h>
#include <stdio.h>

typedef struct {
  char c;
  short x;
} align_short;

typedef struct {
  char c;
  int x;
} align_int;

typedef struct {
  char c;
  long x;
} align_long;

#ifdef HAS_LONG_LONG_TYPE
typedef struct {
  char c;
  long long x;
} align_long_long;
#endif

typedef struct {
  char c;
  float x;
} align_float;

typedef struct {
  char c;
  double x;
} align_double;

#ifdef HAS_LONG_DOUBLE_TYPE
typedef struct {
  char c;
  long double x;
} align_long_double;
#endif

typedef struct {
  char c;
  char *x;
} align_pointer;

typedef struct {
  char c;
  size_t x;
} align_size_t;

typedef struct {
  char c;
  wchar_t x;
} align_wchar_t;

enum E { NEG = -1, POS = 1 };
struct S { 
  int member : 8; 
#ifdef HAS_ENUM_BITFIELDS
  enum E enum_member : 8;
#endif
} var;

int 
main(void)
{
  int i;

  /* Bits in a char */
  printf("set target_char_bit  %d\n\n", (int)CHAR_BIT);

  /* Endianess */
  i = 1;
  printf("set target_little_endian %d\n\n", (*(char *)&i) == 1);

  /* Print the size of the various builtin data types. */
  printf("set target_sizeof_short        %d\n", (int)sizeof(short));
  printf("set target_sizeof_int          %d\n", (int)sizeof(int));
  printf("set target_sizeof_long         %d\n", (int)sizeof(long));
#ifdef HAS_LONG_LONG_TYPE
  printf("set target_sizeof_long_long    %d\n", (int)sizeof(long long));
#endif
  printf("set target_sizeof_float        %d\n", (int)sizeof(float));
  printf("set target_sizeof_double       %d\n", (int)sizeof(double));
#ifdef HAS_LONG_DOUBLE_TYPE
  printf("set target_sizeof_long_double  %d\n", (int)sizeof(long double));
#endif
  printf("set target_sizeof_pointer      %d\n", (int)sizeof(char *));
  printf("set target_sizeof_size_t       %d\n", (int)sizeof(size_t));
  printf("set target_sizeof_wchar_t      %d\n", (int)sizeof(wchar_t));
  printf("\n");

  /* Alignment requirements */
  printf("set target_alignof_short       %d\n", (int)offsetof(align_short, x));
  printf("set target_alignof_int         %d\n", (int)offsetof(align_int, x));
  printf("set target_alignof_long        %d\n", (int)offsetof(align_long, x));
#ifdef HAS_LONG_LONG_TYPE
  printf("set target_alignof_long_long   %d\n", (int)offsetof(align_long_long, x));
#endif
  printf("set target_alignof_float       %d\n", (int)offsetof(align_float, x));
  printf("set target_alignof_double      %d\n", (int)offsetof(align_double, x));
#ifdef HAS_LONG_DOUBLE_TYPE
  printf("set target_alignof_long_double %d\n", (int)offsetof(align_long_double, x));
#endif
  printf("set target_alignof_pointer     %d\n", (int)offsetof(align_pointer, x));
  printf("set target_alignof_size_t      %d\n", (int)offsetof(align_size_t, x));
  printf("set target_alignof_wchar_t     %d\n", (int)offsetof(align_wchar_t, x));
  printf("\n");

  /* Signedness */
  printf("set target_plain_char_is_unsigned %d\n", (char)-1 > 0);
  printf("set target_wchar_t_is_unsigned %d\n",    (wchar_t)-1 > 0);
  var.member = -1;
  printf("set target_plain_iint_bit_field_is_unsigned %d\n", var.member > 0);
#ifdef HAS_ENUM_BITFIELDS
  var.enum_member = NEG;
  printf("set target_enum_bit_fields_are_always_unsigned %d\n",
	 ((signed int)var.enum_member) > 0);
#endif
  printf("\n");

  /* Floating point things */
  printf("set target_flt_min_exp  %d\n", FLT_MIN_EXP);
  printf("set target_flt_max_exp  %d\n", FLT_MAX_EXP);
  printf("set target_dbl_min_exp  %d\n", DBL_MIN_EXP);
  printf("set target_dbl_max_exp  %d\n", DBL_MAX_EXP);
  printf("set target_ldbl_min_exp %d\n", LDBL_MIN_EXP);
  printf("set target_ldbl_max_exp %d\n", LDBL_MAX_EXP);
  printf("\n");

  /* Miscellaneous type stuff */
  if (sizeof(size_t) == sizeof(short)) {
    printf("set target_size_t_int_kind   {unsigned short}\n");
  } else if (sizeof(size_t) == sizeof(int)) {
    printf("set target_size_t_int_kind   {unsigned int}\n");
  } else if (sizeof(size_t) == sizeof(long)) {
    printf("set target_size_t_int_kind   {unsigned long}\n");
#ifdef HAS_LONG_LONG_TYPE
  } else if (sizeof(size_t) == sizeof(long long)) {
    printf("set target_size_t_int_kind   {unsigned long}\n");
#endif
  } else {
    fprintf(stderr, "OOPS: size_t does not match any built-in type\n");
  }
  if (sizeof(wchar_t) == sizeof(short)) {
    printf("set target_wchar_t_int_kind   {unsigned short}\n");
  } else if (sizeof(wchar_t) == sizeof(int)) {
    printf("set target_wchar_t_int_kind   {unsigned int}\n");
  } else if (sizeof(wchar_t) == sizeof(long)) {
    printf("set target_wchar_t_int_kind   {unsigned long}\n");
#ifdef HAS_LONG_LONG_TYPE
  } else if (sizeof(wchar_t) == sizeof(long long)) {
    printf("set target_wchar_t_int_kind   {unsigned long}\n");
#endif
  } else {
    fprintf(stderr, "OOPS: wchar_t does not match any built-in type\n");
  }

  printf("set target_enum_types_can_be_smaller_than_int  0;   # FIXME \n");
  
  printf("\n");
  return 0;
}
