#ifndef _CLUAJITAR_FMT_H
#define _CLUAJITAR_FMT_H

#include <stdint.h>

/*ljar(LuaJIT ar)*/
#define MAGIC_NUMBER  "ljar"
#define MAGIC_NUMBER_LEN (sizeof(MAGIC_NUMBER)-1)

struct cluajitar_header
{
	char magic[MAGIC_NUMBER_LEN];
	uint32_t file_count;
};

#define NAMESIZE 100
#define NAMESIZE_LEN ((NAMESIZE)-1)

struct cluajitar_file_header
{
	char name[NAMESIZE];
	uint32_t file_size;
};

/* ---------- */

struct cluajitar_header_content
{
	struct cluajitar_header header;

	struct cluajitar_file_header *file_header_list;
	uint32_t *file_offset_list;
	/* NOTE : count = cluajitar_header.file_count */
};

#endif /*_CLUAJITAR_FMT_H*/
