/*
  media-filesystem diagnostic utility
  tridge@samba.org, January 2001
  released under the Gnu GPL v2
*/

#include "mfs.h"

static char *prog="";
static void usage(void)
{
	fprintf(stderr,"\n\
usage: %s [options] [<path|fsid>]\n\
\n\
   options:\n\
	-f		Fix the superblock crc, if it is incorrect.\n\
	-h		Display this usage info.\n\
", prog );
	credits();
	exit(1);
}

 int main(int argc, char *argv[])
{
	int c;
	int fix=0;

	while ((c = getopt(argc, argv, "")) != -1 ){
		switch (c) {
		case 'h':
			usage();
		case 'f':
			fix = 1;
			break;
		}
	}

	argc -= optind;
	argv += optind;

	mfs_init_fix(fix);
	mfs_info();

	if (argc > 0) {
		printf("\n");
		mfs_fsid_info(mfs_resolve(argv[0]));
	}

	return 0;
}
