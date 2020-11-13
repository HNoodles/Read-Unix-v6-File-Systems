#include <stdio.h>
#include <assert.h>

#include "inode.h"
#include "diskimg.h"

#define INDIRECT_SECTOR_NUM 7

/**
 * Fetches the specified inode from the filesystem. 
 * Returns 0 on success, -1 on error.  
 */
int inode_iget(struct unixfilesystem *fs, int inumber, struct inode *inp) {
    // inumber starts from 1, off by one
    inumber = inumber - 1;

    // get offset of sector and inumber
    int inode_num_in_sector = DISKIMG_SECTOR_SIZE / sizeof(struct inode);
    int sector_offset = inumber / inode_num_in_sector;
    int inumber_offset = inumber % inode_num_in_sector;

    // get contents of a sector
    struct inode inodes[inode_num_in_sector];
    int result = diskimg_readsector(fs->dfd, INODE_START_SECTOR + sector_offset, inodes);

    // get contents of an inode
    *inp = inodes[inumber_offset];

    return result < 0 ? -1 : 0;
}


/**
 * Given an index of a file block, retrieves the file's actual block number
 * of from the given inode.
 *
 * Returns the disk block number on success, -1 on error.  
 */
int inode_indexlookup(struct unixfilesystem *fs, struct inode *inp, int blockNum) {
    if ((inp->i_mode & ILARG) == 0) {
        // inode is small file, return block number stored in i_addr directly
        return inp->i_addr[blockNum];
    }

    // inode is large file
    int addr_num_in_sector = DISKIMG_SECTOR_SIZE / sizeof(typeof(inp->i_addr[0]));
    int indirect_addr_num = INDIRECT_SECTOR_NUM * addr_num_in_sector;

    // find sector of addr
    int sector_of_addr;
    if (blockNum < indirect_addr_num) { // in first indirect block
        int sector_offset = blockNum / addr_num_in_sector;
        sector_of_addr = inp->i_addr[sector_offset];
    } else { // in doubly indirect block
        // get last block of indirect addr
        uint16_t doubly_sectors[addr_num_in_sector];
        int result = diskimg_readsector(fs->dfd, inp->i_addr[INDIRECT_SECTOR_NUM], doubly_sectors);
        if (result < 0) {
            // error
            return -1;
        }

        // remove bias
        blockNum -= indirect_addr_num;
        int sectorNum = blockNum / addr_num_in_sector;

        // set number of sector that contain targeted addr
        sector_of_addr = doubly_sectors[sectorNum];
    }

    // get offset of sector and addr
    int addr_offset = blockNum % addr_num_in_sector;

    // get content addrs of the sector
    uint16_t addrs[addr_num_in_sector];
    int result = diskimg_readsector(fs->dfd, sector_of_addr, addrs);

    // return error or block number retrieved
    return result < 0 ? -1 : addrs[addr_offset];
}


/**
 * Computes the size in bytes of the file identified by the given inode
 */
int inode_getsize(struct inode *inp) {
    return ((inp->i_size0 << 16) | inp->i_size1);
}
