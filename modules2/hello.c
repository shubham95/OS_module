#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <net/sock.h>

#include <linux/spinlock.h>
#include <linux/spinlock_types.h>
#include <linux/list.h>
#include <net/tcp.h>
#include <linux/time.h>

//Hooks
extern void (*do_sys_open_hook)(int dfd, const char __user *filename, int flags, umode_t mode, int fd);
extern int (*__close_fd_hook)(struct files_struct *files, unsigned fd);
extern ssize_t (*ksys_write_hook)(unsigned int fd, const char __user *buf, size_t count);
static spinlock_t list_lock = __SPIN_LOCK_UNLOCKED(list_lock);
//Socket Helpers
static int PORT = 2325;
static struct socket *tcp_client_connect(u8 *destip);
static int tcp_client_send(struct socket *sock, const char *buf, const size_t length, unsigned long flags);
static u32 create_address(u8 *ip);

static char *dir_name = "connoisseur_dir420";
static u8 destip[5] = {127, 0, 0, 1, '\0'};

//List Declarations
static LIST_HEAD(fd_list);
static void free_list(void);
struct descriptor
{
    bool used_flag;
    int pid, fd;
    struct socket *sock;
    struct list_head list;
};
static struct socket *getSocket(int pid, int fd);
static void save_socket(int pid, int fd, struct socket *sock);
static void free_list_one_node(unsigned fd, int pid);
static u8 padbuff[1024];

static int (___close_fd_hook)(struct files_struct *files, unsigned fd){
    stac();
    struct socket* conn_socket;
    conn_socket = getSocket(current->pid,fd);
    sock_release(conn_socket);
   // tcp_client_send(conn_socket, buf, count, MSG_WAITALL);
    free_list_one_node(fd,current->pid);


    clac();
    return 0;
}
static void _do_sys_open_hook(int dfd, const char __user *filename, int flags, umode_t mode, int fd)
{
    stac();
    char *pch = strstr(filename, dir_name);
    if (pch)
    {
        printk(KERN_INFO "comm [%s], pid [%d] , fd [%d], filename [%s]\n", current->comm, current->pid, fd, filename);
        struct socket* conn_socket;
        char* fname = vzalloc(1024);
        int len = strlen(filename);
        strcat(fname,filename);
        // int padding = 1024-len;
        // int strsize = strlen(filename)+1;
        // copy_to_user(fname, filename, strsize);

        conn_socket = tcp_client_connect(destip);
        // printk(KERN_INFO "Buffer allocated\n");
        //tcp_client_send(conn_socket, filename, strlen(filename), MSG_WAITALL);
        tcp_client_send(conn_socket, fname, 1024, MSG_WAITALL);
        //tcp_client_send(conn_socket, padbuff, 1024, MSG_WAITALL);
        save_socket(current->pid,fd,conn_socket);
    }
    clac();
}
static ssize_t _ksys_write_hook(unsigned int fd, const char __user *buf, size_t count)
{
    stac();
    struct socket* conn_socket = NULL;
    conn_socket = getSocket(current->pid,fd);
    if (conn_socket)
    {
        tcp_client_send(conn_socket, buf, count, MSG_WAITALL);
    }
    clac();
    return -9999;
}
int init_module(void)
{
    printk(KERN_INFO "Hello kernel\n");
    fd_list.next = &fd_list;
    fd_list.prev = &fd_list;
    list_lock = __SPIN_LOCK_UNLOCKED(list_lock);
    do_sys_open_hook = &_do_sys_open_hook;
    ksys_write_hook = &_ksys_write_hook;
    //__close_fd_hook = &___close_fd_hook;
    return 0;
}

void cleanup_module(void)
{
    do_sys_open_hook = NULL;
    ksys_write_hook = NULL;
    __close_fd_hook = NULL;
    free_list();
    printk(KERN_INFO "Goodbye kernel\n");
}

//=======================SOCKET CONNECTION CODES==============================//
static void print_list(void)
{
    struct descriptor *ptr = NULL;
    list_for_each_entry ( ptr , &fd_list, list ) 
    { 
		printk ("Pid [%d], fd [%d], socket [%ld]\n" , ptr->pid,ptr->fd,ptr->sock); 
    }
}
static struct socket *getSocket(int pid, int fd)
{
    struct descriptor *ptr = NULL;
    list_for_each_entry(ptr, &fd_list, list)
    {
        if (ptr->pid == pid && ptr->fd == fd && ptr->used_flag)
        {
            return ptr->sock;
        }
    }
    return NULL;
}

static void save_socket(int pid, int fd, struct socket *sock)
{
    struct descriptor* list_node = vmalloc(sizeof(struct descriptor));
    list_node->used_flag =true;
    list_node->pid = pid;
    list_node->fd = fd;
    list_node->sock = sock;
    unsigned long flags;
    spin_lock_irqsave(&list_lock, flags);
    INIT_LIST_HEAD(&list_node->list);
    list_add(&list_node->list, &fd_list);
    spin_unlock_irqrestore(&list_lock, flags);
    //print_list();
}

static void free_list(void)
{
    struct descriptor *ptr = NULL, *pos;
    list_for_each_entry_safe(pos, ptr, &fd_list, list)
    {
        printk ("Pid [%d], fd [%d], socket [%ld]\n" , ptr->pid,ptr->fd,ptr->sock); 
        if(ptr && ptr->sock)
        {
           sock_release(ptr->sock);
           //kfree(ptr);
        }
        else
        {
            return;
        }
    }
}

// free one node
static void free_list_one_node(unsigned fd, int pid)
{

    struct descriptor *ptr = NULL;
    list_for_each_entry ( ptr , &fd_list, list ) 
    { 
        if(ptr && ptr->fd == fd && ptr->pid == pid)
        {
            ptr->used_flag =false;
        }
    }
}

static int tcp_client_send(struct socket *sock, const char *buf, const size_t length, unsigned long flags)
{
    struct msghdr msg;
    //struct iovec iov;
    struct kvec vec;
    int len, written = 0, left = length;
    mm_segment_t oldmm;

    msg.msg_name = 0;
    msg.msg_namelen = 0;
    /*
        msg.msg_iov     = &iov;
        msg.msg_iovlen  = 1;
        */
    msg.msg_control = NULL;
    msg.msg_controllen = 0;
    msg.msg_flags = flags;

    oldmm = get_fs();
    set_fs(KERNEL_DS);
repeat_send:
    /*
        msg.msg_iov->iov_len  = left;
        msg.msg_iov->iov_base = (char *)buf + written; 
        */
    vec.iov_len = left;
    vec.iov_base = (char *)buf + written;

    //len = sock_sendmsg(sock, &msg, left);
    len = kernel_sendmsg(sock, &msg, &vec, left, left);
    if ((len == -ERESTARTSYS) || (!(flags & MSG_DONTWAIT) &&
                                  (len == -EAGAIN)))
        goto repeat_send;
    if (len > 0)
    {
        written += len;
        left -= len;
        if (left)
            goto repeat_send;
    }
    set_fs(oldmm);
    return written ? written : len;
}

static struct socket *tcp_client_connect(u8 *destip)
{
    struct socket *conn_socket;
    struct sockaddr_in saddr;
    printk(KERN_INFO "Connecting\n");
    //DECLARE_WAIT_QUEUE_HEAD(recv_wait);
    int ret = -1;
    ret = sock_create(PF_INET, SOCK_STREAM, IPPROTO_TCP, &conn_socket);
    if (ret < 0)
    {
        printk(KERN_INFO "Error in Socket creation\n");
        pr_info(" *** mtp | Error: %d while creating first socket. | "
                "setup_connection *** \n",
                ret);
        goto err;
    }
    printk(KERN_INFO "Socket created\n");
    memset(&saddr, 0, sizeof(saddr));
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(PORT);
    saddr.sin_addr.s_addr = htonl(create_address(destip));
    ret = 0;
    ret = conn_socket->ops->connect(conn_socket, (struct sockaddr *)&saddr, sizeof(saddr), O_RDWR);
    if (ret && (ret != -EINPROGRESS))
    {
        pr_info(" *** mtp | Error: %d while connecting using conn "
                "socket. | setup_connection *** \n",
                ret);
        goto err;
    }
    else
    {
        printk(KERN_INFO "Socket connected\n");
        return conn_socket;
    }
err:
    printk(KERN_INFO "Error in Socket connection");
    return NULL;
}

u32 create_address(u8 *ip)
{
    u32 addr = 0;
    int i;

    for (i = 0; i < 4; i++)
    {
        addr += ip[i];
        if (i == 3)
            break;
        addr <<= 8;
    }
    return addr;
}
