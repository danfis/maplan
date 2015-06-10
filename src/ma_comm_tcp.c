#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <boruvka/alloc.h>
#include "plan/ma_comm.h"

#define ESTABLISH_TIMEOUT 10000
#define SHUTDOWN_TIMEOUT 10000

struct _plan_ma_comm_tcp_t {
    plan_ma_comm_t comm;

    int recv_main_sock; /*!< Main listening socket */
    int *sock;          /*!< In/Out sockets */

    struct pollfd *poll_fds; /*!< Pre-filled array for poll(2) */
};
typedef struct _plan_ma_comm_tcp_t plan_ma_comm_tcp_t;

#define TCP(comm) bor_container_of(comm, plan_ma_comm_tcp_t, comm)

static void tcpDel(plan_ma_comm_t *comm);
static int tcpSendToNode(plan_ma_comm_t *comm, int node_id,
                         const plan_ma_msg_t *msg);
static plan_ma_msg_t *tcpRecv(plan_ma_comm_t *comm);
static plan_ma_msg_t *tcpRecvBlock(plan_ma_comm_t *comm, int timeout_in_ms);

static int parseAddr(const char *addr, char *addr_out)
{
    const char *a;
    int i;

    for (i = 0, a = addr; *a != 0x0 && *a != ':'; ++a)
        addr_out[i++] = *a;
    addr_out[i] = 0;
    if (*a == ':')
        return atoi(a + 1);
    return -1;
}

static int initSockAddr(struct sockaddr_in *a, int port, const char *addr)
{
    int ret;

    bzero(a, sizeof(*a));
    ret = inet_pton(AF_INET, addr, &a->sin_addr);
    if (ret == 0){
        fprintf(stderr, "Error TCP: Invalid address: `%s'\n", addr);
        return -1;
    }else if (ret < 0){
        fprintf(stderr, "Error TCP: Could not convert address `%s': %s\n",
                addr, strerror(errno));
        return -1;
    }
    a->sin_family = AF_INET;
    a->sin_port = htons(port);
    return 0;
}

static void sockSetLinger(int sock, int time_in_s)
{
    struct linger lin;

    lin.l_onoff = 1;
    lin.l_linger = time_in_s;
    if (setsockopt(sock, SOL_SOCKET, SO_LINGER, &lin, sizeof(lin)) != 0){
        fprintf(stderr, "Error TCP: Could not set SO_LINGER %s\n",
                strerror(errno));
    }
}

static int recvMainSocket(int agent_id, int agent_size, const char **addr_in)
{
    int sock, port, yes;
    char addr[30];
    struct sockaddr_in recv_addr;

    port = parseAddr(addr_in[agent_id], addr);
    if (port < 0){
        fprintf(stderr, "Error TCP: Could not parse address:port from"
                        " input:`%s'\n", addr_in[agent_id]);
        return -1;
    }

    // Prepare address to bind to
    if (initSockAddr(&recv_addr, port, addr) != 0)
        return -1;

    // Create a socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
        return sock;

    // Set the socket to non-blocking mode
    if (fcntl(sock, F_SETFL, O_NONBLOCK) != 0){
        fprintf(stderr, "Error TCP: Could not set socket non-blocking: %s\n",
                strerror(errno));
        close(sock);
        return -1;
    }

    // Set socket to reuse time-wait address
    yes = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) != 0){
        fprintf(stderr, "Error TCP: Could not set SO_REUSEADDR: %s\n",
                strerror(errno));
        close(sock);
        return -1;
    }

    // Bind socket to the specified address
    if (bind(sock, (struct sockaddr *)&recv_addr, sizeof(recv_addr)) < 0){
        fprintf(stderr, "Error TCP: Could not bind to address `*:%d'\n",
                port);
        close(sock);
        return -1;
    }

    // Listen on this socket and make buffer enough for all agents
    if (listen(sock, agent_size) != 0){
        fprintf(stderr, "Error TCP: listen(2) failed.\n");
        close(sock);
        return -1;
    }

    return sock;
}

static int sendGreetingMsg(plan_ma_comm_tcp_t *tcp, int id)
{
    int ret;
    uint16_t msg;

    // Send the greeting message which is ID of the current agent
    msg = tcp->comm.node_id;
#ifdef BOR_BIG_ENDIAN
    msg = htole16(msg):
#endif /* BOR_BIG_ENDIAN */
    errno = 0;
    ret = send(tcp->sock[id], &msg, 2, 0);
    fprintf(stderr, "send-msg: %d to %d | %d %s\n", (int)msg, id, ret, strerror(errno));

    if (ret != 2){
        fprintf(stderr, "Error TCP: Could not send greeting message (%d): %s\n",
                (int)ret, strerror(errno));
        return -1;
    }

    return 0;
}

static int recvGreetingMsg(int sock)
{
    int ret;
    uint16_t msg;

    msg = 0;
    errno = 0;
    ret = recv(sock, &msg, 2, MSG_WAITALL);
    if (ret != 2){
        fprintf(stderr, "Error TCP: Could not receive greeting message (%d): %s\n",
                (int)ret, strerror(errno));
        return -1;
    }

#ifdef BOR_BIG_ENDIAN
    msg = le16toh(msg):
#endif /* BOR_BIG_ENDIAN */
    fprintf(stderr, "recv-msg: %d\n", (int)msg);

    return msg;
}

/** Creates and connects id'th send socket to the specified remote.
 *  Returns 0 is connection was successful and -1 on error.
 *  If connection is in-progress 1 is returned -- in this case
 *  sendSocketConnectCheck() must be called later to be sure that
 *  connection is ok. */
static int sendSocketConnect(plan_ma_comm_tcp_t *tcp, int id,
                             const char *addr_in)
{
    int sock, port, ret;
    char addr[30];
    struct sockaddr_in send_addr;

    // Check and setup remote port and address
    port = parseAddr(addr_in, addr);
    if (port < 0){
        fprintf(stderr, "Error TCP: Could not parse address:port from"
                        " input:`%s'\n", addr_in);
        return -1;
    }

    if (initSockAddr(&send_addr, port, addr) != 0)
        return -1;

    // Create a new socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0){
        fprintf(stderr, "Error TCP: Could not create send socket: %s\n",
                strerror(errno));
        return -1;
    }
    sockSetLinger(sock, 100000);

    // Set the socket to non-blocking mode
    if (fcntl(sock, F_SETFL, O_NONBLOCK) != 0){
        fprintf(stderr, "Error TCP: Could not set socket non-blocking: %s\n",
                strerror(errno));
        close(sock);
        return -1;
    }

    // Connect to the remote
    ret = connect(sock, (struct sockaddr *)&send_addr, sizeof(send_addr));
    if (ret == 0){
        // The connection was successful
        if (sendGreetingMsg(tcp, id) != 0)
            return -1;
        tcp->sock[id] = sock;
        return 0;

    }else if (errno == EINPROGRESS){
        // In non-blocking mode in-progress is also ok
        tcp->sock[id] = sock;
        return 1;

    }else{
        fprintf(stderr, "Error TCP: Something went wrong during connection to"
                        " `%s': %s\n", addr_in, strerror(errno));
        return -1;
    }
}

/** Returns 0 if connection was established, 1 if the connection should be
 *  retried with sendSocketConnect() or -1 on error.
 *  This function should be called only as result of active poll(). */
static int sendSocketConnectCheck(plan_ma_comm_tcp_t *tcp, int id)
{
    int error;
    socklen_t len;

    len = sizeof(error);
    error = 0;
    if (getsockopt(tcp->sock[id], SOL_SOCKET, SO_ERROR, &error, &len) != 0){
        fprintf(stderr, "Error TCP: Could not get sock error: %s\n",
                strerror(errno));
        return -1;
    }

    if (error == 0){
        if (sendGreetingMsg(tcp, id) != 0)
            return -1;
        return 0;
    }

    fprintf(stderr, "check-error: %d, close\n", error);
    close(tcp->sock[id]);
    tcp->sock[id] = -1;
    if (error == ECONNREFUSED
            || error == EINPROGRESS
            || error == EALREADY)
        return 1;

    fprintf(stderr, "Error TCP: Approached error during connection to: %s\n",
                    strerror(error));
    return -1;
}

static int acceptConnection(plan_ma_comm_tcp_t *tcp)
{
    int sock, id;

    sock = accept(tcp->recv_main_sock, NULL, NULL);
    if (sock < 0){
        fprintf(stderr, "Error TCP: Could not accept connection: %s\n",
                strerror(errno));
        return -1;
    }

    // Set the socket to non-blocking mode
    if (fcntl(sock, F_SETFL, O_NONBLOCK) != 0){
        fprintf(stderr, "Error TCP: Could not set socket non-blocking: %s\n",
                strerror(errno));
        close(sock);
        return -1;
    }

    for (id = tcp->comm.node_id; id < tcp->comm.node_size; ++id){
        if (tcp->sock[id] == -1){
            tcp->sock[id] = sock;
            if (sendGreetingMsg(tcp, id) == -1)
                return -1;
            return 0;
        }
    }
    fprintf(stderr, "Error TCP: All agents are already connected!\n");
    close(sock);
    return -1;
}

static int processGreetingMsgs(plan_ma_comm_tcp_t *tcp)
{
    struct pollfd pfd[tcp->comm.node_size];
    int id, ids[tcp->comm.node_size], tmp[tcp->comm.node_size];
    int i, received = 0, ret;

    fprintf(stderr, "GREET\n");
    for (i = 0; i < tcp->comm.node_size; ++i){
        pfd[i].fd = tcp->sock[i];
        pfd[i].events = POLLIN;
    }

    while (received < tcp->comm.node_size - 1){
        ret = poll(pfd, tcp->comm.node_size, ESTABLISH_TIMEOUT);
        fprintf(stderr, "poll ret: %d\n", ret);
        if (ret == 0){
            fprintf(stderr, "Error TCP: Could not establish agent cluster"
                            " in defined timeout (%d).\n",
                            ESTABLISH_TIMEOUT);
            return -1;

        }else if (ret < 0){
            fprintf(stderr, "Error TCP: Poll error: %s\n",
                    strerror(errno));
            return -1;
        }

        for (i = 0; i < tcp->comm.node_size; ++i){
            if (pfd[i].revents & POLLIN){
                fprintf(stderr, "POLLIN\n");
                pfd[i].fd = -1;
                id = recvGreetingMsg(tcp->sock[i]);
                if (id < 0)
                    return -1;
                ids[i] = id;
                ++received;
            }
        }
    }

    memcpy(tmp, tcp->sock, sizeof(int) * tcp->comm.node_size);
    for (i = 0; i < tcp->comm.node_size; ++i)
        tcp->sock[i] = -1;
    for (i = 0; i < tcp->comm.node_size; ++i){
        if (tmp[i] != -1)
            tcp->sock[ids[i]] = tmp[i];
    }

    for (i = 0; i < tcp->comm.node_size; ++i){
        fprintf(stderr, "sock[%d]: %d\n", i, tcp->sock[i]);
    }

    return 0;
}

static int establishNetwork(plan_ma_comm_tcp_t *tcp, const char **addr)
{
    struct pollfd pfd[tcp->comm.node_size];
    int i, ret, num_connected = 0;

    for (i = 0; i < tcp->comm.node_size; ++i){
        pfd[i].fd = -1;
        pfd[i].events = POLLOUT;
    }
    pfd[tcp->comm.node_id].fd = tcp->recv_main_sock;
    pfd[tcp->comm.node_id].events = POLLIN | POLLPRI;

    while (num_connected != tcp->comm.node_size - 1){
        fprintf(stderr, "C: %d\n", num_connected);

        for (i = 0; i < tcp->comm.node_id; ++i){
            if (tcp->sock[i] == -1){
                ret = sendSocketConnect(tcp, i, addr[i]);
                if (ret == 0){
                    ++num_connected;
                }else if (ret == 1){
                    pfd[i].fd = tcp->sock[i];
                }else if (ret < 0){
                    return -1;
                }
            }
        }

        ret = poll(pfd, tcp->comm.node_size, ESTABLISH_TIMEOUT);
        fprintf(stderr, "poll ret: %d\n", ret);
        if (ret == 0){
            fprintf(stderr, "Error TCP: Could not establish agent cluster"
                            " in defined timeout (%d).\n",
                            ESTABLISH_TIMEOUT);
            return -1;

        }else if (ret < 0){
            fprintf(stderr, "Error TCP: Poll error: %s\n",
                    strerror(errno));
            return -1;
        }

        for (i = 0; i < tcp->comm.node_size; ++i){
            if (pfd[i].revents & POLLIN){
                fprintf(stderr, "POLLIN\n");
                // This can have only recv_main_sock socket
                if (acceptConnection(tcp) != 0)
                    return -1;
                ++num_connected;
            }

            if (pfd[i].revents & POLLOUT){
                fprintf(stderr, "POLLOUT %d\n", i);
                pfd[i].fd = -1;
                ret = sendSocketConnectCheck(tcp, i);
                if (ret == 0){
                    ++num_connected;
                }else if (ret < 0){
                    return -1;
                }
            }
        }

        fprintf(stderr, "C-end: %d\n", num_connected);
    }

    return processGreetingMsgs(tcp);
}

static int shutdownNetwork(plan_ma_comm_tcp_t *tcp)
{
    struct pollfd pfd[tcp->comm.node_size];
    int i, ret, remain;
    char buf[128];

    //sleep(1);
    // First shutdown outgoing connections
    for (i = 0; i < tcp->comm.node_size; ++i){
        if (tcp->sock[i] == -1)
            continue;
        if (shutdown(tcp->sock[i], SHUT_WR) != 0){
            fprintf(stderr, "Error TCP: Could not shutdown socket: %d: %s\n",
                    i, strerror(errno));
        }
    }

    // Then wait for EOFs from all agents
    remain = 0;
    for (i = 0; i < tcp->comm.node_size; ++i){
        pfd[i].fd = tcp->sock[i];
        pfd[i].events = POLLIN;
        if (pfd[i].fd != -1)
            ++remain;
    }

    while (remain > 0){
        ret = poll(pfd, tcp->comm.node_size, SHUTDOWN_TIMEOUT);
        if (ret == 0){
            fprintf(stderr, "Error TCP: Could not shut-down in defined"
                            " timeout (%d).\n", SHUTDOWN_TIMEOUT);
            return -1;

        }else if (ret < 0){
            fprintf(stderr, "Error TCP: Poll error: %s\n",
                    strerror(errno));
            return -1;
        }

        for (i = 0; i < tcp->comm.node_size; ++i){
            if (pfd[i].revents & POLLIN){
                fprintf(stderr, "shutdown-POLLIN\n");
                ret = read(tcp->sock[i], buf, 128);
                fprintf(stderr, "shut-ret: %d\n", ret);
                if (ret < 0){
                    fprintf(stderr, "ERROR: %d %s\n", ret, strerror(errno));
                    return -1;

                }else if (ret == 0){
                    pfd[i].fd = -1;
                    --remain;
                }
            }
        }
    }

    // Close all sockets
    for (i = 0; i < tcp->comm.node_size; ++i){
        if (tcp->sock[i] == -1)
            continue;
        if (close(tcp->sock[i]) != 0){
            fprintf(stderr, "Error TCP: Could not close socket: %d: %s\n",
                    i, strerror(errno));
        }
    }
    close(tcp->recv_main_sock);

    return 0;
}

plan_ma_comm_t *planMACommTCPNew(int agent_id, int agent_size,
                                 const char **addr)
{
    plan_ma_comm_tcp_t *tcp;
    int i;

    tcp = BOR_ALLOC(plan_ma_comm_tcp_t);
    _planMACommInit(&tcp->comm, agent_id, agent_size,
                    tcpDel, tcpSendToNode, tcpRecv, tcpRecvBlock);
    tcp->recv_main_sock = -1;
    tcp->sock = BOR_ALLOC_ARR(int, agent_size);
    tcp->poll_fds = BOR_ALLOC_ARR(struct pollfd, agent_size);
    for (i = 0; i < agent_size; ++i){
        tcp->sock[i] = -1;
    }

    // Create receiving socket but without accepting incomming connections
    tcp->recv_main_sock = recvMainSocket(agent_id, agent_size, addr);
    if (tcp->recv_main_sock < 0){
        tcpDel(&tcp->comm);
        return NULL;
    }

    // Make sure that all remotes are connected
    if (establishNetwork(tcp, addr) != 0){
        tcpDel(&tcp->comm);
        return NULL;
    }

    // Prepare poll descriptors
    /*
    for (i = 0; i < agent_size; ++i){
        if (i == agent_id){
            tcp->poll_fds[i].fd = tcp->recv_main_sock;
            tcp->poll_fds[i].events = POLLIN | POLLPRI;
        }else{
            tcp->poll_fds[i].fd = tcp->recv_sock[i];
            tcp->poll_fds[i].events = POLLIN | POLLPRI;
        }
    }
    */

    /*
    fprintf(stderr, "END\n");
    fflush(stderr);
    if (tcp->comm.node_id > 0)
        write(tcp->sock[tcp->comm.node_id - 1], &i, 4);
    //sleep(10);
    tcpDel(&tcp->comm);
    fprintf(stderr, "Deleted\n");
    exit(-1);
    */
    return &tcp->comm;
}

static void tcpDel(plan_ma_comm_t *comm)
{
    plan_ma_comm_tcp_t *tcp = TCP(comm);

    shutdownNetwork(tcp);

    if (tcp->sock != NULL)
        BOR_FREE(tcp->sock);
    if (tcp->poll_fds)
        BOR_FREE(tcp->poll_fds);
    BOR_FREE(tcp);
}

static int tcpSendToNode(plan_ma_comm_t *comm, int node_id,
                         const plan_ma_msg_t *msg)
{
    plan_ma_comm_tcp_t *tcp = TCP(comm);
    void *buf, *buf2;
    size_t size;
    ssize_t send_ret;
    int ret = 0;

    if (node_id == tcp->comm.node_id)
        return -1;

    // TODO
    buf = planMAMsgPacked(msg, &size);

    buf2 = BOR_ALLOC_ARR(char, size + 4);
#ifdef BOR_BIG_ENDIAN
    *(uint32_t *)buf2 = htole32(size):
#else
    *(uint32_t *)buf2 = (uint32_t)size;
#endif /* BOR_BIG_ENDIAN */
    memcpy((char *)buf2 + 4, buf, size);

    send_ret = send(tcp->sock[node_id], buf2, size + 4, 0);
    if (send_ret < 0){
        fprintf(stderr, "Error TCP[%d]: Could not send message to %d: %s\n",
                tcp->comm.node_id, node_id, strerror(errno));
        ret = -1;
    }else if (send_ret != size + 4){
        fprintf(stderr, "Error TCP[%d]: Could not send WHOLE message to %d"
                        " (%d, %d).\n", tcp->comm.node_id, node_id,
                        (int)(size + 4), (int)send_ret);
        ret = -1;
    }
    fprintf(stderr, "Send to %d, type: %d\n", node_id, planMAMsgType(msg));

    if (buf)
        BOR_FREE(buf);
    if (buf2)
        BOR_FREE(buf2);
    return ret;
}

static plan_ma_msg_t *_tcpRecv(plan_ma_comm_tcp_t *tcp, int timeout_in_ms)
{
    struct pollfd pfd[tcp->comm.node_size];
    plan_ma_msg_t *msg = NULL;
    int i, ret;
    ssize_t rsize;
    uint32_t bufsize;
    char buf[16000];

    for (i = 0; i < tcp->comm.node_size; ++i){
        pfd[i].fd = tcp->sock[i];
        pfd[i].events = POLLIN;
    }

    while (1) {
        fprintf(stderr, "POLL: %d\n", timeout_in_ms);
        ret = poll(pfd, tcp->comm.node_size, timeout_in_ms);
        if (ret == 0){
            // Timeout
            return NULL;

        }else if (ret < 0){
            fprintf(stderr, "Error TCP[%d]: Poll error: %s\n",
                    tcp->comm.node_id, strerror(errno));
            return NULL;
        }

        for (i = 0; i < tcp->comm.node_size; ++i){
            if (pfd[i].revents & POLLIN){
                fprintf(stderr, "recv POLLIN %d\n", i);
                rsize = recv(tcp->sock[i], &bufsize, 4, MSG_WAITALL);
                if (rsize != 4){
                    fprintf(stderr, "Error: rsize != 4 (%d)\n", (int)rsize);
                    exit(-1);
                }

#ifdef BOR_BIG_ENDIAN
                bufsize = le32toh(bufsize);
#endif /* BOR_BIG_ENDIAN */
                rsize = recv(tcp->sock[i], buf, bufsize, MSG_WAITALL);
                if (rsize != bufsize){
                    fprintf(stderr, "Error: rsize != bufsize\n");
                    exit(-1);
                }

                msg = planMAMsgUnpacked(buf, bufsize);
                fprintf(stderr, "RECV from %d, type: %d\n", i, planMAMsgType(msg));
                break;
            }
        }
        break;
    }

    return msg;
}

static plan_ma_msg_t *tcpRecv(plan_ma_comm_t *comm)
{
    plan_ma_comm_tcp_t *tcp = TCP(comm);
    return _tcpRecv(tcp, 0);
}

static plan_ma_msg_t *tcpRecvBlock(plan_ma_comm_t *comm, int timeout_in_ms)
{
    plan_ma_comm_tcp_t *tcp = TCP(comm);
    if (timeout_in_ms <= 0)
        return _tcpRecv(tcp, -1);
    return _tcpRecv(tcp, timeout_in_ms);
}
