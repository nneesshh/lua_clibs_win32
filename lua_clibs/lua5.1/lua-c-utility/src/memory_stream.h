#ifndef __MEMORY_STREAM_H__
#define __MEMORY_STREAM_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct memory_stream_s
{
	char* buffer;
	char* cursor;
	int size;
} memory_stream_t;

memory_stream_t * create_memory_stream(char* buffer, int size);
void              destroy_memory_stream(memory_stream_t *stream);
void              reset_memory_stream(memory_stream_t *stream);

void              memory_stream_seek(memory_stream_t *stream, int pos);
void              memory_stream_skip(memory_stream_t *stream, int len);
int               memory_stream_get_used_size(memory_stream_t *stream);
int               memory_stream_get_free_size(memory_stream_t *stream);

int8_t            memory_stream_read_byte(memory_stream_t *stream);
int16_t           memory_stream_read_int16(memory_stream_t *stream);
int32_t           memory_stream_read_int32(memory_stream_t *stream);
int64_t           memory_stream_read_int64(memory_stream_t *stream);
char *            memory_stream_read_string(memory_stream_t *stream, int16_t *outlen);

void              memory_stream_read_buffer(memory_stream_t *stream, unsigned char *dest, int len);

void              memory_stream_write_byte(memory_stream_t *stream, unsigned char d);
void              memory_stream_write_int16(memory_stream_t *stream, int16_t d2);
void              memory_stream_write_int32(memory_stream_t *stream, int32_t d4);
void              memory_stream_write_int64(memory_stream_t *stream, int64_t d8);
void              memory_stream_write_string(memory_stream_t *stream, const char* str, int16_t len);

void              memory_stream_write_buffer(memory_stream_t *stream, unsigned char *src, int len);

#ifdef __cplusplus
}
#endif

#endif