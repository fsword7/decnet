dnl AC_SYS_SOCKADDR_SA_LEN
AC_DEFUN(AC_SYS_SOCKADDR_SA_LEN,
[AC_MSG_CHECKING([for sa_len in struct sockaddr])
AC_CACHE_VAL(ac_cv_sys_sockaddr_sa_len,
[AC_TRY_COMPILE([
#include <sys/types.h>
#include <sys/socket.h>
], [
int length;
struct sockaddr sock;
length = sock.sa_len;
], ac_cv_sys_sockaddr_sa_len=yes, ac_cv_sys_sockaddr_sa_len=no)])dnl
if test $ac_cv_sys_sockaddr_sa_len = yes; then
  AC_MSG_RESULT(yes)
  AC_DEFINE_UNQUOTED(HAVE_SOCKADDR_SA_LEN)
else
  AC_MSG_RESULT(no)
fi
])dnl
