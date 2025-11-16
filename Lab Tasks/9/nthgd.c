//Write a program to print n'th group descriptor from an ext2 file system

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>

#define EXT2_SUPER_OFFSET 1024
#define EXT2_SUPER_MAGIC  0xEF53

// Simplified ext2 superblock
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

struct ext2_group_desc {
    uint32_t bg_block_bitmap;
    uint32_t bg_inode_bitmap;
    uint32_t bg_inode_table;
    uint16_t bg_free_blocks_count;
    uint16_t bg_free_inodes_count;
    uint16_t bg_used_dirs_count;
    uint16_t bg_pad;
    uint8_t  bg_reserved[12];
};

static void usage(char *prog) {
    printf("Usage: %s <fs_image> <group_num>\n", prog);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        usage(argv[0]);
        return 1;
    }

    int fd = open(argv[1], O_RDONLY);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    int group = atoi(argv[2]);
    if (group < 0) {
        printf("Invalid group number\n");
        close(fd);
        return 1;
    }

    struct ext2_super_block sb;
    lseek(fd, EXT2_SUPER_OFFSET, SEEK_SET);
    if (read(fd, &sb, sizeof(sb)) != sizeof(sb)) {
        perror("read superblock");
        close(fd);
        return 1;
    }

    if (sb.s_magic != EXT2_SUPER_MAGIC) {
        printf("Not an ext2 filesystem (magic=%x)\n", sb.s_magic);
        close(fd);
        return 1;
    }

    unsigned block_size = 1024 << sb.s_log_block_size;
    unsigned groups = (sb.s_blocks_count + sb.s_blocks_per_group - 1) / sb.s_blocks_per_group;

    if ((unsigned)group >= groups) {
        printf("Group %d out of range (only %u groups)\n", group, groups);
        close(fd);
        return 1;
    }

    // GDT usually starts at block after superblock
    unsigned gdt_block = (block_size == 1024) ? 2 : 1;
    off_t gd_offset = (off_t)gdt_block * block_size + group * sizeof(struct ext2_group_desc);

    struct ext2_group_desc gd;
    lseek(fd, gd_offset, SEEK_SET);
    if (read(fd, &gd, sizeof(gd)) != sizeof(gd)) {
        perror("read group desc");
        close(fd);
        return 1;
    }

    printf("Group %d Descriptor:\n", group);
    printf("  Block bitmap at: %u\n", gd.bg_block_bitmap);
    printf("  Inode bitmap at: %u\n", gd.bg_inode_bitmap);
    printf("  Inode table at:  %u\n", gd.bg_inode_table);
    printf("  Free blocks:     %u\n", gd.bg_free_blocks_count);
    printf("  Free inodes:     %u\n", gd.bg_free_inodes_count);
    printf("  Used dirs:       %u\n", gd.bg_used_dirs_count);

    close(fd);
    return 0;
}

