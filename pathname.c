
#include "pathname.h"
#include "directory.h"
#include "inode.h"
#include "diskimg.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

#define SEPARATOR "/"

int pathname_recursive_lookup(struct unixfilesystem *fs, const char *pathname, int dirinumber);

/**
 * Returns the inode number associated with the specified pathname.  This need only
 * handle absolute paths.  Returns a negative number (-1 is fine) if an error is
 * encountered.
 */
int pathname_lookup(struct unixfilesystem *fs, const char *pathname) {
    if (strcmp(pathname, SEPARATOR) == 0) {
        // is root
        return ROOT_INUMBER;
    } else {
        // not root, start from root, find pathname with first separator("/") removed
        return pathname_recursive_lookup(fs, pathname + strlen(SEPARATOR), ROOT_INUMBER);
    }
}

/**
 * Returns the inode number associated with the specified pathname in the given
 * directory inode.  Returns a negative number (-1 is fine) if an error is
 * encountered.
 */
int pathname_recursive_lookup(struct unixfilesystem *fs, const char *pathname, int dirinumber) {
    char* path_from_first_separator = strchr(pathname, '/');

    if (path_from_first_separator == NULL) {
        // plain name, fetch entry
        struct direntv6 entry;
        if (directory_findname(fs, pathname, dirinumber, &entry) < 0) {
            return -1;
        }
        return entry.d_inumber;
    } else {
        // is directory, find sub pathname
        const char* sub_pathname = path_from_first_separator + strlen(SEPARATOR);

        // construct parent directory name
        int parent_dir_len = strlen(pathname) - strlen(sub_pathname);
        char parent_dir_name[parent_dir_len];
        strncpy(parent_dir_name, pathname, parent_dir_len);
        parent_dir_name[parent_dir_len - 1] = '\0'; // remove last separator "/"

        // read in parent directory
        struct direntv6 entry;
        if (directory_findname(fs, parent_dir_name, dirinumber, &entry) < 0) {
            return -1;
        }

        // look up in parent directory
        return pathname_recursive_lookup(fs, sub_pathname, entry.d_inumber);
    }
}