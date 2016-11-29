/*
* srlua.c
* Lua interpreter for self-running programs
* Luiz Henrique de Figueiredo <lhf@tecgraf.puc-rio.br>
* 27 Apr 2012 09:24:34
* This code is hereby placed in the public domain.
*/

#if defined(_WIN32)
  #include <windows.h>
  #define _PATH_MAX MAX_PATH
#else
  #define _PATH_MAX PATH_MAX
#endif

#if defined (__CYGWIN__)
  #include <sys/cygwin.h>
#endif

#if defined(__linux__) || defined(__sun)
  #include <unistd.h> /* readlink */
#endif

#if defined(__APPLE__)
  #include <sys/param.h>
  #include <mach-o/dyld.h>
#endif

#if defined(__FreeBSD__)
  #include <sys/types.h>
  #include <sys/sysctl.h>
#endif

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "glue.h"
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

#include "cluajitar_fmt.h"
#include "cluajitar_srlua.h"

typedef struct
{
 FILE *f;
 size_t size;
 char buff[512];
} State;

static const char *myget(lua_State *L, void *data, size_t *size)
{
 State* s=data;
 size_t n;
 (void)L;
 n=(sizeof(s->buff)<=s->size)? sizeof(s->buff) : s->size;
 n=fread(s->buff,1,n,s->f);
 s->size-=n;
 *size=n;
 return (n>0) ? s->buff : NULL;
}

#define cannot(x) luaL_error(L,"cannot %s %s: %s",x,name,strerror(errno))

static int read_cluajitar_internal(State *s, char found_name[NAMESIZE], const char *name, int (*namecmp)(const char*, const char*), lua_State *L, FILE *f)
{
 struct cluajitar_header h;
 if (fread(&h, sizeof(h), 1, f) < 1) cannot("read header");
 if (strncmp(h.magic, MAGIC_NUMBER, MAGIC_NUMBER_LEN) != 0) cannot("illegal magic number");

 long offset = ftell(f);
 uint32_t skip_size = 0;

 for (uint32_t i = 0; i < h.file_count; ++i) {
  struct cluajitar_file_header fh;
  if (fread(&fh, sizeof(fh), 1, f) < 1) cannot("read file header");

  /* found */
  if (namecmp(fh.name, name) == 0) {
   if (fseek(f, offset + (sizeof(fh) * h.file_count) + skip_size, SEEK_SET) != 0) cannot("seek");
   s->f = f; s->size = (size_t)fh.file_size;
   strcpy(found_name, fh.name);
   return 0;
  }

  skip_size += fh.file_size;
 }

 return -1;
}

static int truecmp(const char* _, const char* __) { return 0; }
static int read_first_from_cluajitar(State *s, char found_name[NAMESIZE], lua_State *L, FILE *f)
{
 return read_cluajitar_internal(s, found_name, "first", truecmp, L, f);
}

static int read_find_from_cluajitar(State *s, const char *find_name, lua_State *L, FILE *f)
{
 char dummy[NAMESIZE];
 return read_cluajitar_internal(s, dummy, find_name, strcmp, L, f);
}

static char* getprog();

int load_file_cluajitar(lua_State *L, const char *find_name)
{
 char *name = getprog();
 Glue t;
 State S;
 FILE *f=fopen(name,"rb");
 int c;
 if (f==NULL) cannot("open");
 if (fseek(f,-sizeof(t),SEEK_END)!=0) cannot("seek");
 if (fread(&t,sizeof(t),1,f)!=1) cannot("read");
 if (memcmp(t.sig,GLUESIG,GLUELEN)!=0) goto on_error;
 if (fseek(f,t.size1,SEEK_SET)!=0) cannot("seek");

 if (read_find_from_cluajitar(&S, find_name, L, f) < 0) goto on_error;
 c=getc(f);
 if (c=='#')
  while (--S.size>0 && c!='\n') c=getc(f);
 else
  ungetc(c,f);
#if LUA_VERSION_NUM <= 501
 int status = lua_load(L,myget,&S,find_name);
#else
 int status = lua_load(L,myget,&S,find_name,NULL);
#endif
 fclose(f);
 free(name);
 return (status == 0) ? 0 : -1;

on_error:
 fclose(f);
 free(name);
 return -1;
}

static void load_main(lua_State *L, const char *name)
{
 Glue t;
 State S;
 FILE *f=fopen(name,"rb");
 int c;
 if (f==NULL) cannot("open");
 if (fseek(f,-sizeof(t),SEEK_END)!=0) cannot("seek");
 if (fread(&t,sizeof(t),1,f)!=1) cannot("read");
 if (memcmp(t.sig,GLUESIG,GLUELEN)!=0) luaL_error(L,"no Lua program found in %s",name);
 if (fseek(f,t.size1,SEEK_SET)!=0) cannot("seek");

 char main_src[NAMESIZE];
 if (read_first_from_cluajitar(&S, main_src, L, f) < 0) cannot("main program"); /* find first program from cluajitar(first is main program) */
 c=getc(f);
 if (c=='#')
  while (--S.size>0 && c!='\n') c=getc(f);
 else
  ungetc(c,f);
#if LUA_VERSION_NUM <= 501
 if (lua_load(L,myget,&S,main_src)!=0) lua_error(L);
#else
 if (lua_load(L,myget,&S,main_src,NULL)!=0) lua_error(L);
#endif
 fclose(f);
}

static int pmain(lua_State *L)
{
 int argc=lua_tointeger(L,1);
 char** argv=lua_touserdata(L,2);
 int i;
 lua_gc(L,LUA_GCSTOP,0);
 luaL_openlibs(L);
 lua_gc(L,LUA_GCRESTART,0);
 load_main(L,argv[0]);
 lua_createtable(L,argc,0);
 for (i=0; i<argc; i++)
 {
  lua_pushstring(L,argv[i]);
  lua_rawseti(L,-2,i);
 }
 lua_setglobal(L,"arg");
 luaL_checkstack(L,argc-1,"too many arguments to script");
 for (i=1; i<argc; i++)
 {
  lua_pushstring(L,argv[i]);
 }
 lua_call(L,argc-1,0);
 return 0;
}

static void fatal(const char* progname, const char* message)
{
#ifdef _WIN32
 MessageBox(NULL,message,progname,MB_ICONERROR | MB_OK);
#else
 fprintf(stderr,"%s: %s\n",progname,message);
#endif
 exit(EXIT_FAILURE);
}

static char* getprog() {
  int nsize = _PATH_MAX + 1;
  char* progdir = malloc(nsize * sizeof(char));
  char *lb;
  int n = 0;
#if defined(__CYGWIN__)
  char win_buff[_PATH_MAX + 1];
  GetModuleFileNameA(NULL, win_buff, nsize);
  cygwin_conv_path(CCP_WIN_A_TO_POSIX, win_buff, progdir, nsize);
  n = strlen(progdir);
#elif defined(_WIN32)
  n = GetModuleFileNameA(NULL, progdir, nsize);
#elif defined(__linux__)
  n = readlink("/proc/self/exe", progdir, nsize);
  if (n > 0) progdir[n] = 0;
#elif defined(__sun)
  pid_t pid = getpid();
  char linkname[256];
  sprintf(linkname, "/proc/%d/path/a.out", pid);
  n = readlink(linkname, progdir, nsize);
  if (n > 0) progdir[n] = 0;
#elif defined(__FreeBSD__)
  int mib[4];
  mib[0] = CTL_KERN;
  mib[1] = KERN_PROC;
  mib[2] = KERN_PROC_PATHNAME;
  mib[3] = -1;
  size_t cb = nsize;
  sysctl(mib, 4, progdir, &cb, NULL, 0);
  n = cb;
#elif defined(__BSD__)
  n = readlink("/proc/curproc/file", progdir, nsize);
  if (n > 0) progdir[n] = 0;
#elif defined(__APPLE__)
  uint32_t nsize_apple = nsize;
  if (_NSGetExecutablePath(progdir, &nsize_apple) == 0)
    n = strlen(progdir);
#else
  // FALLBACK
  // Use 'lsof' ... should work on most UNIX systems (incl. OSX)
  // lsof will list open files, this captures the 1st file listed (usually the executable)
  int pid;
  FILE* fd;
  char cmd[80];
  pid = getpid();

  sprintf(cmd, "lsof -p %d | awk '{if ($5==\"REG\") { print $9 ; exit}}' 2> /dev/null", pid);
  fd = popen(cmd, "r");
  n = fread(progdir, 1, nsize, fd);
  pclose(fd);

  // remove newline
  if (n > 1) progdir[--n] = '\0';
#endif
  if (n == 0 || n == nsize || (lb = strrchr(progdir, (int)LUA_DIRSEP[0])) == NULL)
    return NULL;
  return (progdir);
}

int srlua_main(int argc, char *argv[])
{
 lua_State *L;
 argv[0] = getprog();

 if (argv[0]==NULL) fatal("srlua","cannot locate this executable");
 L=luaL_newstate();
 if (L==NULL) fatal(argv[0],"not enough memory for state");
 lua_pushcfunction(L,&pmain);
 lua_pushinteger(L,argc);
 lua_pushlightuserdata(L,argv);
 if (lua_pcall(L,2,0,0)!=0) fatal(argv[0],lua_tostring(L,-1));
 lua_close(L);
 return EXIT_SUCCESS;
}
