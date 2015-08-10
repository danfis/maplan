#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <boruvka/alloc.h>
#include <boruvka/ring_queue.h>
#include <boruvka/timer.h>
#include "plan/ma_comm.h"

/**
 * Maximal allowed time for network of nodes to be established.
 */
#define ESTABLISH_TIMEOUT 10000 // 10 s

/**
 * Maximal time for shutdown of network.\
 */
#define SHUTDOWN_TIMEOUT 10000 // 10 s

/**
 * Initial buffer size for incoming data
 */
#define BUF_INIT_SIZE 1024

/**
 * Initial size of buffer (queue) of messages.
 */
#define MSG_RING_BUF_SIZE 1024

/**
 * Initial size of the pre-allocated buffer for packed messages.
 */
#define SENDBUF_SIZE 4096


#define ERRM(tcp, text) do { \
    fprintf(stderr, "[%d] Error TCP: " text "\n", (tcp)->comm.node_id); \
    fflush(stderr); \
    } while (0)
#define ERR(tcp, format, ...) do { \
    fprintf(stderr, "[%d] Error TCP: " format "\n", \
            (tcp)->comm.node_id, __VA_ARGS__); \
    fflush(stderr); \
    } while (0)

#define ERRNO(tcp, text) \
    ERR((tcp), text " %s", strerror(errno))

#define ERR2(agent_id, format, ...) do { \
    fprintf(stderr, "[%d] Error TCP: " format "\n", \
            (agent_id), __VA_ARGS__); \
    fflush(stderr); \
    } while (0)

#define ERR2NO(agent_id, text) \
    ERR2((agent_id), text " %s", strerror(errno))

#ifdef BOR_BIG_ENDIAN
# define TO_LE32(x) htole32(x)
# define FROM_LE32(x) htole32(x)
# define TO_LE16(x) htole16(x)
# define FROM_LE16(x) htole16(x)
#else /* BOR_BIG_ENDIAN */
# define TO_LE32(x) (uint32_t)(x)
# define FROM_LE32(x) (uint32_t)(x)
# define TO_LE16(x) (uint16_t)(x)
# define FROM_LE16(x) (uint16_t)(x)
#endif /* BOR_BIG_ENDIAN */

struct _buf_t {
    char *buf;     /*!< Buffer for input data */
    int size;      /*!< Allocated size */
    char *beg;     /*!< Beginning of the filled part of the buffer */
    char *end;     /*!< Beggining of the empty part of the buffer */
    int remain;    /*!< How many bytes remain in buffer (after .end) */
    int read_size; /*!< Number of read bytes starting at .beg */
    int msg_size;  /*!< Pre-read size of the next message */
};
typedef struct _buf_t buf_t;

static void bufInit(buf_t *buf);
static void bufFree(buf_t *buf);
static void bufExtend(buf_t *buf, int size);
static void bufShiftData(buf_t *buf);
/** Tries to fill the remained of the buffer from the given socket.
 *  Returns -1 on error, 0 on EOF and 1 if successful. */
static int bufFillFromSock(buf_t *buf, int sock);
/** Tries to extract next message if there is already enough data in buffer. */
static plan_ma_msg_t *bufExtractMsg(buf_t *buf);

struct _msg_ring_buf_t {
    plan_ma_msg_t **buf; /*!< Ring buffer */
    int size;            /*!< Size of the buffer */
    int head;            /*!< Head pointer */
    int tail;            /*!< Tail pointer */
};
typedef struct _msg_ring_buf_t msg_ring_buf_t;

static void msgRingBufInit(msg_ring_buf_t *b);
static void msgRingBufFree(msg_ring_buf_t *b);
static void msgRingBufExtend(msg_ring_buf_t *b);
static void msgRingBufPush(msg_ring_buf_t *b, plan_ma_msg_t *msg);
static plan_ma_msg_t *msgRingBufPop(msg_ring_buf_t *b);
_bor_inline int msgRingBufNext(int c, int size);
_bor_inline int msgRingBufFull(const msg_ring_buf_t *b);
_bor_inline int msgRingBufEmpty(const msg_ring_buf_t *b);

struct _plan_ma_comm_tcp_t {
    plan_ma_comm_t comm;

    int listen_sock;       /*!< Main listening socket */
    int *send_sock;        /*!< Out sockets */
    int *recv_sock;        /*!< In sockets */
    buf_t *buf;            /*!< Buffer for each recv_sock socket */
    msg_ring_buf_t msgbuf; /*!< Buffer for parsed messages */
    char *sendbuf;         /*!< Preallocated send buffer */
    int sendbuf_size;      /*!< Size of .sendbuf */
    struct pollfd *recv_pfd; /*!< Poll structures for receiving msgs */

    int use_th;                 /*!< True if threaded implementation is used */
    bor_ring_queue_t th_msgbuf; /*!< Message buffer with lock */
    pthread_t th_recv;          /*!< Thread for recvining messages */
    int th_pipe[2];             /*!< Pipe for communication between main
                                     thread and the recv thread */
};
typedef struct _plan_ma_comm_tcp_t plan_ma_comm_tcp_t;
#define TCP(comm) bor_container_of(comm, plan_ma_comm_tcp_t, comm)

/** Comm methods: */
static void tcpDel(plan_ma_comm_t *comm);
static int tcpSendToNode(plan_ma_comm_t *comm, int node_id,
                         const plan_ma_msg_t *msg);
static plan_ma_msg_t *tcpRecv(plan_ma_comm_t *comm);
static plan_ma_msg_t *tcpRecvBlock(plan_ma_comm_t *comm, int timeout_in_ms);
static void _tcpDel(plan_ma_comm_tcp_t *tcp,
                    int pipe, int thread);

static void *thRecv(void *);
static plan_ma_msg_t *tcpRecvTh(plan_ma_comm_t *comm);
static plan_ma_msg_t *tcpRecvBlockTh(plan_ma_comm_t *comm, int timeout_in_ms);

/** Extend memory allocated for the send buffer */
static void sendbufExtend(plan_ma_comm_tcp_t *tcp, int size);
/** Parse address in format x.x.x.x:port */
static int parseAddr(const char *addr, char *addr_out);
/** Initializes address structure using port and string address */
static int initSockAddr(int agent_id, struct sockaddr_in *a,
                        int port, const char *addr);
/** Sets socket as non-blocking */
static int sockSetNonblock(int agent_id, int sock);
/** Sets SO_REUSEADDR on socket */
static int sockSetReuseaddr(int agent_id, int sock);
/** Creates a new listening socket corresponding to the agent_id */
static int createListenSock(int agent_id, int agent_size, const char **addr_in);
/** Send greeting message over id'th socket. Greeting message is just ID
 *  of this node */
static int sendGreetingMsg(plan_ma_comm_tcp_t *tcp, int id);
/** Returns value of the greeting message received over the specified
 *  socket */
static int recvGreetingMsg(int node_id, int sock);
/** Creates and tries to connect a socket to id'th node.
 *  Returns 0 if connection was established, -1 on error and 1 if
 *  connection is in progress. */
static int createAndConnectSock(plan_ma_comm_tcp_t *tcp, int id,
                                const char *addr_in);
/** Returns 0 if connection was established, 1 if the connection should be
 *  retried with createAndConnectSock() or -1 on error.
 *  This function should be called only as result of active poll(). */
static int connectSockCheck(plan_ma_comm_tcp_t *tcp, int id);
/** Accepts pending connection on listening socket */
static int acceptConnection(plan_ma_comm_tcp_t *tcp);
/** Process all greeting messages and reorder sockets accordingly */
static int processGreetingMsgs(plan_ma_comm_tcp_t *tcp);
/** Establish the whole network of all nodes */
static int establishNetwork(plan_ma_comm_tcp_t *tcp, const char **addr);
/** Shutdown already established network. */
static int shutdownNetwork(plan_ma_comm_tcp_t *tcp);

plan_ma_comm_t *planMACommTCPNew(int agent_id, int agent_size,
                                 const char **addr, unsigned flags)
{
    plan_ma_comm_tcp_t *tcp;
    int i;

    tcp = BOR_ALLOC(plan_ma_comm_tcp_t);
    bzero(tcp, sizeof(*tcp));
    if (flags & PLAN_MA_COMM_TCP_NO_THREAD){
        tcp->use_th = 0;
        _planMACommInit(&tcp->comm, agent_id, agent_size,
                        tcpDel, tcpSendToNode, tcpRecv, tcpRecvBlock);
    }else{
        tcp->use_th = 1;
        _planMACommInit(&tcp->comm, agent_id, agent_size,
                        tcpDel, tcpSendToNode, tcpRecvTh, tcpRecvBlockTh);
    }
    tcp->listen_sock = -1;
    tcp->send_sock = BOR_ALLOC_ARR(int, agent_size);
    tcp->recv_sock = BOR_ALLOC_ARR(int, agent_size);
    for (i = 0; i < agent_size; ++i){
        tcp->send_sock[i] = -1;
        tcp->recv_sock[i] = -1;
    }

    // Create receiving socket but without accepting incomming connections
    tcp->listen_sock = createListenSock(agent_id, agent_size, addr);
    if (tcp->listen_sock < 0){
        BOR_FREE(tcp);
        return NULL;
    }

    // Make sure that all remotes are connected
    if (establishNetwork(tcp, addr) != 0){
        _tcpDel(tcp, 0, 0);
        return NULL;
    }

    // Allocate and initialize buffers
    tcp->buf = BOR_CALLOC_ARR(buf_t, agent_size);
    for (i = 0; i < agent_size; ++i){
        if (tcp->recv_sock[i] != -1)
            bufInit(tcp->buf + i);
    }

    // Allocate buffer used for packed messages
    tcp->sendbuf = BOR_ALLOC_ARR(char, SENDBUF_SIZE);
    tcp->sendbuf_size = SENDBUF_SIZE;

    if (tcp->use_th){
        // Initialize queue for incoming messages
        borRingQueueInit(&tcp->th_msgbuf, MSG_RING_BUF_SIZE);

        // Initialize pipe for communication between main thread and recv
        // thread
        if (pipe(tcp->th_pipe) != 0){
            ERRNO(tcp, "Could not create pipe:");
            _tcpDel(tcp, 0, 0);
            return NULL;
        }

        // Start recv thread
        if (pthread_create(&tcp->th_recv, NULL, thRecv, tcp) != 0){
            ERRNO(tcp, "Could not start receive thread.");
            _tcpDel(tcp, 1, 0);
            return NULL;
        }

    }else{
        // Initialize buffer for unpacked messages
        msgRingBufInit(&tcp->msgbuf);

        // Prepare descriptors for polling incoming messages
        tcp->recv_pfd = BOR_ALLOC_ARR(struct pollfd, agent_size);
        for (i = 0; i < agent_size; ++i){
            tcp->recv_pfd[i].fd = tcp->recv_sock[i];
            tcp->recv_pfd[i].events = POLLIN;
        }
    }

    return &tcp->comm;
}

static void _tcpDel(plan_ma_comm_tcp_t *tcp,
                    int pipe, int thread)
{
    int i, tcp_end = 0;

    // First terminate recv thread if used
    if (tcp->use_th){
        if (thread){
            if (write(tcp->th_pipe[1], &tcp_end, sizeof(int)) < 0){
                ERRNO(tcp, "Could not write to recv pipe:");
            }
            pthread_join(tcp->th_recv, NULL);
        }

        if (pipe){
            close(tcp->th_pipe[1]);
            close(tcp->th_pipe[0]);
        }
    }

    // Then shutdown whole network
    shutdownNetwork(tcp);

    // And finally dealloc allocated resources
    if (tcp->use_th){
        borRingQueueFree(&tcp->th_msgbuf);
    }else{
        msgRingBufFree(&tcp->msgbuf);
        if (tcp->recv_pfd)
            BOR_FREE(tcp->recv_pfd);
    }

    if (tcp->send_sock != NULL)
        BOR_FREE(tcp->send_sock);
    if (tcp->recv_sock != NULL)
        BOR_FREE(tcp->recv_sock);
    for (i = 0; tcp->buf != NULL && i < tcp->comm.node_size; ++i)
        bufFree(tcp->buf + i);
    if (tcp->buf != NULL)
        BOR_FREE(tcp->buf);
    if (tcp->sendbuf)
        BOR_FREE(tcp->sendbuf);

    BOR_FREE(tcp);
}

static void tcpDel(plan_ma_comm_t *comm)
{
    plan_ma_comm_tcp_t *tcp = TCP(comm);
    _tcpDel(tcp, 1, 1);
}

static int tcpSendToNode(plan_ma_comm_t *comm, int node_id,
                         const plan_ma_msg_t *msg)
{
    plan_ma_comm_tcp_t *tcp = TCP(comm);
    struct pollfd pfd;
    char *outbuf;
    int size, sent;
    ssize_t send_ret;
    int ret = 0;

    // Never send to itself
    if (node_id == tcp->comm.node_id)
        return -1;

    // Pack the message into prepared buffer. Extend the buffer if needed.
    do {
        outbuf = tcp->sendbuf;
        size = tcp->sendbuf_size - 4;
        ret = planMAMsgPackToBuf(msg, outbuf + 4, &size);
        if (ret != 0)
            sendbufExtend(tcp, size + 4);
    } while (ret != 0);

    // Write size of the message at the beggining of the buffer
    *(uint32_t *)outbuf = TO_LE32(size);

    // Send the buffer over network
    pfd.events = POLLOUT;
    pfd.fd = tcp->send_sock[node_id];
    size += 4;
    sent = 0;
    while (sent != size){
        if (poll(&pfd, 1, -1) < 0){
            ERRNO(tcp, "Poll error:");
            continue;
        }

        if (pfd.revents & POLLOUT){
            send_ret = write(pfd.fd, outbuf + sent, size - sent);
            if (send_ret < 0){
                ERR(tcp, "Could not send message to %d: %s",
                    node_id, strerror(errno));
                return -1;
            }

            sent += send_ret;
        }
    }

    return 0;
}

static plan_ma_msg_t *_tcpRecv(plan_ma_comm_tcp_t *tcp, int timeout_in_ms)
{
    struct pollfd *pfd = tcp->recv_pfd;
    plan_ma_msg_t *msg = NULL;
    int i, ret, rsize;

    // If we have buffered a message, return it immediately
    if ((msg = msgRingBufPop(&tcp->msgbuf)) != NULL)
        return msg;

    while ((msg = msgRingBufPop(&tcp->msgbuf)) == NULL) {
        ret = poll(pfd, tcp->comm.node_size, timeout_in_ms);
        if (ret == 0){
            // Timeout -- this is ok, just don't return message
            return NULL;

        }else if (ret < 0){
            ERRNO(tcp, "Poll error:");
            return NULL;
        }

        for (i = 0; i < tcp->comm.node_size; ++i){
            if (pfd[i].revents & POLLIN){
                // Data are available, so read them into buffer
                rsize = bufFillFromSock(tcp->buf + i, tcp->recv_sock[i]);
                if (rsize < 0){
                    ERRNO(tcp, "Recv error:");
                    continue;

                }else if (rsize == 0){
                    // The remote hung-up on us, so just remove the socket
                    // from the receiving sockets.
                    pfd[i].fd = -1;
                    continue;

                }else{
                    // Parse as many as possible messages from the buffer
                    // and push the messages to the queue.
                    while ((msg = bufExtractMsg(tcp->buf + i)) != NULL){
                        msgRingBufPush(&tcp->msgbuf, msg);
                    }
                }
            }
        }
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

static void *thRecv(void *_tcp)
{
    plan_ma_comm_tcp_t *tcp = _tcp;
    struct pollfd pfd[tcp->comm.node_size];
    plan_ma_msg_t *msg = NULL;
    int i, ret, rsize, terminate;

    for (i = 0; i < tcp->comm.node_size; ++i){
        if (i == tcp->comm.node_id){
            pfd[i].fd = tcp->th_pipe[0];
        }else{
            pfd[i].fd = tcp->recv_sock[i];
        }
        pfd[i].events = POLLIN;
    }

    terminate = 0;
    while (!terminate){
        ret = poll(pfd, tcp->comm.node_size, -1);
        if (ret < 0){
            ERRNO(tcp, "Poll error:");
            continue;
        }

        for (i = 0; i < tcp->comm.node_size; ++i){
            if ((pfd[i].revents & POLLIN) && i == tcp->comm.node_id){
                terminate = 1;
                break;

            }else if (pfd[i].revents & POLLIN){
                // Data are available, so read them into buffer
                rsize = bufFillFromSock(tcp->buf + i, tcp->recv_sock[i]);
                if (rsize < 0){
                    ERRNO(tcp, "Recv error:");
                    continue;

                }else if (rsize == 0){
                    // The remote hung-up on us, so just remove the socket
                    // from the receiving sockets.
                    pfd[i].fd = -1;
                    continue;

                }else{
                    // Parse as many as possible messages from the buffer
                    // and push the messages to the queue.
                    while ((msg = bufExtractMsg(tcp->buf + i)) != NULL){
                        borRingQueuePush(&tcp->th_msgbuf, msg);
                    }
                }
            }
        }
    }

    return NULL;
}

static plan_ma_msg_t *tcpRecvTh(plan_ma_comm_t *comm)
{
    plan_ma_comm_tcp_t *tcp = TCP(comm);
    return borRingQueuePop(&tcp->th_msgbuf);
}

static plan_ma_msg_t *tcpRecvBlockTh(plan_ma_comm_t *comm, int timeout_in_ms)
{
    plan_ma_comm_tcp_t *tcp = TCP(comm);
    if (timeout_in_ms <= 0)
        return borRingQueuePopBlock(&tcp->th_msgbuf);
    return borRingQueuePopBlockTimeout(&tcp->th_msgbuf, timeout_in_ms);
}


static void sendbufExtend(plan_ma_comm_tcp_t *tcp, int size)
{
    int pagesize;

    pagesize = sysconf(_SC_PAGESIZE);
    size = ((size / pagesize) + 1) * pagesize;
    tcp->sendbuf_size = size;
    BOR_FREE(tcp->sendbuf);
    tcp->sendbuf = BOR_ALLOC_ARR(char, tcp->sendbuf_size);
}

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

static int initSockAddr(int agent_id, struct sockaddr_in *a,
                        int port, const char *addr)
{
    int ret;

    bzero(a, sizeof(*a));
    ret = inet_pton(AF_INET, addr, &a->sin_addr);
    if (ret == 0){
        ERR2(agent_id, "Invalid address: `%s'", addr);
        return -1;

    }else if (ret < 0){
        ERR2(agent_id, "Could not convert address `%s': %s",
             addr, strerror(errno));
        return -1;
    }

    a->sin_family = AF_INET;
    a->sin_port = htons(port);
    return 0;
}

static int sockSetNonblock(int agent_id, int sock)
{
    if (fcntl(sock, F_SETFL, O_NONBLOCK) != 0){
        ERR2NO(agent_id, "Could not set socket as non-blocking:");
        return -1;
    }
    return 0;
}

static int sockSetReuseaddr(int agent_id, int sock)
{
    int yes = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) != 0){
        ERR2NO(agent_id, "Could not set SO_REUSEADDR:");
        return -1;
    }
    return 0;
}

static int createListenSock(int agent_id, int agent_size, const char **addr_in)
{
    int sock, port;
    char addr[30];
    struct sockaddr_in recv_addr;

    port = parseAddr(addr_in[agent_id], addr);
    if (port < 0){
        ERR2(agent_id, "Could not parse address:port from input:`%s'",
             addr_in[agent_id]);
        return -1;
    }

    // Prepare address to bind to
    if (initSockAddr(agent_id, &recv_addr, port, addr) != 0)
        return -1;

    // Create a socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
        return sock;

    // Set the socket to non-blocking mode
    if (sockSetNonblock(agent_id, sock) != 0){
        close(sock);
        return -1;
    }

    // Set socket to reuse time-wait address
    if (sockSetReuseaddr(agent_id, sock) != 0){
        close(sock);
        return -1;
    }

    // Bind socket to the specified address
    if (bind(sock, (struct sockaddr *)&recv_addr, sizeof(recv_addr)) < 0){
        ERR2(agent_id, "Could not bind to address `%s:%d': %s",
             addr, port, strerror(errno));
        close(sock);
        return -1;
    }

    // Listen on this socket and make buffer enough for all agents
    if (listen(sock, agent_size) != 0){
        ERR2NO(agent_id, "listen(2) failed:");
        close(sock);
        return -1;
    }

    return sock;
}

static int sendGreetingMsg(plan_ma_comm_tcp_t *tcp, int id)
{
    int ret;
    uint16_t msg;

    // Send the greeting message which is ID of the current node
    msg = TO_LE16(tcp->comm.node_id);
    errno = 0;
    ret = send(tcp->send_sock[id], &msg, 2, 0);

    if (ret != 2){
        ERRNO(tcp, "Could not send greeting message:");
        return -1;
    }

    return 0;
}

static int recvGreetingMsg(int node_id, int sock)
{
    int ret;
    uint16_t msg;

    msg = 0;
    errno = 0;
    ret = recv(sock, &msg, 2, MSG_WAITALL);
    if (ret != 2){
        ERR2NO(node_id, "Could not receive greeting message:");
        return -1;
    }

    return FROM_LE16(msg);
}

static int createAndConnectSock(plan_ma_comm_tcp_t *tcp, int id,
                                const char *addr_in)
{
    int sock, port, ret;
    char addr[30];
    struct sockaddr_in send_addr;

    // Check and setup remote port and address
    port = parseAddr(addr_in, addr);
    if (port < 0){
        ERR(tcp, "Could not parse address:port from input:`%s'", addr_in);
        return -1;
    }

    if (initSockAddr(tcp->comm.node_id, &send_addr, port, addr) != 0)
        return -1;

    // Create a new socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0){
        ERRNO(tcp, "Could not create a socket:");
        return -1;
    }

    // Set the socket to non-blocking mode
    if (sockSetNonblock(tcp->comm.node_id, sock) != 0){
        close(sock);
        return -1;
    }

    // Connect to the remote
    ret = connect(sock, (struct sockaddr *)&send_addr, sizeof(send_addr));
    if (ret == 0){
        // The connection was successful
        if (sendGreetingMsg(tcp, id) != 0)
            return -1;
        tcp->send_sock[id] = sock;
        return 0;

    }else if (errno == EINPROGRESS){
        // In non-blocking mode in-progress is also ok
        tcp->send_sock[id] = sock;
        return 1;

    }else{
        ERR(tcp, "Something went wrong during connection to `%s': %s",
            addr_in, strerror(errno));
        return -1;
    }
}

static int connectSockCheck(plan_ma_comm_tcp_t *tcp, int id)
{
    int error;
    socklen_t len;

    len = sizeof(error);
    error = 0;
    if (getsockopt(tcp->send_sock[id], SOL_SOCKET, SO_ERROR, &error, &len) != 0){
        ERRNO(tcp, "Could not get sock error:");
        return -1;
    }

    if (error == 0){
        if (sendGreetingMsg(tcp, id) != 0)
            return -1;
        return 0;
    }

    close(tcp->send_sock[id]);
    tcp->send_sock[id] = -1;
    if (error == ECONNREFUSED
            || error == EINPROGRESS
            || error == EALREADY)
        return 1;

    ERRNO(tcp, "Unexpected error:");
    return -1;
}

static int acceptConnection(plan_ma_comm_tcp_t *tcp)
{
    int sock, id;

    sock = accept(tcp->listen_sock, NULL, NULL);
    if (sock < 0){
        ERRNO(tcp, "Could not accept connection:");
        return -1;
    }

    // Set the socket to non-blocking mode
    if (sockSetNonblock(tcp->comm.node_id, sock) != 0){
        close(sock);
        return -1;
    }

    // Save connected socket and send greeting message
    for (id = 0; id < tcp->comm.node_size; ++id){
        if (tcp->recv_sock[id] == -1){
            tcp->recv_sock[id] = sock;
            //if (sendGreetingMsg(tcp, id) == -1)
            //    return -1;
            return 0;
        }
    }

    ERRM(tcp, "It seems all nodes are already connected.");
    close(sock);
    return -1;
}

static int processGreetingMsgs(plan_ma_comm_tcp_t *tcp)
{
    struct pollfd pfd[tcp->comm.node_size];
    int id, ids[tcp->comm.node_size], tmp[tcp->comm.node_size];
    int i, received = 0, ret;

    for (i = 0; i < tcp->comm.node_size; ++i){
        pfd[i].fd = tcp->recv_sock[i];
        pfd[i].events = POLLIN;
    }

    while (received < tcp->comm.node_size - 1){
        ret = poll(pfd, tcp->comm.node_size, ESTABLISH_TIMEOUT);
        if (ret == 0){
            ERR(tcp, "Could not establish agent cluster in defined timeout"
                     " (%d s).", ESTABLISH_TIMEOUT);
            return -1;

        }else if (ret < 0){
            ERRNO(tcp, "Poll error:");
            return -1;
        }

        for (i = 0; i < tcp->comm.node_size; ++i){
            if (pfd[i].revents & POLLIN){
                pfd[i].fd = -1;
                id = recvGreetingMsg(tcp->comm.node_id, tcp->recv_sock[i]);
                if (id < 0)
                    return -1;
                ids[i] = id;
                ++received;
            }
        }
    }

    // Reorder sockets according to their IDs
    memcpy(tmp, tcp->recv_sock, sizeof(int) * tcp->comm.node_size);
    for (i = 0; i < tcp->comm.node_size; ++i)
        tcp->recv_sock[i] = -1;
    for (i = 0; i < tcp->comm.node_size; ++i){
        if (tmp[i] != -1)
            tcp->recv_sock[ids[i]] = tmp[i];
    }

    return 0;
}

static int establishNetwork(plan_ma_comm_tcp_t *tcp, const char **addr)
{
    struct pollfd pfd[tcp->comm.node_size];
    int i, ret, num_connected = 0;
    bor_timer_t timer;

    borTimerStart(&timer);
    for (i = 0; i < tcp->comm.node_size; ++i){
        pfd[i].fd = -1;
        pfd[i].events = POLLOUT;
    }
    pfd[tcp->comm.node_id].fd = tcp->listen_sock;
    pfd[tcp->comm.node_id].events = POLLIN | POLLPRI;

    while (num_connected != 2 * (tcp->comm.node_size - 1)){
        // Try to connect all remaining sockets but initialize connection
        // only to the nodes with lower ID
        for (i = 0; i < tcp->comm.node_size; ++i){
            if (i == tcp->comm.node_id)
                continue;
            if (tcp->send_sock[i] == -1){
                ret = createAndConnectSock(tcp, i, addr[i]);
                if (ret == 0){
                    // Socket connected
                    ++num_connected;

                }else if (ret == 1){
                    // Socket is pending, add it to poll descritors
                    pfd[i].fd = tcp->send_sock[i];

                }else if (ret < 0){
                    return -1;
                }
            }
        }

        ret = poll(pfd, tcp->comm.node_size, ESTABLISH_TIMEOUT);
        if (ret == 0){
            ERR(tcp, "Could not establish agent cluster in defined timeout"
                     " (%d ms).", ESTABLISH_TIMEOUT);
            return -1;

        }else if (ret < 0){
            ERRNO(tcp, "Poll error:");
            return -1;
        }

        for (i = 0; i < tcp->comm.node_size; ++i){
            if (pfd[i].revents & POLLIN){
                // Accept incoming connection on listen socket
                if (acceptConnection(tcp) != 0)
                    return -1;
                ++num_connected;
            }

            if (pfd[i].revents & POLLOUT){
                // The pending connection is ready or it failed.
                pfd[i].fd = -1;
                ret = connectSockCheck(tcp, i);
                if (ret == 0){
                    ++num_connected;
                }else if (ret < 0){
                    return -1;
                }
            }
        }

        borTimerStop(&timer);
        if (borTimerElapsedInMs(&timer) > ESTABLISH_TIMEOUT){
            ERR(tcp, "Could not establish agent cluster in defined timeout"
                     " (%d ms).", ESTABLISH_TIMEOUT);
            return -1;
        }
    }

    // Now all connections are established.
    // Process all greeting messages and reorder sockets accordingly.
    return processGreetingMsgs(tcp);
}

static int shutdownNetwork(plan_ma_comm_tcp_t *tcp)
{
    struct pollfd pfd[tcp->comm.node_size];
    int i, ret, remain;
    char buf[128];

    // First shutdown all outgoing connections (send EOF)
    for (i = 0; i < tcp->comm.node_size; ++i){
        if (tcp->send_sock[i] == -1)
            continue;
        if (shutdown(tcp->send_sock[i], SHUT_WR) != 0){
            ERR(tcp, "Could not shutdown socket connected to %d: %s",
                i, strerror(errno));
        }
    }

    // Then wait for EOFs from all agents using poll method
    remain = 0;
    for (i = 0; i < tcp->comm.node_size; ++i){
        pfd[i].fd = tcp->recv_sock[i];
        pfd[i].events = POLLIN;
        if (pfd[i].fd != -1)
            ++remain;
    }

    while (remain > 0){
        ret = poll(pfd, tcp->comm.node_size, SHUTDOWN_TIMEOUT);
        if (ret == 0){
            ERR(tcp, "Could not shut-down network in defined timeout (%d ms).",
                SHUTDOWN_TIMEOUT);
            return -1;

        }else if (ret < 0){
            ERRNO(tcp, "Poll error:");
            return -1;
        }

        for (i = 0; i < tcp->comm.node_size; ++i){
            if (pfd[i].revents & POLLIN){
                ret = read(tcp->recv_sock[i], buf, 128);
                pfd[i].fd = -1;
                if (ret < 0){
                    ERR(tcp, "Error while receiving EOF from %d: %s",
                        i, strerror(errno));

                }else if (ret == 0){
                    --remain;
                }
            }
        }
    }

    // Close all sockets
    for (i = 0; i < tcp->comm.node_size; ++i){
        if (tcp->send_sock[i] == -1)
            continue;
        if (close(tcp->send_sock[i]) != 0){
            ERR(tcp, "Error while closing socket %d: %s", i, strerror(errno));
        }
    }
    for (i = 0; i < tcp->comm.node_size; ++i){
        if (tcp->recv_sock[i] == -1)
            continue;
        if (close(tcp->recv_sock[i]) != 0){
            ERR(tcp, "Error while closing socket %d: %s", i, strerror(errno));
        }
    }
    close(tcp->listen_sock);

    return 0;
}

/*** msg_ring_buf_t ***/
static void msgRingBufInit(msg_ring_buf_t *b)
{
    b->buf = BOR_ALLOC_ARR(plan_ma_msg_t *, MSG_RING_BUF_SIZE);
    b->size = MSG_RING_BUF_SIZE;
    b->head = b->tail = 0;
}

static void msgRingBufFree(msg_ring_buf_t *b)
{
    int i;

    for (i = b->head; i != b->tail; i = msgRingBufNext(i, b->size)){
        planMAMsgDel(b->buf[i]);
    }
    BOR_FREE(b->buf);
}

static void msgRingBufExtend(msg_ring_buf_t *b)
{
    int oldsize, i, j;
    plan_ma_msg_t **newbuf;

    oldsize = b->size;
    b->size *= 2;
    newbuf = BOR_ALLOC_ARR(plan_ma_msg_t *, b->size);
    for (j = 0, i = b->head; i != b->tail; i = msgRingBufNext(i, oldsize), ++j){
        newbuf[j] = b->buf[i];
    }

    BOR_FREE(b->buf);
    b->buf = newbuf;
    b->head = 0;
    b->tail = j;
}

static void msgRingBufPush(msg_ring_buf_t *b, plan_ma_msg_t *msg)
{
    if (msgRingBufFull(b))
        msgRingBufExtend(b);
    b->buf[b->tail] = msg;
    b->tail = msgRingBufNext(b->tail, b->size);
}

static plan_ma_msg_t *msgRingBufPop(msg_ring_buf_t *b)
{
    plan_ma_msg_t *msg;

    if (msgRingBufEmpty(b))
        return NULL;

    msg = b->buf[b->head];
    b->head = msgRingBufNext(b->head, b->size);
    return msg;
}

_bor_inline int msgRingBufNext(int c, int size)
{
    return (c + 1) % size;
}

_bor_inline int msgRingBufFull(const msg_ring_buf_t *b)
{
    return msgRingBufNext(b->tail, b->size) == b->head;
}

_bor_inline int msgRingBufEmpty(const msg_ring_buf_t *b)
{
    return b->head == b->tail;
}


/*** buf_t ***/
static void bufInit(buf_t *buf)
{
    buf->buf = BOR_ALLOC_ARR(char, BUF_INIT_SIZE);
    buf->size = BUF_INIT_SIZE;
    buf->beg = buf->end = buf->buf;
    buf->remain = buf->size;
    buf->read_size = 0;
    buf->msg_size = -1;
}

static void bufFree(buf_t *buf)
{
    if (buf->buf)
        BOR_FREE(buf->buf);
}

static void bufExtend(buf_t *buf, int size)
{
    int ext, pagesize;

    pagesize = sysconf(_SC_PAGESIZE);
    size = ((size / pagesize) + 1) * pagesize;
    ext = size - buf->size;

    buf->size = size;
    buf->buf = BOR_REALLOC_ARR(buf->buf, char, buf->size);
    buf->remain += ext;
    buf->end = buf->buf + buf->size - buf->remain;
    buf->beg = buf->end - buf->read_size;
}

static void bufShiftData(buf_t *buf)
{
    int i;

    for (i = 0; i < buf->read_size; ++i)
        buf->buf[i] = buf->beg[i];

    buf->beg = buf->buf;
    buf->end = buf->beg + buf->read_size;
    buf->remain = buf->size - buf->read_size;
}

static int bufFillFromSock(buf_t *buf, int sock)
{
    ssize_t rsize;

    rsize = recv(sock, buf->end, buf->remain, 0);
    if (rsize <= 0)
        return rsize;

    buf->end += rsize;
    buf->remain -= rsize;
    buf->read_size += rsize;
    return 1;
}

static plan_ma_msg_t *bufExtractMsg(buf_t *buf)
{
    plan_ma_msg_t *msg = NULL;

    // If msg_size is not pre-parsed and there is enough data to read the
    // size of the next message, do it.
    if (buf->msg_size <= 0 && buf->read_size >= 4){
        buf->msg_size = FROM_LE32(*(uint32_t *)buf->beg);
        buf->beg += 4;
        buf->read_size -= 4;

        // Make sure there is always at least twice as memory that is
        // needed for the biggest message received so far.
        if (2 * buf->msg_size + 8 > buf->size)
            bufExtend(buf, 2 * buf->msg_size + 8);

        // Shift data to the beggining of the buffer array if there is not
        // enough space for it. We need the packed message to be stored in
        // a consecutive byte array.
        if (buf->msg_size > buf->read_size + buf->remain)
            bufShiftData(buf);
    }

    // If we have pre-parsed message size and we have also enough data to
    // parse the packed message, unpack the message from buffer.
    if (buf->msg_size > 0 && buf->read_size >= buf->msg_size){
        msg = planMAMsgUnpacked(buf->beg, buf->msg_size);
        buf->beg += buf->msg_size;
        buf->read_size -= buf->msg_size;
        buf->msg_size = -1;
    }

    // If the buffer is empty (buf->read_size), reset the buffer pointers
    // because we will not need to shift data later.
    // Also, if we are at the end of the buffer and there is not enough
    // space for 4 bytes of size of a message, shift data to the beggining
    // of the buffer.
    if (buf->read_size == 0
            || (buf->msg_size <= 0 && buf->read_size + buf->remain < 4)){
        bufShiftData(buf);
    }

    return msg;
}
