/*
  media-filesystem diagnostic utility
  tridge@samba.org, January 2001
  released under the Gnu GPL v2
*/

#include "mfs.h"

 int main(int argc, char *argv[])
{
	int c;

	while ((c = getopt(argc, argv, "")) != -1 ){
		switch (c) {
		}
	}

	argc -= optind;
	argv += optind;

	mfs_init();
	mfs_info();

	if (argc > 0) {
		printf("\n");
		mfs_fsid_info(mfs_resolve(argv[0]));
	}

	return 0;
}
