/*
  media-filesystem object export code
  tridge@samba.org, September 2002
  released under the Gnu GPL v2

  11/05/04: new version adapted from mfs_import. jamie@DDB/AO

*/

#include <sys/types.h>
#include <sys/socket.h>
#include <sched.h>
#include <time.h>
#include <unistd.h>

#include "mfs.h"


// Write a complete buffer to a file descriptor, handling partial writes
static inline ssize_t writeall( int fd, void *buf, size_t count ) 
{
	unsigned char *p = buf;
	int rc = 0;
	ssize_t ret = 0;

	while (count > 0) {
	  ret = write( fd, p, count );
	  if (ret < 0) {
	    if (errno != EINTR && errno != EAGAIN)
	      return ret;
	    continue;
	  }
	  p += ret;
	  count -= ret;
	  rc += ret;
	}
	return rc;
}

//  
// Read from <fsid> and write to <fd>, starting at byte offset <start> and
// reading <count> bytes.
//
// <nbufs> is the chunk size, in units of 512byte sectors.  If <=0, a
// default is used (currently 256).
//
// <delayms> is a delay to impose between chunks.  -1 means no delay, 0 means just a sched_yield,
// positive is the number of milliseconds to sleep.
//
// If <verbose> is true progress and other status messages will be printed on
// stderr.  Otherwise, only error messages will come out there.
//
// Returns the number of bytes written to fd, or -1 on error.
//
// 
int export_file(const u32 fsid, const int fd, u64 start, 
		 u64 count, int delayms, u32 nbufs, int verbose )
{
        struct mfs_inode inode;
        int run;
        int pct, last_pct=0;
        u64 ofs=0, size;
	u64 total;
	struct timespec delay;
	int nb = (nbufs>0) ? nbufs : 256;

        unsigned char *buf = alloca(nb*SECTOR_SIZE);
	if (!buf) {
		fprintf( stderr, "Couldn't allocate buffer: %d\n", 
			 nb*SECTOR_SIZE );
		exit(1);
	}

	if (delayms > 0) {
		delay.tv_sec = delayms/1000;
		delayms %= 1000;
		delay.tv_nsec = delayms*1000000;
	}

        mfs_load_inode(fsid, &inode);
        size = mfs_fsid_size(fsid);
	if (start>size) {
		return -1;
	}
	if (start+count>size || count==0) {
		count = size-start;
	}
	total = count;

	if (verbose)
		fprintf(stderr,
			"exporting fsid %d of size %lld starting at offset %lld for %lld bytes\n", 
			fsid, size, start, count);

	mfs_readahead(1);

        for (run=0;count>0 && run<inode.num_runs;run++) {
                int len=inode.u.runs[run].len;   //number of sectors left
                int sec=inode.u.runs[run].start; //sector to write to

		if (start > ofs+((u64)len<<SECTOR_SHIFT)) { // are we there yet?
			ofs +=((u64)len<<SECTOR_SHIFT);
			continue; /* nyet */
		}
                while (len>0 && count>0) {
			int rlen, blen, ret;
			if (start > ofs) {
				// Find first sector we want
				int bofs = start-ofs;
				int sofs = bofs>>SECTOR_SHIFT;
				int bmod = bofs&(SECTOR_SIZE-1);
				ofs += bofs;
				len -= sofs;
				sec += sofs;
				// Partial first sector
				if (bmod) {
					int blen = MIN(SECTOR_SIZE-bmod,count);
					int ret;
					mfs_read_sectors(buf,sec,1);
					ret = writeall( fd, buf+bmod, blen);
					if (ret <= 0) {
						perror("write failed:");
						return -1;
					} else if (ret != blen) {
						fprintf(stderr,"short write: %d/%d\n", 
							ret, blen );
					}
					sec++;
					len--;
					count -= ret;
					ofs += blen;
				}
			}
                        rlen = MIN(nb,len);  //number of sectors to read this round
			blen = MIN(rlen<<SECTOR_SHIFT,count);

			mfs_read_partial(buf, sec, blen);
                        ret  = writeall(fd,buf,blen); //number of BYTES read
                        if (ret < 0 || ret != blen) {
				perror("write failed:");
				return -1;
			}
			ofs += ret;
			count -= ret;
                        ret >>= SECTOR_SHIFT; //convert to SECTORS
                        len -= ret;
                        sec += ret;

			// Report progress
			if (verbose) {
				pct = (100 * (total-count))/total;
				if (pct != last_pct) {
					fprintf(stderr,"%d%%\r", pct);
					fflush(stderr);
					last_pct = pct;
				}
			}

			// Delay, if requested
			if (delayms > 0)  /* play nice: throttle bandwidth */
				nanosleep( &delay, NULL ); 
			else if (delayms == 0)
				sched_yield();
                }
        }
	return total-count;
}
