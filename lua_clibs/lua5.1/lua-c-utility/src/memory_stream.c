#include "memory_stream.h"

#include <stdlib.h>
#include <memory.h>
#include <assert.h>

#if 1
#define HTONS(A) (A)
#define NTOHS(a) (a) 
#define HTONL(A) (A)
#define NTOHL(a) (a)
#define HTONLL(A) (A)
#define NTOHLL(a) (a)
#else
#define HTONS(A)  ((((A) & 0xff00) >> 8) | ((A) & 0x00ff) << 8)   
#define NTOHS(a)  HTONS(a)
#define HTONL(A)  ((((A) & 0xff000000) >> 24) | (((A) & 0x00ff0000) >> 8) | (((A) & 0x0000ff00) << 8) | (((A) & 0x000000ff) << 24))
#define NTOHL(a)  HTONL(a)
#endif

memory_stream_t *
create_memory_stream(char* buffer, int size)
{
	memory_stream_t *stream = malloc(sizeof(memory_stream_t));
	stream->buffer = buffer;
	stream->cursor = buffer;
	stream->size = size;
	return stream;
}

void
destroy_memory_stream(memory_stream_t *stream)
{
	free(stream);
}

void
reset_memory_stream(memory_stream_t *stream)
{
	memory_stream_seek(stream, 0);
}

void
memory_stream_seek(memory_stream_t *stream, int pos)
{
	stream->cursor = stream->buffer + pos;
}

void
memory_stream_skip(memory_stream_t *stream, int len)
{
	stream->cursor += len;
}

int
memory_stream_get_used_size(memory_stream_t *stream)
{
	return stream->cursor - stream->buffer;
}

int
memory_stream_get_free_size(memory_stream_t *stream){
	return stream->size - (stream->cursor - stream->buffer);
}

int8_t
memory_stream_read_byte(memory_stream_t *stream)
{
	unsigned char d;
	memcpy(&d, stream->cursor, 1);
	stream->cursor += 1;
	return d;
}

int16_t
memory_stream_read_int16(memory_stream_t *stream)
{
	unsigned short d;
	memcpy(&d, stream->cursor, 2);
	d = HTONS(d);
	stream->cursor += 2;
	return d;
}

int32_t
memory_stream_read_int32(memory_stream_t *stream)
{
	int32_t d;
	memcpy(&d, stream->cursor, 4);
	d = HTONL(d);
	stream->cursor += 4;
	return d;
}

int64_t
memory_stream_read_int64(memory_stream_t *stream)
{
	int64_t d;
	memcpy(&d, stream->cursor, 8);
	d = HTONLL(d);
	stream->cursor += 8;
	return d;
}

char*
memory_stream_read_string(memory_stream_t *stream, int16_t *outlen)
{
	int16_t len = memory_stream_read_int16(stream);
	char* p = stream->cursor;
	int n = stream->cursor - stream->buffer + len;
	if (n > stream->size)
	{
		static char g_space[1] = { 0 };
		return g_space;
	}
	stream->cursor += len;
	*outlen = len;
	return p;
}

void
memory_stream_read_buffer(memory_stream_t *stream, unsigned char *dest, int len)
{
	memcpy(dest, stream->cursor, len);
	stream->cursor += len;
}

void
memory_stream_write_byte(memory_stream_t *stream, unsigned char d)
{
	if (stream->cursor - stream->buffer + 1 > stream->size)
		return;
	memcpy(stream->cursor, &d, 1);
	stream->cursor += 1;
}

void
memory_stream_write_int16(memory_stream_t *stream, int16_t d2)
{
	if (stream->cursor - stream->buffer + 2 > stream->size)
		return;
	d2 = HTONS(d2);
	memcpy(stream->cursor, &d2, 2);
	stream->cursor += 2;
}

void
memory_stream_write_int32(memory_stream_t *stream, int32_t d4)
{
	if (stream->cursor - stream->buffer + 4 > stream->size)
		return;
	d4 = HTONL(d4);
	memcpy(stream->cursor, &d4, 4);
	stream->cursor += 4;
}

void
memory_stream_write_int64(memory_stream_t *stream, int64_t d8)
{
	if (stream->cursor - stream->buffer + 8 > stream->size)
		return;
	d8 = HTONLL(d8);
	memcpy(stream->cursor, &d8, 8);
	stream->cursor += 8;
}

void
memory_stream_write_string(memory_stream_t *stream, const char* str, int16_t len)
{
	if (stream->cursor - stream->buffer + len > stream->size)
		return;
	memory_stream_write_int16(stream, len);
	memcpy(stream->cursor, str, len);
	stream->cursor += len;
}

void
memory_stream_write_buffer(memory_stream_t *stream, unsigned char* src, int len)
{
	if (stream->cursor - stream->buffer + len > stream->size)
		return;
	memcpy(stream->cursor, src, len);
	stream->cursor += len;
}