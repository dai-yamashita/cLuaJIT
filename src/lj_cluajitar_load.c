#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <errno.h>

#include "cluajitar_fmt.h"
#include "cluajitar_load.h"

static
void calculate_file_offset_list(uint32_t *file_offset_list, uint32_t file_start_offset, const struct cluajitar_file_header *file_header_list, uint32_t file_count)
{
	uint32_t offset = file_start_offset;
	for (uint32_t i = 0; i < file_count; ++i) {
		file_offset_list[i] = offset;
		offset += file_header_list[i].file_size;
	}
}

struct cluajitar_header_content *cluajitar_load(FILE *fp)
{
	struct cluajitar_header_content *content = (struct cluajitar_header_content *)malloc(sizeof(struct cluajitar_header_content));
	if (!content) goto on_error;

	/* read header */
	if (fread(&content->header, sizeof(content->header), 1, fp) < 1) goto on_error;

	if (strncmp(content->header.magic, MAGIC_NUMBER, MAGIC_NUMBER_LEN) != 0) {
		fprintf(stderr, "error: illegal cluajitar_load magic number");
		goto on_error;
	}

	/* read file_header */
	content->file_header_list = (struct cluajitar_file_header *)malloc(sizeof(struct cluajitar_file_header) * content->header.file_count);
	if (!content->file_header_list) goto on_error;
	if (fread(&content->file_header_list[0], sizeof(struct cluajitar_file_header), content->header.file_count, fp) < content->header.file_count) goto on_error;

	uint32_t file_start_offset = ftell(fp);

	/* calculate offset */
	content->file_offset_list = (uint32_t *)malloc(sizeof(uint32_t) * content->header.file_count);
	if (!content->file_offset_list) goto on_error;

	calculate_file_offset_list(content->file_offset_list, file_start_offset, content->file_header_list, content->header.file_count);

	return content;

on_error:
	if (errno > 0) perror(NULL);

	if (content) {
		free(content->file_header_list);
		free(content->file_offset_list);
	}
	free(content);
	return NULL;
}


void cluajitar_free(struct cluajitar_header_content *content)
{
	free(content->file_header_list);
	free(content->file_offset_list);
	free(content);
}


static
char *load_file_internal(FILE *fp, uint32_t size, uint32_t offset)
{
	char *buf = (char *)malloc(size);
	if (!buf) goto on_error;

	if (fseek(fp, offset, SEEK_SET) != 0) goto on_error;
	if (fread(buf, 1, size, fp) < size) goto on_error;

	return buf;

on_error:
	if (errno > 0) perror(NULL);
	free(buf);

	return NULL;
}

char *cluajitar_load_file(const char *name, FILE *fp, const struct cluajitar_header_content *content)
{
	/* search file */
	for (uint32_t i = 0; i < content->header.file_count; ++i) {
		const struct cluajitar_file_header *cur_file_header = &content->file_header_list[i];
		if (strcmp(name, cur_file_header->name) == 0) {
			return load_file_internal(fp, cur_file_header->file_size, content->file_offset_list[i]);
		}
	}

	fprintf(stderr, "error: cluajitar_load_file not found name:%s", name);
	return NULL; /* load failed */
}

void cluajitar_free_file(char *buf)
{
	free(buf);
}

