#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include <netinet/in.h>


#include "telnetd_port.h"
#include "mid_msgq.h"
#include "mid_mutex.h"
#include "mid_telnet.h"
#include "ind_string.h"

#define ERR_OUT() \
do{                                     \
    printf("error!\n");  \
    goto Err;                           \
} while(0)


extern void *memmem(const void *haystack, size_t haystacklen, const void *needle, size_t needlelen);

#define    IAC          255        /* interpret as command: */
#define    DO           253        /* please, you use option */
#define    WILL         251        /* I will use option */
#define    SB           250        /* interpret as subnegotiation */
#define    SE           240        /* end sub negotiation */

#define TELOPT_ECHO     1        /* echo */
#define    TELOPT_SGA   3        /* suppress go ahead */
#define    TELOPT_NAMS  4        /* approximate message size */

#define    TELPORT      24

#define INPUT_BUF_SIZE      (4*1024)

#define OUTPUT_BUF_SIZE     (32*1024)
#define OUTPUT_ELEM_SIZE    (4*1024)

#define CMD_BUF_SIZE        (4*1024)
#define IAC_MAX_SIZE        16

#define CALL_NUM_MAX        256
#define CALL_NAME_LEN       16

enum {
    TELSTATE_INIT = 0,
    TELSTATE_SYNC,
    TELSTATE_AUTH_USER,
    TELSTATE_AUTH_PASS,
    TELSTATE_SESSION
};

struct td_call {
    struct td_call *next;
    char name[CALL_NAME_LEN];
    int type;
    int argc;
    char argt[TD_ARGC_MAX];
    td_callfunc func;
};

#define HASHSIZE    101

struct telnetd {
    int master_fd;
    int session_fd;

    int CR;

    int state;
    mid_mutex_t mutex;
    mid_msgq_t msgq;
    int msgq_fd;

    char in_buf[INPUT_BUF_SIZE];
    int write_idx;
    int read_idx;

    struct ringbuf* out_rng;

    char cmd_buf[CMD_BUF_SIZE + 4];
    int cmd_len;

    struct td_call call_array[CALL_NUM_MAX];
    int call_num;
    struct td_call* call_hash[HASHSIZE];
};

static unsigned int hash(unsigned char *s)
{
    unsigned int hashval;

    for (hashval = 0; *s != '\0'; s ++)
        hashval = *s + 31 * hashval;
    return hashval % HASHSIZE;
}

static struct telnetd *g_td = NULL;

/*void *ind_memmem(void *haystack, size_t haystacklen,
		     void *needle, size_t needlelen)
{
	char *ph, *pn;
	char *plast;
	size_t n;

	if (needlelen == 0)
		return (void *) haystack;

	if (haystacklen < needlelen)
		return NULL;

	ph = (char *) haystack;
	pn = (char *) needle;
	plast = ph + (haystacklen - needlelen);

	do {
		n = 0;
		while (ph[n] == pn[n]) {
			if (++n == needlelen) {
				return (void *) ph;
			}
		}
	} while (++ph <= plast);

	return NULL;
}*/


static int int_td_active(struct telnetd *td);
static void int_td_suspend(struct telnetd *td);

#define echo(td, buf, len)    rng_buf_enter(td->out_rng, (char *)buf, len)

#define ECHO_OUT(td, buf, len)  \
do{                             \
    echo(td, buf, len);         \
    goto Out;                   \
}while(0)

static u_char iacse[2] = {IAC, SE};
static int iac_remove(struct telnetd *td)
{
    u_char ch;
    int ret, len, length;
    u_char *buf;

    ret = 0;
    buf = (u_char *)(td->in_buf + td->read_idx);
    len = td->write_idx - td->read_idx;

    length = len;

    while (len > 0 && ret == 0) {
        ch = buf[0];
        if (ch != IAC) {
            switch (ch) {
            case 0:
                if (td->CR == 1) {
                    echo(td, "\n", 1);
                    td->cmd_buf[td->cmd_len] = 0;
                    td->cmd_len = 0;
                    ret = 1;
                }
                break;
            case '\b':
            case 0x7f:
                if (td->cmd_len > 0)
                    td->cmd_len --;
                echo(td, "\b \b", 3);
                break;
            case '\r':
                break;
            case '\n':
                echo(td, "\n", 1);
                td->cmd_buf[td->cmd_len] = 0;
                td->cmd_len = 0;
                ret = 1;
                break;
            default:
                td->cmd_buf[td->cmd_len] = ch;
                td->cmd_len ++;
                if (td->state == TELSTATE_AUTH_PASS)
                    echo(td, "*", 1);
                else
                    echo(td, &ch, 1);
                break;
            }
            if (ch == '\r')
                td->CR = 1;
            else
                td->CR = 0;
            buf ++;
            len --;
        } else {
            td->CR = 0;
            if (len < 3)
                break;

            if (buf[1] == SB) {
                int n;
                u_char *p = ind_memmem(buf + 2, len - 2, iacse, 2);
                if (p == NULL)
                    break;
                n = p + 2 - buf;
                buf += n;
                len -= n;
            } else {
                buf += 3;
                len -= 3;
            }
            if (td->state == TELSTATE_SYNC) {
                td->state = TELSTATE_AUTH_USER;
                echo(td, "login:", 6);
            }
        }
    }

    td->read_idx += length - len;
    if (td->read_idx >= (INPUT_BUF_SIZE - IAC_MAX_SIZE)) {
        if (len > 0)
            memcpy(td->in_buf, td->in_buf + td->read_idx, len);
        td->write_idx = len;
        td->read_idx = 0;
    }
    return ret;
}


static void iac_send(struct telnetd *td, u_char command, u_char option)
{
    char buf[3];
    buf[0] = IAC;
    buf[1] = command;
    buf[2] = option;
    echo(td, buf, 3);
}

static void telnetd_auth(struct telnetd *td)
{
    switch(td->state) {
    case TELSTATE_AUTH_USER:
        if (telnetd_port_user_ok(td->cmd_buf)) {
            td->state = TELSTATE_AUTH_PASS;
            echo(td, "password:", 10);
        } else {
            echo(td, "invalid user!\nlogin:", 20);
        }
        break;

    case TELSTATE_AUTH_PASS:
        if (telnetd_port_pass_ok(td->cmd_buf)) {
            echo(td, "\n>", 2);
            td->state = TELSTATE_SESSION;
            //telnetd_port_post_state(eTelnet_state_session);
        } else {
            echo(td, "incorrect password\nlogin:", 25);
            td->state = TELSTATE_AUTH_USER;
        }
        break;
    default:
        break;
    }
}

static void telnetd_exec(struct telnetd *td)
{
    char *p, *argt, *argv[TD_ARGC_MAX];
    int i, ret, l, argc;
    struct td_call *call = NULL;

    if (td->cmd_buf[0] == 0)
        ECHO_OUT(td, ">", 1);

    p = td->cmd_buf;
    while(*p != 0 && !isspace(*p))
        p ++;
    if (*p == 0)
        p = NULL;
    else
        *p = 0;

    l = hash((u_char *)td->cmd_buf);
    for (call = td->call_hash[l]; call; call = call->next) {
        if (strcmp(call->name, td->cmd_buf) == 0)
            break;
    }
    if (call == NULL)
        ECHO_OUT(td, "cmd not exist!\n>", 16);

    l = 0;
    argc = 0;
    while(p) {
        p ++;
        if (isspace(*p) || *p == 0) {
            if (l > 0) {
                argc ++;
                if (argc > call->argc)
                    ECHO_OUT(td, "too many args!\n>", 16);
            }
            if (*p == 0)
                break;
            l = 0;
            *p = 0;
        } else {
            if (argc >= TD_ARGC_MAX)
                ECHO_OUT(td, "too many args!!\n>", 16);
            if (l == 0)
                argv[argc] = p;
            l ++;
        }
    }
    if (argc < call->argc)
        ECHO_OUT(td, "too few args!\n>", 15);

    argt = call->argt;
    for (i = 0; i < argc; i ++) {
        if (argt[i] == 'd') {
            if (strcmp(argv[0], "-1") && atoi(argv[0]) == -1)
                ECHO_OUT(td, "args error!\n>", 13);
        }
    }

    ret = call->func(argc, argv);

    l = sprintf(td->cmd_buf, "return value = %d\n>", ret);
    echo(td, td->cmd_buf, l);

Out:
    return;
}

#define session_make(td, fd)            \
do {                                    \
    td->session_fd = fd;                \
    td->state = TELSTATE_SYNC;          \
    td->CR = 0;                         \
    /*telnetd_port_post_state(eTelnet_state_connect);   */         \
                                        \
    iac_send(td, DO, TELOPT_ECHO);      \
/*    iac_send(td, DO, TELOPT_NAWS);*/  \
    iac_send(td, WILL, TELOPT_ECHO);    \
    iac_send(td, WILL, TELOPT_SGA);     \
} while(0)

#define session_free(td)                \
do {                                    \
    if (td->session_fd >= 0) {          \
        close(td->session_fd);          \
        td->session_fd = -1;            \
    }                                   \
    td->write_idx = 0;                  \
    td->read_idx = 0;                   \
    td->state = TELSTATE_INIT;          \
    /*telnetd_port_post_state(eTelnet_state_idle);*/ \           
} while(0)

#define MAX_FD_SET(fd, set) \
do {                        \
    FD_SET(fd, set);        \
    if (fd > maxfd)         \
        maxfd = fd;         \
} while(0)

static int telnetd_quit(int argc, char **argv)
{
    session_free(g_td);

    return 0;
}

static int telnetd_help(int argc, char **argv)
{
    int i, call_num;
    char *name;
    struct telnetd *td;
    struct td_call *call_array;

    td = g_td;

    call_num = td->call_num;
    call_array = td->call_array;

    for (i = 0; i < call_num; i ++) {
        name = call_array[i].name;
        echo(td, name, strlen(name));
        echo(td, "\t", 1);
    }
    echo(td, ">", 1);

    return 0;
}

static void telnetd_task(void *arg)
{
    int on;
    int count, outlen, insize;
    struct sockaddr_in sa;
    fd_set rset, wset;
    int maxfd;

    struct telnetd *td = (struct telnetd *)arg;

    insize = 0;
    outlen = 0;
    while(1) {
        maxfd = 0;
        FD_ZERO(&rset);
        FD_ZERO(&wset);
        if (-1 != td->master_fd)
            MAX_FD_SET(td->master_fd, &rset);
        MAX_FD_SET(td->msgq_fd, &rset);

        if (td->state != TELSTATE_INIT) {
            if (iac_remove(td) == 1) {
                if (td->state == TELSTATE_SESSION)
                    telnetd_exec(td);
                else
                    telnetd_auth(td);
                continue;
            }

            outlen = rng_buf_len(td->out_rng);
            if (outlen > 0)
                MAX_FD_SET(td->session_fd, &wset);

            if (td->write_idx < INPUT_BUF_SIZE - IAC_MAX_SIZE)
                insize = INPUT_BUF_SIZE - td->write_idx;
            else
                insize = 0;

            if (insize > 0)
                MAX_FD_SET(td->session_fd, &rset);
        }

        if (select(maxfd + 1, &rset, &wset, 0, 0) <= 0)
            continue;
        /* First check for and accept new sessions. */
        if (-1 != td->master_fd && FD_ISSET(td->master_fd, &rset)) {
            int fd;
            #ifdef ST_OS21
            int salen;
            #else
            socklen_t salen;
            #endif
            salen = sizeof(sa);
            fd = accept(td->master_fd, (struct sockaddr *)&sa, &salen);
            if (fd < 0)
                continue;
            on = 1;
            if (setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (caddr_t)&on, sizeof(on))) {
                close(fd);
                printf("setsockopt %d\n", errno);
				ERR_OUT();
            } else {
                if (td->state != TELSTATE_INIT)
                    session_free(td);
                session_make(td, fd);
                printf("session_make\n");
            }
            continue;
        }

        if (FD_ISSET(td->msgq_fd, &rset)) {
            int msgno = 0;
            mid_msgq_getmsg(td->msgq, (char*)&msgno);
            switch (msgno) {
            case 1:
                int_td_active(td);
                break;
            case 2:
                int_td_suspend(td);
                break;
            default:
                break;
            }
            if (td->state != TELSTATE_SESSION)
                rng_buf_reset(td->out_rng);
        }

        if (td->state != TELSTATE_INIT) {

            if (outlen > 0 && FD_ISSET(td->session_fd, &wset)) {
                count = rng_buf_send(td->out_rng, td->session_fd);
                if (count <= 0) {
                    printf("recv count = %d, errno = %d\n", count, errno);
					goto Err;
                }
            }

            if (insize > 0 && FD_ISSET(td->session_fd, &rset)) {
                count = recv(td->session_fd, td->in_buf + td->write_idx, insize, 0);
                if (count <= 0) {
                    printf("send count = %d, errno = %d\n", count, errno);
					goto Err;
                }

                td->write_idx += count;
            }
        }
        continue;
Err:
        session_free(td);
    }
}

static int int_td_active(struct telnetd *td)
{
    int on;
    struct sockaddr_in sa;

    printf("master_fd = %d\n", td->master_fd);
    if (-1 != td->master_fd)
        return 0;

    td->master_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (td->master_fd < 0) {
        printf("socket\n");
		ERR_OUT();
    }

    on = 1;
    setsockopt(td->master_fd, SOL_SOCKET, SO_REUSEADDR, (caddr_t)&on, sizeof(on));

    memset((void *)&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_port = htons(TELPORT);
    sa.sin_addr.s_addr = INADDR_ANY;
    if (bind(td->master_fd, (struct sockaddr *)&sa, sizeof(sa))) {
        printf("bind\n");
		ERR_OUT();
    }
    if (listen(td->master_fd, 5)) {
        printf("listen\n");
		ERR_OUT();
    }
    return 0;
Err:
    int_td_suspend(td);
    return -1;
}

static void int_td_suspend(struct telnetd *td)
{
    if (-1 == td->master_fd)
        return;

    session_free(td);

    close(td->master_fd);
    td->master_fd = -1;
}

int telnetd_init(int active)
{
    struct telnetd *td = NULL;

    printf("SIZE: td_call = %d\n", sizeof(struct td_call));
    //mid_netwk_buildtime( );

    if (g_td) {
        printf("telnetd already init!\n");
		ERR_OUT();
    }
    td = (struct telnetd *)malloc(sizeof(struct telnetd));
    if (td == NULL) {
        printf("malloc\n");
		ERR_OUT();
    }
    memset(td, 0, sizeof(struct telnetd));

    td->master_fd = -1;
    td->session_fd = -1;

    td->out_rng = rng_buf_create(OUTPUT_BUF_SIZE, OUTPUT_ELEM_SIZE);
    if (td->out_rng == NULL) {
        printf("rng_buf_create\n");
		ERR_OUT();
    }

    td->mutex = mid_mutex_create( );
    td->msgq = mid_msgq_create(50, sizeof(int));
    td->msgq_fd = mid_msgq_fd(td->msgq);

    telnetd_port_task_create(telnetd_task, td);

    g_td = td;

    telnetd_regist("quit", telnetd_quit, "");
    telnetd_regist("help", telnetd_help, "");

    if (active)
        telnetd_active( );

    return 0;
Err:
    if (td) {
        if (td->out_rng)
            rng_buf_delete(td->out_rng);
        free(td);
    }
    return -1;
}

int telnetd_regist(const char *callname, td_callfunc callfunc, const char *argstype)
{
    int i, argc;
    struct td_call *call, *c;

    if (g_td == NULL) {
        printf("telnetd not exist!\n");
        return -1;
    }

    mid_mutex_lock(g_td->mutex);
    if (callname == NULL || argstype == NULL || callfunc == NULL) {
        printf("callname =%p, callfunc = %p, argtypes = %p!\n", callname, callfunc, argstype);
		ERR_OUT();
    }
    i = strlen(callname);
    if (i <= 0 || i >= CALL_NAME_LEN) {
        printf("callname len = %d\n", i);
		ERR_OUT();
    }

    if (g_td->call_num >= CALL_NUM_MAX) {
        printf("call_num = %d\n", g_td->call_num);
		ERR_OUT();
    }
    argc = strlen(argstype);
    if (argc > TD_ARGC_MAX) {
        printf("argc = %d\n", argc);
		ERR_OUT();
    }
    for (i = 0; i < argc; i ++) {
        if (argstype[i] != 'd' && argstype[i] != 's') {
            printf("argstype \"%s\" invalid!\n", argstype);
			ERR_OUT();
        }
    }
    call = &g_td->call_array[g_td->call_num];
    call->argc = argc;
    memcpy(call->argt, argstype, argc);
    call->func = callfunc;
    strcpy(call->name, callname);

    i = hash((u_char *)callname);
    for (c = g_td->call_hash[i]; c; c = c->next) {
        if (strcmp(c->name, callname) == 0) {
            printf("%s already exist!\n", callname);
			ERR_OUT();
        }
    }
    call->next = g_td->call_hash[i];
    g_td->call_hash[i] = call;


    g_td->call_num ++;

    mid_mutex_unlock(g_td->mutex);
    return 0;
Err:
    mid_mutex_unlock(g_td->mutex);
    return -1;
}

int telnetd_output(const char *buf, unsigned int len)
{
    char s[64];
    int ret;

    if (g_td == NULL || g_td->state != TELSTATE_SESSION)
        return -1;
    ret = rng_buf_enter(g_td->out_rng, buf, len);
    if (ret <= 0) {
        int l = sprintf(s, "ERROR! telnetd_output: ret = %d / len = %d!\n", ret, len);
        ret = rng_buf_enter(g_td->out_rng, s, l);
        if (ret <= 0)
            return -1;
    }

    {
        int msgno = 0;
        mid_msgq_putmsg(g_td->msgq, (char*)&msgno);
    }

    return 0;
}

void telnetd_active(void)
{
    int msgno;

    printf("\n");
    if (g_td == NULL)
        return;

    msgno = 1;
    mid_msgq_putmsg(g_td->msgq, (char*)&msgno);
}

void telnetd_suspend(void)
{
    int msgno;

    printf("\n");
    if (g_td == NULL)
        return;

    msgno = 2;
    mid_msgq_putmsg(g_td->msgq, (char*)&msgno);
}

