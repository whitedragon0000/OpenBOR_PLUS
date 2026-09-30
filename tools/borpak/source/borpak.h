#ifndef BORPAK_H
#define BORPAK_H

#ifndef WIN32
	#ifndef _FILE_OFFSET_BITS
		#define _FILE_OFFSET_BITS 64
	#endif
	#ifndef _LARGEFILE_SOURCE
		#define _LARGEFILE_SOURCE 1
	#endif
	#ifndef _GNU_SOURCE
		#define _GNU_SOURCE 1
	#endif
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <limits.h>

/*
* Maximum source path and maximum archive path lengths. 
*
* Source is the path on disk to the file being packed. 
* Archive is the path within the PAK archive. We keep
* separate limits because the source path may be and 
* probably is much longer than the archive path, and
* we need to read it in full to get the file data, but
* only the archive path length is relevant for the PAK 
* format.
*
* In practice it usually doesn't matter - Windows paths
* are limited to 260 chars by default.
*
* ex: source path: 	sysdrive/openbor_projects/mygame/data/chars/johndoe/idle_0.png
*     archive path: data/chars/johndoe/idle_0.png
*
* Note the engine requires a buffer size at least equal
* to the maximum archive path length.
*
*/
#define ARCHIVE_PATH_MAX_LEN	512U	
#define SOURCE_PATH_MAX_LEN		1024U

#define PACK_HEADER_SIZE		8U
#define PACK_MAGIC_BYTE_COUNT 	4U

#define PAK32_ENTRY_BASE_SIZE 	12U
#define PAK64_ENTRY_BASE_SIZE 	20U
#define PAK32_FOOTER_SIZE     	4U
#define PAK64_FOOTER_SIZE     	8U

/*
* Caskey, Damon V.
* 2026-06-10
*
* Local type definitions, just to make things
* a bit more self-documenting and future adjustable. 
*/
typedef uint32_t pak_u32;
typedef uint64_t pak_offset_t;
typedef uint64_t pak_size_t;

/*
* Non-streamed assets are capped to the engine's int-sized
* public read API. PAK64 may carry larger streamed media
* when the engine has a dedicated 64-bit seek/tell path for it.
*/
#define PAK_NONSTREAMED_ASSET_LIMIT ((pak_size_t)INT_MAX)

/*
* PAK32 is the compatibility format for older 
* engines, so the packer keeps its completed 
* archives under the signed 32-bit file-position 
* boundary those builds commonly rely on. 
*
* The data section stops at 2,000,000,000 bytes, 
* leaving the remaining archive budget for every 
* file table record plus the final 32-bit footer.
*
* Technically PAK32 format can support up to 4GB 
* now, and so can latest engine, but there's no
* use case. Any engine builds that can safely read 
* >2GB PAK32 archives also support PAK64 format.
*/
#define PAK32_ARCHIVE_LIMIT ((pak_size_t)INT_MAX)
#define PAK32_DATA_LIMIT    ((pak_size_t)2000000000ULL)
#define PAK32_TABLE_CUSHION (PAK32_ARCHIVE_LIMIT - PAK32_DATA_LIMIT)

/*
* PAK format enumeration. The packer uses this 
* to differentiate between the 32-bit and 64-bit
* PAK formats.
*/
typedef enum {
	PAK_FORMAT_PAK32 = 0U,
	PAK_FORMAT_PAK64 = 1U
} pak_format_t;

/*
* One file table record kept in memory until 
* the archive footer table is written.
*/
typedef struct {
	pak_u32      record_size;
	pak_offset_t data_offset;
	pak_size_t   data_size;
	char         *name;
} pak_entry_t;

/*
* Linked list node for deferred table records.
* File data is written first, then this list is 
* walked to write the final archive table.
*/
typedef struct pak_entry_node_t {
	pak_entry_t              entry;
	struct pak_entry_node_t *next;
} pak_entry_node_t;

void file_write(const void *buffer, size_t size, FILE *file_object);
int pack_file(FILE *pak_file_object, const char *source_file_path, const char *archive_file_path, pak_format_t format);
void extract_file(FILE *pak_file_object, char *file_path, pak_offset_t data_offset, pak_size_t data_size);
pak_u32 read_le_uint(FILE *pak_file_object, int bits);
uint64_t read_le_u64(FILE *pak_file_object);
void write_le_uint(FILE *pak_file_object, pak_u32 num, int bits);
void write_le_u64(FILE *pak_file_object, uint64_t value);
int pack_directory(FILE *pak_file_object, const char *source_directory_path, const char *archive_directory_path, pak_format_t format);
void write_err(void);
void std_err(void);
pak_offset_t pak_tell(FILE *pak_file_object);
pak_size_t pak_file_size(FILE *file_object);
void pak_seek_relative(FILE *pak_file_object, int64_t offset, int origin);
void pak_seek_to_offset(FILE *pak_file_object, pak_offset_t offset);
void validate_pak32_budget(const char *file_path, pak_offset_t data_offset, pak_size_t data_size, pak_u32 record_size);
int copy_archive_path(char *destination, size_t destination_size, const char *source);
int validate_archive_path(const char *archive_path, const char *source_file_path);
void copy_archive_root_from_input_directory(char *destination, size_t destination_size, const char *input_directory);
void print_format_details(pak_format_t format);
int is_unsafe_path(const char *name);
const char *format_name(pak_format_t format);

#ifdef BORPAK_WINPAUSE
void winpause(void);
#endif

#endif