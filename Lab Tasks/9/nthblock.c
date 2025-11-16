//Write another program to print n'th data block from an ext2 file system. 

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>

#define EXT2_SUPER_OFFSET 1024
#define EXT2_SUPER_MAGIC  0xEF53

// minimal ext2 superblock (fields we need)
struct ext2_super_block {
    uint32_t s_inodes_count;
    uint32_t s_blocks_count;
    uint32_t s_r_blocks_count;
    uint32_t s_free_blocks_count;
    uint32_t s_free_inodes_count;
    uint32_t s_first_data_block;
    uint32_t s_log_block_size;
    uint32_t s_log_frag_size;
    uint32_t s_blocks_per_group;
    uint32_t s_frags_per_group;
    uint32_t s_inodes_per_group;
    uint32_t s_mtime;
    uint32_t s_wtime;
    uint16_t s_mnt_count;
    uint16_t s_max_mnt_count;
    uint16_t s_magic;
    uint16_t s_state;
    uint16_t s_errors;
    uint16_t s_minor_rev_level;
    uint32_t s_lastcheck;
    uint32_t s_checkinterval;
    uint32_t s_creator_os;
    uint32_t s_rev_level;
    uint16_t s_def_resuid;
    uint16_t s_def_resgid;
    uint32_t s_first_ino;
    uint16_t s_inode_size;
    uint16_t s_block_group_nr;
    uint32_t s_feature_compat;
    uint32_t s_feature_incompat;
    uint32_t s_feature_ro_compat;
    uint8_t  s_uuid[16];
    uint8_t  s_volume_name[16];
    uint8_t  s_last_mounted[64];
    uint32_t s_algorithm_usage_bitmap;
};

static void usage(char *prog) {
    printf("Usage: %s <imagefile> <blockno> [-x|-a]\n", prog);
    printf("   -x : hex dump\n");
    printf("   -a : ascii dump (printable only)\n");
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        usage(argv[0]);
        return 1;
    }

    int fd = open(argv[1], O_RDONLY);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    int blk = atoi(argv[2]);
    char mode = 'x';   // default hex
    if (argc > 3) {
        if (!strcmp(argv[3], "-a")) mode = 'a';
        else if (!strcmp(argv[3], "-x")) mode = 'x';
        else {
            usage(argv[0]);
            close(fd);
            return 1;
        }
    }

    // read superblock
    struct ext2_super_block sb;
    lseek(fd, EXT2_SUPER_OFFSET, SEEK_SET);
    if (read(fd, &sb, sizeof(sb)) != sizeof(sb)) {
        perror("read superblock");
        close(fd);
        return 1;
    }
    if (sb.s_magic != EXT2_SUPER_MAGIC) {
        printf("Not an ext2 filesystem\n");
        close(fd);
        return 1;
    }

    unsigned block_size = 1024 << sb.s_log_block_size;
    if (blk >= (int)sb.s_blocks_count) {
        printf("Block %d out of range (only %u blocks)\n", blk, sb.s_blocks_count);
        close(fd);
        return 1;
    }

    // seek and read block
    off_t offset = (off_t)blk * block_size;
    lseek(fd, offset, SEEK_SET);

    unsigned char *buf = malloc(block_size);
    if (!buf) {
        perror("malloc");
        close(fd);
        return 1;
    }

    if (read(fd, buf, block_size) != (ssize_t)block_size) {
        perror("read block");
        free(buf);
        close(fd);
        return 1;
    }

    // dump block
    if (mode == 'x') {
        printf("Hex dump of block %d:\n", blk);
        for (unsigned i = 0; i < block_size; i++) {
            if (i % 16 == 0) printf("%08x  ", i);
            printf("%02x ", buf[i]);
            if ((i+1) % 16 == 0) {
                printf(" ");
                for (unsigned j = i-15; j <= i; j++) {
                    printf("%c", isprint(buf[j]) ? buf[j] : '.');
                }
                printf("\n");
            }
        }
    } else if (mode == 'a') {
        printf("ASCII dump of block %d:\n", blk);
        for (unsigned i = 0; i < block_size; i++) {
            char c = buf[i];
            if (isprint(c) || c == '\n' || c == '\t')
                putchar(c);
            else
                putchar('.');
        }
        printf("\n");
    }

    free(buf);
    close(fd);
    return 0;
}

