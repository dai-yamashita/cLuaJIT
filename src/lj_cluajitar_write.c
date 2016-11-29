#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <errno.h>

#include "cluajitar_fmt.h"
#include "cluajitar_write.h"

static
void close_files(FILE **fps, int count)
{
	if (fps) for (int i = 0; i < count; ++i) if (fps[i]) fclose(fps[i]);
	free(fps);
}

long cluajitar_write(const char **src_files, size_t count, FILE *out_fp)
{
	char *buf = NULL;
	long *sizes = NULL;

	long start_offset = ftell(out_fp);

	FILE **fps = (FILE **)calloc(count, sizeof(FILE)); /* NOTE : calloc zero fill, for fclose success only */
	if (!fps) goto on_error;

	/* file open */
	for (size_t i = 0; i < count; ++i) {
		FILE *fp = fopen(src_files[i], "rb");
		if (!fp) goto on_error;
		fps[i] = fp;
	}

	/* input phase */

	/* get size */
	sizes = (long *)malloc(sizeof(long) * count);
	if (!sizes) goto on_error;

	long max_size = 0;
	for (size_t i = 0; i < count; ++i) {
		FILE *fp = fps[i];

		long size;
		if (fseek(fp, 0, SEEK_END) != 0) goto on_error;
		size = ftell(fp);
		if (fseek(fp, 0, SEEK_SET) != 0) goto on_error;

		if (max_size < size) max_size = size;
		sizes[i] = size;
	}

	/* output phase */

	/* header */
	struct cluajitar_header header = {
		.magic = MAGIC_NUMBER,
		.file_count = count,
	};
	if (fwrite(&header, sizeof(header), 1, out_fp) < 1) goto on_error;

	/* file_header */
	for (size_t i = 0; i < count; ++i) {
		struct cluajitar_file_header file_header = {
			.file_size = sizes[i],
		};
		size_t len = strlen(src_files[i]);
		if (len > NAMESIZE_LEN) {
			fprintf(stderr, "src file length(%d) is over %d", len, NAMESIZE_LEN);
			goto on_error;
		}
		strcpy(file_header.name, src_files[i]);

		if (fwrite(&file_header, sizeof(file_header), 1, out_fp) < 1) goto on_error;
	}

	/* from file */
	buf = malloc(sizeof(char) * max_size);
	if (!buf) goto on_error;

	for (size_t i = 0; i < count; ++i) {
		FILE *fp = fps[i];
		long size = sizes[i];

		if (fread(&buf[0], size, 1, fp) < 1) if (ferror(fp)) goto on_error;
		if (fwrite(&buf[0], size, 1, out_fp) < 1) goto on_error;
	}

	/* done */
	free(buf);
	free(sizes);
	close_files(fps, count);

	return ftell(out_fp) - start_offset;

on_error:
	if (errno > 0) perror(NULL);

	free(buf);
	free(sizes);
	close_files(fps, count);
	return -1;
}
