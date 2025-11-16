#define _LARGEFILE64_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h> 
#include <ext2fs/ext2_fs.h> 

int main(int argc, char *argv[]) {
    int fd, i;
    int ino, bgno, index;
    int inode_size, block_size;
    unsigned long inode_offset;
    ssize_t count;

    struct ext2_super_block sb;
    struct ext2_group_desc bgdesc;
    struct ext2_inode inode;

    if (argv[2][0] == '/')
        ino = atoi(argv[2] + 1);
    else
        ino = atoi(argv[2]);

    fd = open(argv[1], O_RDONLY);
    if (fd == -1) {
        perror("open");
        exit(errno);
    }

    // Read superblock
    lseek64(fd, 1024, SEEK_SET);
    count = read(fd, &sb, sizeof(sb));

    printf("=== Superblock ===\n");
    printf("Magic: 0x%x\n", sb.s_magic);
    printf("Inodes Count: %u\n", sb.s_inodes_count);
    printf("Blocks Count: %u\n", sb.s_blocks_count);
    printf("Block size (log): %u\n", sb.s_log_block_size);
    printf("Size of group descriptor = %lu bytes\n", sizeof(struct ext2_group_desc));

    inode_size = sb.s_inode_size;
    block_size = 1024 << sb.s_log_block_size;

    bgno = (ino - 1) / sb.s_inodes_per_group;
    index = (ino - 1) % sb.s_inodes_per_group;

    unsigned long gd_block = (block_size == 1024) ? 2 : 1;
    unsigned long gd_offset = gd_block * block_size + bgno * sizeof(struct ext2_group_desc);
    lseek64(fd, gd_offset, SEEK_SET);
    count = read(fd, &bgdesc, sizeof(bgdesc));

    printf("Inode Table starts at block: %u\n", bgdesc.bg_inode_table);

    unsigned int inodes_per_block = block_size / inode_size;
    unsigned int block_index = index / inodes_per_block;
    unsigned int offset_in_block = (index % inodes_per_block) * inode_size;

    inode_offset = ((unsigned long)bgdesc.bg_inode_table + block_index) * block_size + offset_in_block;

    lseek64(fd, inode_offset, SEEK_SET);
    count = read(fd, &inode, sizeof(inode));
    if (count != sizeof(inode)) { perror("read inode"); exit(1); }

    printf("\n=== Inode %d Information ===\n", ino);
    printf("Mode: %o\n", inode.i_mode);
    printf("UID: %u\n", inode.i_uid);
    printf("GID: %u\n", inode.i_gid);
    printf("Links count: %u\n", inode.i_links_count);
    printf("File size: %u bytes\n", inode.i_size);
    printf("Blocks count: %u\n", inode.i_blocks);

    for (i = 0; i < EXT2_N_BLOCKS; i++) {
        if (inode.i_block[i] != 0)
            printf("Block[%d]: %u\n", i, inode.i_block[i]);
    }

    close(fd);
}

