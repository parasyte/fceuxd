#define VERSION_NUMERIC 97
#define VERSION_STRING "0.97.5"
#define FCEUXD_VERSION_STRING "0.03C PRIVATE BUILD - DO NOT DISTRIBUTE"

#if PSS_STYLE==2

#define PSS "\\"
#define PS '\\'

#elif PSS_STYLE==1

#define PSS "/"
#define PS '/'

#elif PSS_STYLE==3

#define PSS "\\"
#define PS '\\'

#elif PSS_STYLE==4

#define PSS ":"
#define PS ':'

#endif

#ifdef NOSTDOUT
#define puts(x) 
#define printf(x,...)
#endif
