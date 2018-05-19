/*
 * Copyright 1999-2001 Red Hat, Inc.
 * 
 * All Rights Reserved.
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 * Except as contained in this notice, the name of Red Hat shall not be
 * used in advertising or otherwise to promote the sale, use or other dealings
 * in this Software without prior written authorization from Red Hat.
 *
 */

#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <net/ethernet.h>
#include <net/if.h>
#include <net/if_packet.h>
#include <net/route.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <popt.h>
#include <resolv.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/utsname.h>
#include <sys/wait.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>

#include "config.h"
#include "pump.h"

int verbose = 0;
#if !UDEB
int bootp_client_port;
int bootp_server_port;
#endif

#define N_(foo) (foo)

#define PROGNAME "pump"
#define CONTROLSOCKET "/var/run/pump.sock"

#define _(foo) ((foo))
#include <stdarg.h>

struct command {
    enum { CMD_STARTIFACE, CMD_RESULT, CMD_DIE, CMD_STOPIFACE, 
	   CMD_FORCERENEW, CMD_REQSTATUS, CMD_STATUS } type;
    union {
	struct {
	    char device[20];
	    int flags;
	    int reqLease;			/* in seconds */
	    char reqHostname[200];
	    struct pumpOverrideInfo override;
	} start;
	int result;				/* 0 for success */
	struct {
	    char device[20];
	} stop;
	struct {
	    char device[20];
	} renew;
	struct {
	    char device[20];
	} reqstatus;
	struct {
	    struct pumpNetIntf intf;
	    char hostname[1024];
	    char domain[1024];
	    char bootFile[1024];
	    char nisDomain[1024];
	} status;
    } u;
};

char * readSearchPath(void) {
    int fd;
    struct stat sb;
    char * buf;
    char * start;

    fd = open("/etc/resolv.conf", O_RDONLY);
    if (fd < 0) return NULL;

    fstat(fd, &sb);
    buf = alloca(sb.st_size + 2);
    if (read(fd, buf, sb.st_size) != sb.st_size) return NULL;
    buf[sb.st_size] = '\n';
    buf[sb.st_size + 1] = '\0';
    close(fd);

    start = buf;
    while (start && *start) {
	while (isspace(*start) && (*start != '\n')) start++;
	if (*start == '\n') {
	    start++;
	    continue;
	}

	if (!strncmp("search", start, 6) && isspace(start[6])) {
	    start += 6;
	    while (isspace(*start) && *start != '\n') start++;
	    if (*start == '\n') return NULL;

	    buf = strchr(start, '\n');
	    *buf = '\0';
	    return strdup(start);
	}

	while (*start && (*start != '\n')) start++;
    }

    return NULL;
}

static void createResolvConf(struct pumpNetIntf * intf, struct pumpOverrideInfo * override, char * domain) {
    FILE * f;
    int i;
    char * chptr;
    int resolvconf;

    /* force a reread of /etc/resolv.conf if we need it again */
    res_close();
    endhostent();

    if (!domain) {
	domain = readSearchPath();
 	if (domain) {
	    chptr = alloca(strlen(domain) + 1);
	    strcpy(chptr, domain);
	    free(domain);
	    domain = chptr;
	}
    }

    resolvconf = !(override->flags & OVERRIDE_FLAG_NORESOLVCONF);
    if (resolvconf) {
	struct stat buf;

	if (stat("/sbin/resolvconf", &buf) < 0)
	    resolvconf = 0;
    }

    if (resolvconf) {
	char *arg;

	f = NULL;
	if (asprintf(&arg, "/sbin/resolvconf -a %s", intf->device) >= 0) {
	    f = popen(arg, "w");
	    free(arg);
	}
    } else
	f = fopen("/etc/resolv.conf", "w");

    if (!f) {
	syslog(LOG_ERR, "cannot open resolv.conf: %s\n", strerror(errno));
	return;
    }

    if (domain)
	fprintf(f, "search %s\n", domain);

    for (i = 0; i < intf->numDns; i++)
	fprintf(f, "nameserver %s\n", inet_ntoa(intf->dnsServers[i]));

    if ((resolvconf ? pclose(f) : fclose(f)) != 0)
	syslog(LOG_ERR, "failed to update resolv.conf: %s\n", strerror(errno));
}

void setupDomain(struct pumpNetIntf * intf, 
		 struct pumpOverrideInfo * override) {
    int bufSize = 128;
    char * buf = NULL;

    if (override->flags & OVERRIDE_FLAG_NONISDOMAIN)
	return;
    if (!(intf->set & PUMP_NETINFO_HAS_NISDOMAIN))
	return;

    buf = malloc(bufSize);
    while (getdomainname(buf, bufSize)) {
	if (errno != EINVAL) {
	    syslog(LOG_ERR, "failed to get domainname: %s", strerror(errno));
	    return;
	}

	buf += 128;
    }

    /* if the domainname is set, then don't override it */
    if (strcmp(buf, "localdomain") && strcmp(buf, "")) {
	return;
    }

    if (setdomainname(intf->domain, strlen(intf->domain))) {
	syslog(LOG_ERR, "failed to set domainname: %s", strerror(errno));
	return ;
    }

    return;
}

void setupDns(struct pumpNetIntf * intf, struct pumpOverrideInfo * override) {
    char * hn, * dn = NULL;
    struct hostent * he;

    if (override->flags & OVERRIDE_FLAG_NODNS) {
	return;
    }

    if (override->searchPath[0]) {
	createResolvConf(intf, override, override->searchPath);
	return;
    }

    if (intf->set & PUMP_NETINFO_HAS_DNS) {
	if (!(intf->set & PUMP_NETINFO_HAS_DOMAIN))  {
	    if (intf->set & PUMP_NETINFO_HAS_HOSTNAME) {
		hn = intf->hostname;
	    } else {
		createResolvConf(intf, override, NULL);

		he = gethostbyaddr((char *) &intf->ip, sizeof(intf->ip),
				   AF_INET);
		if (he) {
		    hn = he->h_name;
		} else {
		    hn = NULL;
		}
	    }

	    if (hn) {
		dn = strchr(hn, '.');
		if (dn)
		    dn++;
	    }
	} else {
	    dn = intf->domain;
	}

	createResolvConf(intf, override, dn);
    }
}

void unsetupDns(struct pumpNetIntf * intf, struct pumpOverrideInfo * override) {
    struct stat buf;
    char *arg;

    if (override->flags & OVERRIDE_FLAG_NODNS)
	return;
    if (override->flags & OVERRIDE_FLAG_NORESOLVCONF)
	return;
    if (stat("/sbin/resolvconf", &buf) < 0)
	return;
    if (asprintf(&arg, "/sbin/resolvconf -d %s", intf->device) < 0) {
	syslog(LOG_ERR, "failed to release resolvconf: %s", strerror(errno));
	return;
    }

    if (system(arg) != 0)
	syslog(LOG_ERR, "resolvconf -d %s failed", intf->device);
    free(arg);
}

static void callIfupPost(struct pumpNetIntf* intf) {
#ifdef debian
    /* can/should we call a debian one? */
    return;
#else
    pid_t child;
    char * argv[3];
    char arg[64];

    argv[0] = "/etc/sysconfig/network-scripts/ifup-post";
    snprintf(arg,64,"ifcfg-%s",intf->device);
    argv[1] = arg;
    argv[2] = NULL;

    if (!(child = fork())) {
	/* send the script to init */
	if (fork()) _exit(0);

	execvp(argv[0], argv);

	syslog(LOG_ERR,"failed to run %s: %s", argv[0], strerror(errno));

	_exit(0);
    }

    waitpid(child, NULL, 0);
#endif
}

static void callScript(char* script,int msg,struct pumpNetIntf* intf) {
    pid_t child;
    char * argv[20];
    char ** nextArg;
    char * class, * chptr;

    if (!*script) return;

    argv[0] = script;
    argv[2] = intf->device;
    nextArg = argv + 3;

    switch (msg) {
	default:
#ifdef DEBUG
		abort();
#endif
	case PUMP_SCRIPT_NEWLEASE:
	    class = "up";
	    chptr = inet_ntoa(intf->ip);
	    *nextArg = alloca(strlen(chptr) + 1);
	    strcpy(*nextArg, chptr);
	    nextArg++;
	    break;

	case PUMP_SCRIPT_RENEWAL:
	    class = "renewal";
	    chptr = inet_ntoa(intf->ip);
	    *nextArg = alloca(strlen(chptr) + 1);
	    strcpy(*nextArg, chptr);
	    nextArg++;
	    break;

	case PUMP_SCRIPT_DOWN:
	    class = "down";
	    break;
    }

    argv[1] = class;
    *nextArg = NULL;

    if (!(child = fork())) {
	/* send the script to init */
	if (fork()) _exit(0);

	execvp(argv[0], argv);

	syslog(LOG_ERR,"failed to run %s: %s", argv[0], strerror(errno));

	_exit(0);
    }

    waitpid(child, NULL, 0);
}


static void gotNewLease(struct pumpNetIntf *intf) {
    struct pumpOverrideInfo *o = &intf->override;

    pumpSetupInterface(intf);

    syslog(LOG_INFO, "configured interface %s", intf->device);

    if (!(o->flags & OVERRIDE_FLAG_NOGATEWAY)) {
	int i;

	for (i = intf->numGateways - 1; i >= 0; i--)
	    pumpSetupDefaultGateway(&intf->gateways[i]);
    }

    setupDns(intf, o);
    setupDomain(intf, o);

    callScript(o->script, PUMP_SCRIPT_NEWLEASE, intf);
}

static void killLease(struct pumpNetIntf *intf) {
    struct pumpOverrideInfo *o = &intf->override;

    unsetupDns(intf, o);
    callScript(o->script, PUMP_SCRIPT_DOWN, intf);
}

static void runDaemon(int sock, int sock_in) {
    int conn;
    struct sockaddr_un addr;
    int addrLength = sizeof(struct sockaddr_un);
    struct command cmd;
    struct pumpNetIntf intf[20];
    const int maxIntf = sizeof(intf) / sizeof(intf[0]);
    int numInterfaces = 0;
    int i;
    int closest;
    struct timeval tv;
    fd_set fds;

    while (1) {
	FD_ZERO(&fds);
	FD_SET(sock, &fds);
	FD_SET(sock_in, &fds);

	tv.tv_sec = tv.tv_usec = 0;
	closest = -1;
	if (numInterfaces) {
	    for (i = 0; i < numInterfaces; i++) {
		if (!(intf[i].set &
		   (PUMP_INTFINFO_NEEDS_NEWLEASE | PUMP_INTFINFO_HAS_LEASE)))
		    continue;
		/* if this interface has an expired lease due to
		 * renewal failures and it's time to try again to
		 * get a new lease, then try again
		 *
		 * note: this trys every 30 secs FOREVER; this may
		 * or may not be desirable.  could also have a back-off
		 * hueristic that increases the retry delay after each
		 * failed attempt and a maximum number of tries or
		 * maximum period of time to try for.
		 */
		if ((intf[i].set & PUMP_INTFINFO_NEEDS_NEWLEASE) &&
		   (intf[i].renewAt < pumpUptime())) {
		    if (pumpDhcpRun(intf[i].device, 0, 
			  intf[i].reqLease,
			  intf[i].set & PUMP_NETINFO_HAS_HOSTNAME
			    ? intf[i].hostname : NULL,
			  intf + i, &intf[i].override)) {

			    /* failed to get a new lease, so try
			     * again in 30 seconds
                             */
			    intf[i].renewAt = pumpUptime() + 30;

		    } else {
			intf[i].set &= ~PUMP_INTFINFO_NEEDS_NEWLEASE;
			gotNewLease(intf + i);
                    }
		}
		if (closest == -1 || (intf[closest].renewAt > intf[i].renewAt))
		    closest = i;
	    }
	    if (closest != -1) {
		tv.tv_sec = intf[closest].renewAt - pumpUptime();
		if (tv.tv_sec <= 0) {
		    if (pumpDhcpRenew(intf + closest)) {
			syslog(LOG_INFO,
				"failed to renew lease for device %s",
				intf[closest].device);

			/* if the renewal failed, then set renewAt to
			 * try again in 30 seconds AND then if renewAt's
			 * value is after the lease expiration then
			 * try to get a fresh lease for the interface
			 */
			if ((intf[closest].renewAt = pumpUptime() + 30) >
			    intf[closest].leaseExpiration) {
			    intf[closest].set &= ~PUMP_INTFINFO_HAS_LEASE;
			    intf[closest].set |= PUMP_INTFINFO_NEEDS_NEWLEASE;


			    if (pumpDhcpRun(intf[closest].device, 
				  intf[closest].flags, 
				  intf[closest].reqLease,
				  intf[closest].set & PUMP_NETINFO_HAS_HOSTNAME
				    ? intf[closest].hostname : NULL,
				  intf + closest, &intf[closest].override)) {
 
 				/* failed to get a new lease, so try
				 * again in 30 seconds
				 */
				intf[closest].renewAt = pumpUptime() + 30;
				killLease(intf + closest);
			    } else {
				killLease(intf + closest);
				intf[closest].set &=
					~PUMP_INTFINFO_NEEDS_NEWLEASE;
				gotNewLease(intf + closest);
                            }
			}
		    } else {
			callScript(intf[closest].override.script,
				   PUMP_SCRIPT_RENEWAL, &intf[closest]);
			callIfupPost(&intf[closest]);
		    }

		    continue;	    /* recheck timeouts */
		}
	    }
	}

	if (select(sock + 1, &fds, NULL, NULL, 
		   closest != -1 ? &tv : NULL) > 0) {
	    if (!FD_ISSET(sock, &fds)) {
		char c = 0;
		struct sockaddr_in addr_in;
		socklen_t len;
		struct stat buf;

		if (!FD_ISSET(sock_in, &fds))
		    continue;

		conn = accept(sock_in, (struct sockaddr *) &addr_in, &len);

		if (!stat(CONTROLSOCKET, &buf))
		    goto out;

		close(sock);

		addr.sun_family = AF_UNIX;
		strcpy(addr.sun_path, CONTROLSOCKET);
		addrLength = sizeof(addr.sun_family) + strlen(addr.sun_path);

		if ((sock = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
		    syslog(LOG_ERR, "failed to create socket: %s\n",
			   strerror(errno));
		    exit(1);
		}

		umask(077);
		if (bind(sock, (struct sockaddr *) &addr, addrLength)) {
		    syslog(LOG_ERR, "bind to %s failed: %s\n", CONTROLSOCKET,
			   strerror(errno));
		    exit(1);
		}
		umask(033);

		listen(sock, 5);

		write(conn, &c, 1);

out:
		close(conn);
		continue;
	    }
	    conn = accept(sock, (struct sockaddr *) &addr, &addrLength);

	    if (read(conn, &cmd, sizeof(cmd)) != sizeof(cmd)) {
		close(conn);
		continue;
	    }

	    switch (cmd.type) {
	      case CMD_DIE:
		for (i = 0; i < numInterfaces; i++) {
		    pumpDhcpRelease(intf + i);
		    killLease(intf + i);
		}

		syslog(LOG_INFO, "terminating at root's request");

		cmd.type = CMD_RESULT;
		cmd.u.result = 0;
		write(conn, &cmd, sizeof(cmd));
		exit(0);

	      case CMD_STARTIFACE:
		if (numInterfaces >= maxIntf) {
		    syslog(LOG_INFO, "too many interfaces");
		    cmd.u.result = 1;
		    break;
		}

		if (pumpDhcpRun(cmd.u.start.device,
			        cmd.u.start.flags, cmd.u.start.reqLease, 
			        cmd.u.start.reqHostname[0] ? 
			            cmd.u.start.reqHostname : NULL,
			        intf + numInterfaces, &cmd.u.start.override)) {
		    cmd.u.result = 1;
		} else {
		    gotNewLease(intf + numInterfaces);
		    cmd.u.result = 0;
		    numInterfaces++;
		}
		break;

	      case CMD_FORCERENEW:
		for (i = 0; i < numInterfaces; i++)
		    if (!strcmp(intf[i].device, cmd.u.renew.device)) break;
		if (i == numInterfaces)
		    cmd.u.result = RESULT_UNKNOWNIFACE;
		else {
		    cmd.u.result = pumpDhcpRenew(intf + i);
		    if (!cmd.u.result) {
			callScript(intf[i].override.script,
				   PUMP_SCRIPT_RENEWAL, intf + i);
			callIfupPost(intf + i);
		    }
		}
		break;

	      case CMD_STOPIFACE:
		for (i = 0; i < numInterfaces; i++)
		    if (!strcmp(intf[i].device, cmd.u.stop.device)) break;
		if (i == numInterfaces)
		    cmd.u.result = RESULT_UNKNOWNIFACE;
		else {
		    cmd.u.result = pumpDhcpRelease(intf + i);
		    killLease(intf + i);
		    if (numInterfaces == 1) {
			cmd.type = CMD_RESULT;
			write(conn, &cmd, sizeof(cmd));

			syslog(LOG_INFO, "terminating as there are no "
				"more devices under management");

			exit(0);
		    }

		    intf[i] = intf[numInterfaces - 1];
		    numInterfaces--;
		}
		break;

	      case CMD_REQSTATUS:
		for (i = 0; i < numInterfaces; i++)
		    if (!strcmp(intf[i].device, cmd.u.stop.device)) break;
		if (i == numInterfaces) {
		    cmd.u.result = RESULT_UNKNOWNIFACE;
		} else {
		    cmd.type = CMD_STATUS;
		    cmd.u.status.intf = intf[i];
		    if (intf[i].set & PUMP_NETINFO_HAS_HOSTNAME)
			strncpy(cmd.u.status.hostname,
			    intf->hostname, sizeof(cmd.u.status.hostname));
		    cmd.u.status.hostname[sizeof(cmd.u.status.hostname)] = '\0';

		    if (intf[i].set & PUMP_NETINFO_HAS_DOMAIN)
			strncpy(cmd.u.status.domain,
			    intf->domain, sizeof(cmd.u.status.domain));
		    cmd.u.status.domain[sizeof(cmd.u.status.domain) - 1] = '\0';

		    if (intf[i].set & PUMP_INTFINFO_HAS_BOOTFILE)
			strncpy(cmd.u.status.bootFile,
			    intf->bootFile, sizeof(cmd.u.status.bootFile));
		    cmd.u.status.bootFile[sizeof(cmd.u.status.bootFile) - 1] = 
		    							'\0';
		    if (intf[i].set & PUMP_NETINFO_HAS_NISDOMAIN)
			strncpy(cmd.u.status.nisDomain,
			    intf->nisDomain, sizeof(cmd.u.status.nisDomain));
		    cmd.u.status.nisDomain[sizeof(cmd.u.status.nisDomain)-1] = 
		    							'\0';
		}

	      case CMD_STATUS:
	      case CMD_RESULT:
		/* can't happen */
		break;
	    }

	    if (cmd.type != CMD_STATUS) cmd.type = CMD_RESULT;
	    write(conn, &cmd, sizeof(cmd));

	    close(conn);
	}
    }

    exit(0);
}

static int openControlSocket(void) {
    struct sockaddr_un addr;
    struct sockaddr_in addr_in;
    int sock;
    int sock_in;
    size_t addrLength;
    pid_t child;
    int status;
    int error;
    struct timeval timeout;

    if ((sock = socket(PF_UNIX, SOCK_STREAM, 0)) < 0)
	return -1;

    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, CONTROLSOCKET);
    addrLength = sizeof(addr.sun_family) + strlen(addr.sun_path);

    if (!connect(sock, (struct sockaddr *) &addr, addrLength)) 
	return sock;

    error = errno;
    if (error != ENOENT && error != ECONNREFUSED) {
	fprintf(stderr, "failed to connect to %s: %s\n", CONTROLSOCKET,
		strerror(error));
	goto err;
    }

    unlink(CONTROLSOCKET);

    if ((sock_in = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
	goto err;
    }

    addr_in.sin_family = AF_INET;
    addr_in.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr_in.sin_port = bootp_client_port;

    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    setsockopt(sock_in, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
    if (!connect(sock_in, (struct sockaddr *) &addr_in, sizeof(addr_in))) {
	char c;

	read(sock_in, &c, 1);
	close(sock_in);
	goto again;
    }

    error = errno;
    close(sock_in);
    if (error != ECONNREFUSED && error != ETIMEDOUT) {
	fprintf(stderr, "failed to connect to localhost:bootpc: %s\n",
		strerror(error));
	fprintf(stderr, "There might be another pump running!\n");
    }

    addr_in.sin_addr.s_addr = htonl(INADDR_ANY);

    if (!(child = fork())) {
	close(sock);

	close(0);
	close(1);
	close(2);

	openlog("pumpd", LOG_PID, LOG_DAEMON);

	if ((sock_in = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
	    syslog(LOG_ERR, "failed to create IP socket: %s\n",
		   strerror(errno));
	    exit(1);
	}

	if (bind(sock_in, (struct sockaddr *) &addr_in, sizeof(addr_in))) {
	    syslog(LOG_ERR, "bind to bootpc/tcp failed: %s\n",
		   strerror(errno));
	    exit(1);
	}

	listen(sock_in, 5);

	if ((sock = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
	    syslog(LOG_ERR, "failed to create socket: %s\n", strerror(errno));
	    exit(1);
	}

	chdir("/");
	umask(077);
	if (bind(sock, (struct sockaddr *) &addr, addrLength)) {
	    syslog(LOG_ERR, "bind to %s failed: %s\n", CONTROLSOCKET,
		    strerror(errno));
	    exit(1);
	}
	umask(033);

	listen(sock, 5);

	if (fork()) _exit(0);

	setsid();

	{
	    time_t now,upt;
	    int updays,uphours,upmins,upsecs;

	    now = time(NULL);
	    upt = pumpUptime();
	    if (now <= upt)
		syslog(LOG_INFO, "starting at %s\n", ctime(&now));
	    else {
		upsecs = upt % 60;
		upmins = (upt / 60) % 60;
		uphours = (upt / 3600) % 24;
		updays = upt / 86400;
		syslog(LOG_INFO, "starting at (uptime %d days, %d:%02d:%02d) %s\n", updays, uphours, upmins, upsecs, ctime(&now));
	    }
	}

	runDaemon(sock, sock_in);
    }

    close(sock_in);

    waitpid(child, &status, 0);
    if (!WIFEXITED(status) || WEXITSTATUS(status))
	return -1;

again:
    if (!connect(sock, (struct sockaddr *) &addr, addrLength)) 
	return sock;

    fprintf(stderr, "failed to connect to localhost:bootpc: %s\n",
	    strerror(errno));

err:
    close(sock);
    return -1;
}

void printStatus(struct pumpNetIntf i, char * hostname, char * domain,
		 char * bootFile, char * nisDomain) {
    int j;
    time_t now,upnow,localAt,localExpiration;

    printf("Device %s\n", i.device);
    printf("\tIP: %s\n", inet_ntoa(i.ip));
    printf("\tNetmask: %s\n", inet_ntoa(i.netmask));
    printf("\tBroadcast: %s\n", inet_ntoa(i.broadcast));
    printf("\tNetwork: %s\n", inet_ntoa(i.network));
    printf("\tBoot server %s\n", inet_ntoa(i.bootServer));
    printf("\tNext server %s\n", inet_ntoa(i.nextServer));

    if (i.numGateways) {
	printf("\tGateway: %s\n", inet_ntoa(i.gateways[0]));
	printf("\tGateways:");
	for (j = 0; j < i.numGateways; j++)
	    printf(" %s", inet_ntoa(i.gateways[j]));
	printf("\n");
    }

    if (i.set & PUMP_INTFINFO_HAS_BOOTFILE)
	printf("\tBoot file: %s\n", bootFile);

    if (i.set & PUMP_NETINFO_HAS_HOSTNAME)
	printf("\tHostname: %s\n", hostname);

    if (i.set & PUMP_NETINFO_HAS_DOMAIN)
	printf("\tDomain: %s\n", domain);

    if (i.numDns) {
	printf("\tNameservers:");
	for (j = 0; j < i.numDns; j++)
	    printf(" %s", inet_ntoa(i.dnsServers[j]));
	printf("\n");
    }

    if (i.set & PUMP_NETINFO_HAS_NISDOMAIN)
	printf("\tNIS Domain: %s\n", nisDomain);

    if (i.numLog) {
	printf("\tLogservers:");
	for (j = 0; j < i.numLog; j++)
	    printf(" %s", inet_ntoa(i.logServers[j]));
	printf("\n");
    }
 
    if (i.numLpr) {
	printf("\tLprservers:");
	for (j = 0; j < i.numLpr; j++)
	    printf(" %s", inet_ntoa(i.lprServers[j]));
	printf("\n");
    }

    if (i.numNtp) {
	printf("\tNtpservers:");
	for (j = 0; j < i.numNtp; j++)
	    printf(" %s", inet_ntoa(i.ntpServers[j]));
	printf("\n");
    }

    if (i.numXfs) {
	printf("\tXfontservers:");
	for (j = 0; j < i.numXfs; j++)
	    printf(" %s", inet_ntoa(i.xfntServers[j]));
	printf("\n");
    }

    if (i.numXdm) {
	printf("\tXdmservers:");
	for (j = 0; j < i.numXdm; j++)
	    printf(" %s", inet_ntoa(i.xdmServers[j]));
	printf("\n");
    }

    if (i.set & PUMP_INTFINFO_HAS_LEASE) {
	upnow = pumpUptime();
	tzset();
	now = time(NULL);
	localAt = now + (i.renewAt - upnow);
	localExpiration = now + (i.leaseExpiration - upnow);
	printf("\tRenewal time: %s", ctime(&localAt)); 
	printf("\tExpiration time: %s", ctime(&localExpiration)); 
    }
}

int main (int argc, const char ** argv) {
    char * device = "eth0";
    char * hostname = "";
    poptContext optCon;
    int rc;
    int test = 0;
    int flags = 0;
    int lease_hrs = 0;
    int lease = 12*3600;
    int killDaemon = 0;
    int winId = 0;
    int release = 0, renew = 0, status = 0, lookupHostname = 0, nodns = 0;
    int nogateway = 0, nobootp = 0;
    int nosetup = 0;
    int noresolvconf = 0;
    struct command cmd, response;
    char * configFile = "/etc/pump.conf";
    char * script = NULL;
    struct pumpOverrideInfo * overrides;
    int cont;
    struct poptOption options[] = {
	    { "config-file", 'c', POPT_ARG_STRING, &configFile, 0,
			N_("Configuration file to use instead of "
			   "/etc/pump.conf") },
            { "hostname", 'h', POPT_ARG_STRING, &hostname, 0, 
			N_("Hostname to request"), N_("hostname") },
            { "interface", 'i', POPT_ARG_STRING, &device, 0, 
			N_("Interface to configure (normally eth0)"), 
			N_("iface") },
	    { "kill", 'k', POPT_ARG_NONE, &killDaemon, 0,
			N_("Kill daemon (and disable all interfaces)"), NULL },
	    { "lease", 'l', POPT_ARG_INT, &lease_hrs, 0,
			N_("Lease time to request (in hours)"), N_("hours") },
	    { "leasesecs", 'L', POPT_ARG_INT, &lease, 0,
			N_("Lease time to request (in seconds)"), N_("seconds") },
	    { "lookup-hostname", '\0', POPT_ARG_NONE, &lookupHostname, 0,
			N_("Force lookup of hostname") },
	    { "release", 'r', POPT_ARG_NONE, &release, 0,
			N_("Release interface"), NULL },
	    { "renew", 'R', POPT_ARG_NONE, &renew, 0,
			N_("Force immediate lease renewal"), NULL },
            { "verbose", 'v', POPT_ARG_NONE, &verbose, 0,
                        N_("Log verbose debug info"), NULL },
	    { "status", 's', POPT_ARG_NONE, &status, 0,
			N_("Display interface status"), NULL },
	    { "no-dns", 'd', POPT_ARG_NONE, &nodns, 0,
			N_("Don't update resolv.conf"), NULL },
	    { "no-gateway", '\0', POPT_ARG_NONE, &nogateway, 0,
			N_("Don't set a gateway for this interface"), NULL },
	    { "no-setup", '\0', POPT_ARG_NONE, &nosetup, 0,
			N_("Don't set up anything"), NULL },
	    { "no-resolvconf", '\0', POPT_ARG_NONE, &noresolvconf, 0,
			N_("Don't set up resolvconf"), NULL },
	    { "no-bootp", '\0', POPT_ARG_NONE, &nobootp, 0,
	                N_("Ignore non-DHCP BOOTP responses"), NULL },
	    { "script", '\0', POPT_ARG_STRING, &script, 0,
			N_("Script to use") },
	    { "win-client-ident", '\0', POPT_ARG_NONE, &winId, 0,
			N_("Set the client identifier to match Window's") },
	    /*{ "test", 't', POPT_ARG_NONE, &test, 0,
			N_("Don't change the interface configuration or "
			   "run as a deamon.") },*/
	    POPT_AUTOHELP
	    { NULL, '\0', 0, NULL, 0 }
        };
#if !UDEB
    struct servent *servent;

    servent = getservbyname("bootpc", "udp");
    if (!servent) {
	perror("Cannot resolve bootpc/udp service");
	return 1;
    }
    bootp_client_port = servent->s_port;

    servent = getservbyname("bootps", "udp");
    if (!servent) {
	perror("Cannot resolve bootps/udp service");
	return 1;
    }
    bootp_server_port = servent->s_port;
#endif

    memset(&cmd, 0, sizeof(cmd));
    memset(&response, 0, sizeof(response));

    optCon = poptGetContext(PROGNAME, argc, argv, options,0);
    poptReadDefaultConfig(optCon, 1);

    if ((rc = poptGetNextOpt(optCon)) < -1) {
	fprintf(stderr, _("%s: bad argument %s: %s\n"), PROGNAME,
		poptBadOption(optCon, POPT_BADOPTION_NOALIAS), 
		poptStrerror(rc));
	return 1;
    }

    if (poptGetArg(optCon)) {
	fprintf(stderr, _("%s: no extra parameters are expected\n"), PROGNAME);
	return 1;
    }

    if (script && strlen(script) > sizeof(overrides->script)) {
	fprintf(stderr, _("%s: --script argument is too long\n"), PROGNAME);
	return 1;
    }

    /* make sure the config file is parseable before going on any further */
    if (readPumpConfig(configFile, &overrides)) return 1;

    if (geteuid()) {
	fprintf(stderr, _("%s: must be run as root\n"), PROGNAME);
	exit(1);
    }

    if (test)
	flags = PUMP_FLAG_NODAEMON | PUMP_FLAG_NOCONFIG;
    if (winId)
	flags |= PUMP_FLAG_WINCLIENTID;
    if (lookupHostname)
	flags |= PUMP_FLAG_FORCEHNLOOKUP;

    if (killDaemon) {
	cmd.type = CMD_DIE;
    } else if (status) {
	cmd.type = CMD_REQSTATUS;
	strcpy(cmd.u.reqstatus.device, device);
    } else if (renew) {
	cmd.type = CMD_FORCERENEW;
	strcpy(cmd.u.renew.device, device);
    } else if (release) {
	cmd.type = CMD_STOPIFACE;
	strcpy(cmd.u.stop.device, device);
    } else {
	struct pumpOverrideInfo * o;

	cmd.type = CMD_STARTIFACE;
	strcpy(cmd.u.start.device, device);
	cmd.u.start.flags = flags;
	if(lease_hrs)
		cmd.u.start.reqLease = lease_hrs * 60 * 60;
	else
		cmd.u.start.reqLease = lease;
	strcpy(cmd.u.start.reqHostname, hostname);

	o = overrides + 1;
	while (*o->device && strcmp(o->device, device))
	    o++;
	if (!*o->device)
	    o = overrides;

	if (nodns)
	    o->flags |= OVERRIDE_FLAG_NODNS;
	if (nobootp)
	    o->flags |= OVERRIDE_FLAG_NOBOOTP;
	if (nogateway)
	    o->flags |= OVERRIDE_FLAG_NOGATEWAY;
	if (nosetup)
	    o->flags |=
		OVERRIDE_FLAG_NOSETUP |
		OVERRIDE_FLAG_NODNS |
		OVERRIDE_FLAG_NOGATEWAY |
		OVERRIDE_FLAG_NONISDOMAIN;
	if (noresolvconf)
	    o->flags |= OVERRIDE_FLAG_NORESOLVCONF;
	if (script)
	    strcpy(o->script, script);

	memcpy(&cmd.u.start.override, o, sizeof(*o));
    }

    free(overrides);

again:
    cont = openControlSocket();
    if (cont < 0) 
	exit(1);

    if (write(cont, &cmd, sizeof(cmd)) < 0) {
retry:
	close(cont);
	goto again;
    }
    if (read(cont, &response, sizeof(response)) <= 0)
	goto retry;

    if (response.type == CMD_RESULT) {
	if (response.u.result) {
	    fprintf(stderr, "Operation failed.\n");
	    return 1;
	}
    } else if (response.type == CMD_STATUS) {
	printStatus(response.u.status.intf, response.u.status.hostname, 
		    response.u.status.domain, response.u.status.bootFile,
		    response.u.status.nisDomain);
    }

    return 0;
}
