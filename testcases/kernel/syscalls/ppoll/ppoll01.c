/******************************************************************************/
/* Copyright (c) Crackerjack Project., 2007-2008 ,Hitachi, Ltd                */
/*          Author(s): Takahiro Yasui <takahiro.yasui.mp@hitachi.com>,	      */
/*		       Yumiko Sugita <yumiko.sugita.yf@hitachi.com>, 	      */
/*		       Satoshi Fujiwara <sa-fuji@sdl.hitachi.co.jp>	      */
/*                                                                  	      */
/* This program is free software;  you can redistribute it and/or modify      */
/* it under the terms of the GNU General Public License as published by       */
/* the Free Software Foundation; either version 2 of the License, or          */
/* (at your option) any later version.                                        */
/*                                                                            */
/* This program is distributed in the hope that it will be useful,            */
/* but WITHOUT ANY WARRANTY;  without even the implied warranty of            */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See                  */
/* the GNU General Public License for more details.                           */
/*                                                                            */
/* You should have received a copy of the GNU General Public License          */
/* along with this program;  if not, write to the Free Software               */
/* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA    */
/*                                                                            */
/******************************************************************************/
/******************************************************************************/
/*                                                                            */
/* File:        ppoll01.c                                                     */
/*                                                                            */
/* Description: This tests the ppoll01() syscall                              */
/*									      */
/* 									      */
/*									      */
/*									      */
/*									      */
/*                                                                            */
/* Usage:  <for command-line>                                                 */
/* ppoll01 [-c n] [-e][-i n] [-I x] [-p x] [-t]                               */
/*      where,  -c n : Run n copies concurrently.                             */
/*              -e   : Turn on errno logging.                                 */
/*              -i n : Execute test n times.                                  */
/*              -I x : Execute test for x seconds.                            */
/*              -P x : Pause for x seconds between iterations.                */
/*              -t   : Turn on syscall timing.                                */
/*                                                                            */
/* Total Tests: 1                                                             */
/*                                                                            */
/* Test Name:   ppoll01                                                       */
/* History:     Porting from Crackerjack to LTP is done by                    */
/*              Manas Kumar Nayak maknayak@in.ibm.com>                        */
/******************************************************************************/
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/wait.h>
#include <getopt.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <libgen.h>
#include <limits.h>
#include <signal.h>
#include "asm/poll.h"


#include "../utils/include_j_h.h"
#include "../utils/common_j_h.c"

/* Harness Specific Include Files. */
#include "test.h"
#include "usctest.h"
#include "linux_syscall_numbers.h"

/* Extern Global Variables */
extern int Tst_count;           /* counter for tst_xxx routines.         */
extern char *TESTDIR;           /* temporary dir created by tst_tmpdir() */

/* Global Variables */
char *TCID = "ppoll01";  /* Test program identifier.*/
int  testno;
int  TST_TOTAL = 1;                   /* total number of tests in this file.   */

/* Extern Global Functions */
/******************************************************************************/
/*                                                                            */
/* Function:    cleanup                                                       */
/*                                                                            */
/* Description: Performs all one time clean up for this test on successful    */
/*              completion,  premature exit or  failure. Closes all temporary */
/*              files, removes all temporary directories exits the test with  */
/*              appropriate return code by calling tst_exit() function.       */
/*                                                                            */
/* Input:       None.                                                         */
/*                                                                            */
/* Output:      None.                                                         */
/*                                                                            */
/* Return:      On failure - Exits calling tst_exit(). Non '0' return code.   */
/*              On success - Exits calling tst_exit(). With '0' return code.  */
/*                                                                            */
/******************************************************************************/
extern void cleanup() {
        /* Remove tmp dir and all files in it */
        TEST_CLEANUP;
        tst_rmdir();

        /* Exit with appropriate return code. */
        tst_exit();
}

/* Local  Functions */
/******************************************************************************/
/*                                                                            */
/* Function:    setup                                                         */
/*                                                                            */
/* Description: Performs all one time setup for this test. This function is   */
/*              typically used to capture signals, create temporary dirs      */
/*              and temporary files that may be used in the course of this    */
/*              test.                                                         */
/*                                                                            */
/* Input:       None.                                                         */
/*                                                                            */
/* Output:      None.                                                         */
/*                                                                            */
/* Return:      On failure - Exits by calling cleanup().                      */
/*              On success - returns 0.                                       */
/*                                                                            */
/******************************************************************************/
void setup() {
        /* Capture signals if any */
        /* Create temporary directories */
        TEST_PAUSE;
        tst_tmpdir();
}


/*
 * Macros
 */
#define SYSCALL_NAME    "ppoll"

#ifndef POLLRDHUP
#  define POLLRDHUP     0x2000
#endif


/*
 * Global variables
 */
static int opt_debug;
static char *progname;
static char *progdir;

enum test_type {
	NORMAL,
        MASK_SIGNAL,
        TIMEOUT,
        FD_ALREADY_CLOSED,
        SEND_SIGINT,
        INVALID_NFDS,
        INVALID_FDS,
        MINUS_NSEC,
        TOO_LARGE_NSEC,

};


/*
 * Data Structure
 */
struct test_case {
	short expect_revents;
        int ttype;
        int ret;
        int err;
};


/* Test cases
 *
 *   test status of errors on man page
 *
 *   EBADF              can't check because EBADF never happen even though
 *                      fd was invalid. In this case, information of invalid
 *                      fd is set in revents
 *   EFAULT             v ('fds' array in the invalid address space)
 *   EINTR              v (a non blocked signal was caught)
 *   EINVAL             v ('nfds' is over the 'RLIMIT_NOFILE' value)
 *   ENOMEM             can't check because it's difficult to create no-memory
 */


static struct test_case tcase[] = {
	{ // case00
                .ttype          = NORMAL,
                .expect_revents = POLLOUT,
                .ret            = 0,
                .err            = 0,
        },
        { // case01
                .ttype          = MASK_SIGNAL,
                .expect_revents = 0, // don't care
                .ret            = 0,
                .err            = 0,
        },
        { // case02
                .ttype          = TIMEOUT,
                .expect_revents = 0, // don't care
                .ret            = 0,
                .err            = 0,
        },
        { // case03
                .ttype          = FD_ALREADY_CLOSED,
                .expect_revents = POLLNVAL,
                .ret            = 0,
                .err            = 0,
        },
        { // case04
                .ttype          = SEND_SIGINT,
                .ret            = -1,
                .err            = EINTR,
        },
        { // case05
                .ttype          = INVALID_NFDS,
                .ret            = -1,
                .err            = EINVAL,
        },
        { // case06
                .ttype          = INVALID_FDS,
                .ret            = -1,
                .err            = EFAULT,
        },
#if 0
        { // case07
                .ttype          = MINUS_NSEC,
                .ret            = -1,
                .err            = EINVAL, // RHEL4U1 + 2.6.18 returns SUCCESS
        },
        { // case08
                .ttype          = TOO_LARGE_NSEC,
                .ret            = -1,
                .err            = EINVAL, // RHEL4U1 + 2.6.18 returns SUCCESS
        },
#endif
};

#define NUM_TEST_FDS    1

/*
 * do_test()
 *
 *   Input  : TestCase Data
 *   Return : RESULT_OK(0), RESULT_NG(1)
 *
 */

static int do_test(struct test_case *tc)
{
        int sys_ret;
        int sys_errno;
        int result = RESULT_OK;
	int fd = -1 , cmp_ok = 1;
	char fpath[PATH_MAX];
	struct pollfd *p_fds, fds[NUM_TEST_FDS];
        unsigned int nfds = NUM_TEST_FDS;
        struct timespec *p_ts, ts;
        sigset_t *p_sigmask, sigmask;
        size_t sigsetsize = 0;
        pid_t pid = 0;

	TEST(fd = setup_file(progdir, "test.file", fpath));
        if (fd < 0)
                return 1;
        fds[0].fd = fd;
        fds[0].events = POLLIN | POLLPRI | POLLOUT | POLLRDHUP;
        fds[0].revents = 0;
        p_fds = fds;
        p_ts = NULL;
        p_sigmask = NULL;

	switch (tc->ttype) {
        case TIMEOUT:
                nfds = 0;
                ts.tv_sec = 0;
                ts.tv_nsec = 50000000;  // 50msec
                p_ts = &ts;
                break;
	case FD_ALREADY_CLOSED:
                TEST(close(fd));
                fd = -1;
                TEST(cleanup_file(fpath));
                break;
        case MASK_SIGNAL:
                TEST(sigemptyset(&sigmask));
                TEST(sigaddset(&sigmask, SIGINT));
                p_sigmask = &sigmask;
                //sigsetsize = sizeof(sigmask);
                sigsetsize = 8;
                nfds = 0;
                ts.tv_sec = 0;
                ts.tv_nsec = 300000000; // 300msec => need to be enough for
                                        //   waiting the signal
                p_ts = &ts;
                // fallthrough
	case SEND_SIGINT:
                nfds = 0;
                TEST(pid = create_sig_proc(100000, SIGINT)); // 100msec
                if (pid < 0)
                        return 1;
                break;
        case INVALID_NFDS:
                //nfds = RLIMIT_NOFILE + 1; ==> RHEL4U1 + 2.6.18 returns SUCCESS
                nfds = -1;
                break;
        case INVALID_FDS:
                p_fds = (void*)0xc0000000;
                break;
        case MINUS_NSEC:
                ts.tv_sec = 0;
                ts.tv_nsec = -1;
                p_ts = &ts;
                break;
	case TOO_LARGE_NSEC:
                ts.tv_sec = 0;
                ts.tv_nsec = 1000000000;
                p_ts = &ts;
                break;
        }

	/*
	   * Execute system call
         */
        errno = 0;
        TEST(sys_ret = syscall(__NR_ppoll, p_fds, nfds, p_ts, p_sigmask, sigsetsize));
        sys_errno = errno;
        if (sys_ret <= 0 || tc->ret < 0)
                goto TEST_END;

        cmp_ok = fds[0].revents == tc->expect_revents;
        if (opt_debug) {
                tst_resm(TINFO,"EXPECT: revents=0x%04x", tc->expect_revents);
                tst_resm(TINFO,"RESULT: revents=0x%04x", fds[0].revents);
        }

TEST_END:
        /*
         * Check results
         */
        result |= (sys_errno != tc->err) || !cmp_ok;
        PRINT_RESULT_CMP(sys_ret >= 0, tc->ret, tc->err, sys_ret, sys_errno,
                         cmp_ok);

        if (fd >= 0)
                cleanup_file(fpath);
        if (pid > 0) {
                int st;
                kill(pid, SIGTERM);
                wait(&st);
        }
	return result;
}


/*
 * sighandler()
 */
void sighandler(int sig)
{
        if (sig == SIGINT)
                return;
        // NOTREACHED
        return;
}


/*
 * usage()
 */

static void usage(const char *progname)
{
        tst_resm(TINFO,"usage: %s [options]", progname);
        tst_resm(TINFO,"This is a regression test program of %s system call.",SYSCALL_NAME);
        tst_resm(TINFO,"options:");
        tst_resm(TINFO,"    -d --debug           Show debug messages");
        tst_resm(TINFO,"    -h --help            Show this message");
        tst_resm(TINFO,"NG");
        exit(1);
}


/*
 * main()
 */



int main(int ac, char **av) {
	int result = RESULT_OK;
        int c;
        int i;
        int lc;                 /* loop counter */
        char *msg;              /* message returned from parse_opts */

	struct option long_options[] = {
                { "debug", no_argument, 0, 'd' },
                { "help",  no_argument, 0, 'h' },
                { NULL, 0, NULL, 0 }
        };

	progname = strchr(av[0], '/');
        progname = progname ? progname + 1 : av[0];	

	progdir = strdup(av[0]);
        progdir = dirname(progdir);
	
        /* parse standard options */
        if ((msg = parse_opts(ac, av, (option_t *)NULL, NULL)) != (char *)NULL){
             tst_brkm(TBROK, cleanup, "OPTION PARSING ERROR - %s", msg);
             tst_exit();
           }

        setup();

        /* Check looping state if -i option given */
        for (lc = 0; TEST_LOOPING(lc); ++lc) {
                Tst_count = 0;
                for (testno = 0; testno < TST_TOTAL; ++testno) {
			 TEST(c = getopt_long(ac, av, "dh", long_options, NULL));
			 while(TEST_RETURN != -1) {
		                switch (c) {
                		case 'd':
		                        opt_debug = 1;
                		        break;
		                default:
                		        usage(progname);
                        		// NOTREACHED
                		}
		        }


		if (ac != optind) {
        	        tst_resm(TINFO,"Options are not match.");
                	usage(progname);
                	// NOTREACHED
	        }

		/*
		* Execute test
         	*/
	        for (i = 0; i < (int)(sizeof(tcase) / sizeof(tcase[0])); i++) {
        	        int ret;
	                tst_resm(TINFO,"(case%02d) START", i);
	                ret = do_test(&tcase[i]);
	                tst_resm(TINFO,"(case%02d) END => %s", i, (ret == 0) ? "OK" : "NG");
	                result |= ret;
        	}
		
		/*
        	 * Check results
         	*/
        	switch(result) {
	        case RESULT_OK:
        			tst_resm(TPASS, "ppoll01 call succeeded ");
		                break;

	        default:
                 	   	tst_resm(TFAIL, "%s failed - errno = %d : %s", TCID, TEST_ERRNO, strerror(TEST_ERRNO));
        		        RPRINTF("NG");
				cleanup();
				tst_exit();
		                break;
        	}

                }
        }	
        cleanup();
	tst_exit();
}
