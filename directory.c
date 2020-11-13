#include "directory.h"
#include "inode.h"
#include "diskimg.h"
#include "file.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

/**
 * Looks up the specified name (name) in the specified directory (dirinumber).  
 * If found, return the directory entry in space addressed by dirEnt.  Returns 0 
 * on success and something negative on failure. 
 */
int directory_findname(struct unixfilesystem *fs, const char *name, int dirinumber, struct direntv6 *dirEnt) {
    // fetch inode
    struct inode ino;
    if (inode_iget(fs, dirinumber, &ino) < 0) {
        return -1;
    }

    // check if inode is directory
    if ((ino.i_mode & IFMT) != IFDIR) {
        return -1;
    }

    // calc block num
    int inode_size = inode_getsize(&ino);
    int blockNum = inode_size / DISKIMG_SECTOR_SIZE;
    if (inode_size % DISKIMG_SECTOR_SIZE != 0) {
        // inode size not aligned, add the last block
        blockNum++;
    }

    // go through each block, read in directory entries in block
    struct direntv6 dir_entries[DISKIMG_SECTOR_SIZE / sizeof(struct direntv6)];
    for (int block = 0; block < blockNum; block++) {
        // fetch block
        int block_size;
        if ((block_size = file_getblock(fs, dirinumber, block, dir_entries)) < 0) {
            return -1;
        }

        int entryNum = block_size / sizeof(struct direntv6);
        // go through each entry, find file of the given name
        for (int entry = 0; entry < entryNum; entry++) {
            // compare directory name with given name
            if (strcmp(dir_entries[entry].d_name, name) == 0) {
                // found, read in content and return succesfully
                *dirEnt = dir_entries[entry];
                return 0;
            }
        }
    }

    // not found
    return -1;
}
