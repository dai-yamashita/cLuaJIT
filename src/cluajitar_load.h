#ifndef _CLUAJITAR_LOAD_H
#define _CLUAJITAR_LOAD_H

struct cluajitar_header_content *cluajitar_load(FILE *fp);
void cluajitar_free(struct cluajitar_header_content *content);

char *cluajitar_load_file(const char *name, FILE *fp, const struct cluajitar_header_content *content);
void cluajitar_free_file(char *buf);

#endif /*_CLUAJITAR_LOAD_H*/
