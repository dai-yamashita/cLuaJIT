/*
* glue.c
* glue exe and script
* Luiz Henrique de Figueiredo <lhf@tecgraf.puc-rio.br>
* 19 Feb 2005 09:14:06
* This code is hereby placed in the public domain.
*/

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include "glue.h"
#include "cluajitar.h"

static void cannot(const char* what, const char* name)
{
 fprintf(stderr,"cannot %s %s: %s\n",what,name,strerror(errno));
 exit(EXIT_FAILURE);
}

static long copy(const char* name, FILE* out, const char* outname)
{

 FILE* f;
 long size;
 f=fopen(name,"rb");
 if (f==NULL) cannot("open",name);
 if (fseek(f,0,SEEK_END)!=0) cannot("seek",name);
 size=ftell(f);
 if (fseek(f,0,SEEK_SET)!=0) cannot("seek",name);
 for (;;)
 {
  char b[BUFSIZ];
  int n=fread(&b,1,sizeof(b),f);
  if (n==0) { if (ferror(f)) cannot("read",name); else break; }
  if (fwrite(&b,n,1,out)!=1) cannot("write",outname);
 }
 fclose(f);
 return size;
}

static long archive_and_copy(int argc, char *argv[], FILE* out, const char* outname)
{
 /* pick lua code */
 for (int i = 0; i < argc - 1; ++i) {
  argv[i] = argv[i + 1];
 }
 argv[argc - 2][0] = '\0';
 argv[argc - 2] = NULL;
 argc -= 2;
 
 /* archive and copy */
 long size = cluajitar_write((const char **)argv, argc, out);
 if (size < 0) cannot("archive", outname);
 
 return size;
}

int glue_main(int argc, char* argv[])
{
 char name[MAX_PATH];
 Glue t= { GLUESIG, 0, 0 };
 FILE* f;
 if (argc<3)
 {
  fprintf(stderr,"usage: lua.exe  prog.lua [prog2.lua prog3.lua ...] prog.exe -link\n");
  return 1;
 }
 GetModuleFileName(NULL, name, sizeof(name));
 for (int i = 2; i < argc - 1; ++i) {
  if (strcmp(argv[1], argv[i]) == 0) {
   cannot("write",argv[1]);
  }
 }

 int lst_argi = argc - 1;
 const char *out_file = argv[lst_argi];

 f = fopen(out_file, "wb");
 if (f==NULL) cannot("open",out_file);
 t.size1 = copy(name, f, out_file);
 t.size2 = archive_and_copy(argc, argv, f, out_file); /* NOTE : argv is inout */
 t.sig[GLUETYP]= 'L';
 if (fwrite(&t,sizeof(t),1,f)!=1) cannot("write",out_file);
 if (fclose(f)!=0) cannot("close",out_file);
 return 0;
}
