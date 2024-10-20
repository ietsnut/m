#ifndef RESOURCE_H
#define RESOURCE_H

#include <cosmo.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "libc/zip.h"
#include "libc/dce.h" 
/* These are defined in dce.h and can be used to check current platform
#define IsLinux() $_HOSTLINUX,__hostos(%rip)
#define IsMetal() $_HOSTMETAL,__hostos(%rip)
#define IsWindows() $_HOSTWINDOWS,__hostos(%rip)
#define IsBsd() $_HOSTXNU|_HOSTFREEBSD|_HOSTOPENBSD|_HOSTNETBSD,__hostos(%rip)
#define IsXnu() $_HOSTXNU,__hostos(%rip)
#define IsFreebsd() $_HOSTFREEBSD,__hostos(%rip)
#define IsOpenbsd() $_HOSTOPENBSD,__hostos(%rip)
#define IsNetbsd() $_HOSTNETBSD,__hostos(%rip)
*/
#include "third_party/zlib/zlib.h"

#define EOCD_SIZE 22
#define EOCD_MAGIC 0x06054b50
#define CDIR_MAGIC 0x02014b50
#define LFILE_MAGIC 0x04034b50

typedef struct resource_t resource_t;

struct resource_t {
    int (*extract) (char *loc);
    int (*exists) (void);
};

static int resource_extract(char *loc);
static int resource_exists(void);

static resource_t resource = {
    .extract = resource_extract,
    .exists = resource_exists
};

// Function to ensure directories exist
int mkdirs(const char *path) {
    char tmp[PATH_MAX];
    char *p = NULL;
    size_t len;

    snprintf(tmp, sizeof(tmp), "%s", path);
    len = strlen(tmp);
    if (tmp[len - 1] == '/')
        tmp[len - 1] = '\0';
    for (p = tmp + 1; *p; p++)
        if (*p == '/') {
            *p = '\0';
            mkdir(tmp, 0755);
            *p = '/';
        }
    mkdir(tmp, 0755);
    return 0;
}

// Function to extract a file from the ZIP archive
int extract_file(const uint8_t *zip_data, size_t zip_size, const uint8_t *cdir_entry) {
    // Read central directory entry fields
    uint16_t name_len = ZIP_CFILE_NAMESIZE(cdir_entry);
    uint16_t extra_len = ZIP_CFILE_EXTRASIZE(cdir_entry);
    uint16_t comment_len = ZIP_CFILE_COMMENTSIZE(cdir_entry);
    uint16_t comp_method = ZIP_CFILE_COMPRESSIONMETHOD(cdir_entry);
    uint32_t comp_size = ZIP_CFILE_COMPRESSEDSIZE(cdir_entry);
    uint32_t uncomp_size = ZIP_CFILE_UNCOMPRESSEDSIZE(cdir_entry);
    uint32_t local_header_offset = ZIP_CFILE_OFFSET(cdir_entry);

    const char *file_name = (const char *)(cdir_entry + kZipCfileHdrMinSize);

    // Ensure the file name is null-terminated
    char *name = strndup(file_name, name_len);
    if (!name) {
        perror("Memory allocation failed for file name");
        return -1;
    }

    //printf("Extracting: %s (Compression Method: %d)\n", name, comp_method);

    // Skip directory entries
    if (name[name_len - 1] == '/') {
        mkdirs(name);
        free(name);
        return 0;
    }

    // Create directories if needed
    char *dir = strdup(name);
    char *slash = strrchr(dir, '/');
    if (slash) {
        *slash = '\0';
        mkdirs(dir);
    }
    free(dir);

    // Locate the local file header
    if (local_header_offset + kZipLfileHdrMinSize > zip_size) {
        fprintf(stderr, "Invalid local header offset for file: %s\n", name);
        free(name);
        return -1;
    }

    const uint8_t *local_file_hdr = zip_data + local_header_offset;

    if (ZIP_LFILE_MAGIC(local_file_hdr) != kZipLfileHdrMagic) {
        fprintf(stderr, "Invalid local file header magic for file: %s\n", name);
        free(name);
        return -1;
    }

    uint16_t lf_name_len = ZIP_LFILE_NAMESIZE(local_file_hdr);
    uint16_t lf_extra_len = ZIP_LFILE_EXTRASIZE(local_file_hdr);
    uint32_t lf_header_size = kZipLfileHdrMinSize + lf_name_len + lf_extra_len;

    // Locate the compressed data
    const uint8_t *comp_data = local_file_hdr + lf_header_size;

    // Ensure we don't read beyond the zip data
    if (comp_data + comp_size > zip_data + zip_size) {
        fprintf(stderr, "Compressed data goes beyond zip data for file: %s\n", name);
        free(name);
        return -1;
    }

    if (comp_method == kZipCompressionDeflate) {
        // Allocate memory for the uncompressed data
        uint8_t *uncomp_data = malloc(uncomp_size);
        if (!uncomp_data) {
            perror("Memory allocation failed for uncompressed data");
            free(name);
            return -1;
        }

        // Decompress the data using zlib
        z_stream strm = {0};
        strm.next_in = (Bytef *)comp_data;
        strm.avail_in = comp_size;
        strm.next_out = uncomp_data;
        strm.avail_out = uncomp_size;

        if (inflateInit2(&strm, -MAX_WBITS) != Z_OK) {
            fprintf(stderr, "inflateInit failed\n");
            free(uncomp_data);
            free(name);
            return -1;
        }

        int ret = inflate(&strm, Z_FINISH);
        if (ret != Z_STREAM_END) {
            fprintf(stderr, "inflate failed: %d\n", ret);
            inflateEnd(&strm);
            free(uncomp_data);
            free(name);
            return -1;
        }

        inflateEnd(&strm);

        // Write the uncompressed data to a file
        int out_fd = creat(name, 0644);
        if (out_fd == -1) {
            perror("Error creating output file");
            free(uncomp_data);
            free(name);
            return -1;
        }

        if (write(out_fd, uncomp_data, uncomp_size) != uncomp_size) {
            perror("Error writing file data");
            close(out_fd);
            free(uncomp_data);
            free(name);
            return -1;
        }

        close(out_fd);
        free(uncomp_data);
    } else if (comp_method == kZipCompressionNone) {
        // No compression, write the data directly
        int out_fd = creat(name, 0644);
        if (out_fd == -1) {
            perror("Error creating output file");
            free(name);
            return -1;
        }

        if (write(out_fd, comp_data, comp_size) != comp_size) {
            perror("Error writing file data");
            close(out_fd);
            free(name);
            return -1;
        }

        close(out_fd);
    } else {
        fprintf(stderr, "Unsupported compression method: %d\n", comp_method);
        free(name);
        return -1;
    }

    free(name);
    return 0;
}

int resource_extract(char* loc) {
    FILE *f;
    uint32_t exe_size;
    uint32_t total_size, zip_data_length;
    uint8_t *zip_data;
    long pos;

    // Open the executable file (argv[0])
    f = fopen(loc, "rb");
    if (!f) {
        perror("Error opening executable");
        return 1;
    }

    // Get the total size of the file
    fseek(f, 0, SEEK_END);
    pos = ftell(f);
    total_size = pos;

    // Read the last 4 bytes to get the size of the executable part
    fseek(f, -4, SEEK_END);
    uint8_t size_bytes[4];
    if (fread(size_bytes, 1, 4, f) != 4) {
        perror("Error reading size");
        fclose(f);
        return 1;
    }

    // Convert the 4 bytes into a uint32_t (little endian)
    exe_size = size_bytes[0] | (size_bytes[1] << 8) | (size_bytes[2] << 16) | (size_bytes[3] << 24);

    // Calculate the length of the zip data
    zip_data_length = total_size - exe_size - 4;

    // Check if zip data exists
    if (zip_data_length <= 0) {
        printf("No zip data found.\n");
        fclose(f);
        return 1;
    }

    // Allocate memory for the zip data
    zip_data = (uint8_t *)malloc(zip_data_length);
    if (!zip_data) {
        perror("Memory allocation failed");
        fclose(f);
        return 1;
    }

    // Read the zip data from the executable
    fseek(f, exe_size, SEEK_SET);
    if (fread(zip_data, 1, zip_data_length, f) != zip_data_length) {
        perror("Error reading zip data");
        free(zip_data);
        fclose(f);
        return 1;
    }

    fclose(f);

    // Find the End of Central Directory Record (EOCD)
    const uint8_t *eocd = NULL;
    size_t eocd_offset;
    for (eocd_offset = zip_data_length - EOCD_SIZE; eocd_offset > 0; eocd_offset--) {
        if (ZIP_CDIR_MAGIC(zip_data + eocd_offset) == EOCD_MAGIC) {
            eocd = zip_data + eocd_offset;
            break;
        }
    }

    if (!eocd) {
        fprintf(stderr, "EOCD not found\n");
        free(zip_data);
        return 1;
    }

    // Read the central directory offset and size
    uint32_t cdir_size = ZIP_CDIR_SIZE(eocd);
    uint32_t cdir_offset = ZIP_CDIR_OFFSET(eocd);

    if (cdir_offset + cdir_size > zip_data_length) {
        fprintf(stderr, "Invalid central directory offset or size\n");
        free(zip_data);
        return 1;
    }

    const char* os_folder = NULL;
    if (IsLinux()) {
        os_folder = "resource/linux/";
    } else if (IsWindows()) {
        os_folder = "resource/windows/";
    } else if (IsXnu()) {
        os_folder = "resource/mac/";
    } else {
        fprintf(stderr, "Unsupported operating system\n");
        free(zip_data);
        return 1;
    }

    size_t os_folder_len = strlen(os_folder);

    // Parse the central directory entries
    const uint8_t *cdir_ptr = zip_data + cdir_offset;
    const uint8_t *cdir_end = cdir_ptr + cdir_size;

    while (cdir_ptr < cdir_end) {
        if (ZIP_CFILE_MAGIC(cdir_ptr) != CDIR_MAGIC) {
            fprintf(stderr, "Invalid central directory file header magic\n");
            free(zip_data);
            return 1;
        }

        // Read central directory entry fields
        uint16_t name_len = ZIP_CFILE_NAMESIZE(cdir_ptr);
        const char *file_name = (const char *)(cdir_ptr + kZipCfileHdrMinSize);

        // Check if the file is in the correct OS folder
        if (name_len >= os_folder_len && strncmp(file_name, os_folder, os_folder_len) == 0) {
            // Extract the file
            if (extract_file(zip_data, zip_data_length, cdir_ptr) != 0) {
                free(zip_data);
                return 1;
            }
        }

        // Move to the next central directory entry
        uint16_t extra_len = ZIP_CFILE_EXTRASIZE(cdir_ptr);
        uint16_t comment_len = ZIP_CFILE_COMMENTSIZE(cdir_ptr);
        uint32_t cdir_entry_size = kZipCfileHdrMinSize + name_len + extra_len + comment_len;

        cdir_ptr += cdir_entry_size;
    }

    free(zip_data);
    return 0;
}


int resource_exists(void) {
    struct stat st;

    // Check if the directory exists
    if (stat("./resource", &st) == 0 && S_ISDIR(st.st_mode)) {
        return 1; // Directory exists
    }
    
    return 0; // Directory does not exist
}


#endif // RESOURCE_H