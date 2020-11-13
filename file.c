#include <stdio.h>
#include <assert.h>

#include "file.h"
#include "inode.h"
#include "diskimg.h"

/**
 * Fetches the specified file block from the specified inode.
 * Returns the number of valid bytes in the block, -1 on error.
 */
int file_getblock(struct unixfilesystem *fs, int inumber, int blockNum, void *buf) {
    // fetch inode
    struct inode ino;
    if (inode_iget(fs, inumber, &ino) < 0) {
        return -1;
    }

    // get sector num of inode
    int sector_num = inode_indexlookup(fs, &ino, blockNum);
    if (sector_num < 0) {
        return -1;
    }

    // fetch actual file in block
    if (diskimg_readsector(fs->dfd, sector_num, buf) < 0) {
        return -1;
    }

    // calc number of valid bytes
    int full_size = inode_getsize(&ino);
    int total_blocks_of_file = full_size / DISKIMG_SECTOR_SIZE;
    if (blockNum == total_blocks_of_file) {
        // fetching last block, return the tail size
        return full_size % DISKIMG_SECTOR_SIZE;
    } else {
        // other blocks are all full blocks
        return DISKIMG_SECTOR_SIZE;
    }
}
