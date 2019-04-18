/** 
* @file hello.c 
* @author Akshat Sinha 
* @date 10 Sept 2016 
* @version 0.1 
* @brief An introductory "Hello World!" loadable kernel 
* module (LKM) that can display a message in the /var/log/kern.log 
* file when the module is loaded and removed. The module can accept 
* an argument when it is loaded -- the name, which appears in the 
* kernel log files. 
*/


//ref : https://github.com/abysamross/simple-linux-kernel-tcp-client-server/blob/master/network_client.c
//https://stackoverflow.com/questions/46573001/send-data-from-linux-kernel-module
#include <linux/module.h>	 /* Needed by all modules */ 
#include <linux/kernel.h>	 /* Needed for KERN_INFO */ 
#include <linux/init.h>	 /* Needed for the macros */ 
#include<linux/mm.h>
#include<linux/mm_types.h>
#include<linux/file.h>
#include<linux/fs.h>
#include<linux/path.h>
#include<linux/slab.h>
#include<linux/dcache.h>
#include<linux/sched.h>
#include<linux/uaccess.h>
#include<linux/fs_struct.h>
#include <asm/tlbflush.h>
//#include<linux/uaccess.h>
#include<linux/device.h>

#include <linux/net.h>
#include <net/sock.h>
#include <linux/tcp.h>
#include <linux/in.h>
#include <asm/uaccess.h>
#include <linux/socket.h>
#include <linux/slab.h>
#define PORT 2325


///< The license type -- this affects runtime behavior 
//MODULE_LICENSE("GPL"); 

///< The author -- visible when you use modinfo 
MODULE_AUTHOR("Shubham Pandey"); 

///< The description -- see modinfo 
MODULE_DESCRIPTION("A simple Hello world LKM!"); 

///< The version of the module 
MODULE_VERSION("0.1"); 

extern int (*do_sys_open_hook)(int dfd, const char __user *filename, int flags, umode_t mode, int fd);
extern int (*ksys_write_hook)(unsigned int fd, const char __user *buf, size_t count);

extern int checkpoint_pid;
struct socket *conn_socket = NULL;

//clent socket-----------------------


int tcp_client_send(struct socket *sock, const char *buf, const size_t length,\
                unsigned long flags)
{
        struct msghdr msg;
        //struct iovec iov;
        struct kvec vec;
        int len, written = 0, left = length;
        mm_segment_t oldmm;

        msg.msg_name    = 0;
        msg.msg_namelen = 0;
        /*
        msg.msg_iov     = &iov;
        msg.msg_iovlen  = 1;
        */
        msg.msg_control = NULL;
        msg.msg_controllen = 0;
        msg.msg_flags   = flags;

        oldmm = get_fs(); set_fs(KERNEL_DS);
repeat_send:
        /*
        msg.msg_iov->iov_len  = left;
        msg.msg_iov->iov_base = (char *)buf + written; 
        */
        vec.iov_len = left;
        vec.iov_base = (char *)buf + written;

        //len = sock_sendmsg(sock, &msg, left);
        len = kernel_sendmsg(sock, &msg, &vec, left, left);
        if((len == -ERESTARTSYS) || (!(flags & MSG_DONTWAIT) &&\
                                (len == -EAGAIN)))
                goto repeat_send;
        if(len > 0)
        {
                written += len;
                left -= len;
                if(left)
                        goto repeat_send;
        }
        set_fs(oldmm);
        return written ? written:len;
}


int ret = -1;
u32 create_address(u8 *ip)
{
        u32 addr = 0;
        int i;

        for(i=0; i<4; i++)
        {
                addr += ip[i];
                if(i==3)
                        break;
                addr <<= 8;
        }
        return addr;
}


int tcp_client_connect(void)
{
        struct sockaddr_in saddr;
        /*
        struct sockaddr_in daddr;
        struct socket *data_socket = NULL;
        */
        unsigned char destip[5] = {172,23,58,173,'\0'};
        /*
        char *response = kmalloc(4096, GFP_KERNEL);
        char *reply = kmalloc(4096, GFP_KERNEL);
        */
        int len = 49;
        char response[len+1];
        char reply[len+1];
        int ret = -1;

        //DECLARE_WAITQUEUE(recv_wait, current);
        DECLARE_WAIT_QUEUE_HEAD(recv_wait);
        
        ret = sock_create(PF_INET, SOCK_STREAM, IPPROTO_TCP, &conn_socket);
        if(ret < 0)
        {
                pr_info(" *** mtp | Error: %d while creating first socket. | "
                        "setup_connection *** \n", ret);
                goto err;
        }

        memset(&saddr, 0, sizeof(saddr));
        saddr.sin_family = AF_INET;
        saddr.sin_port = htons(PORT);
        saddr.sin_addr.s_addr = htonl(create_address(destip));

        ret = conn_socket->ops->connect(conn_socket, (struct sockaddr *)&saddr\
                        , sizeof(saddr), O_RDWR);
        if(ret && (ret != -EINPROGRESS))
        {
                pr_info(" *** mtp | Error: %d while connecting using conn "
                        "socket. | setup_connection *** \n", ret);
                goto err;
        }

        //memset(&reply, 0, len+1);
        //strcat(reply, "HOLA"); 
       // tcp_client_send(conn_socket, reply, strlen(reply), MSG_DONTWAIT);

        // wait_event_timeout(recv_wait,\
        //                 !skb_queue_empty(&conn_socket->sk->sk_receive_queue),\
        //                                                                 5*HZ);
        // /*
        // add_wait_queue(&conn_socket->sk->sk_wq->wait, &recv_wait);
        // while(1)
        // {
        //         __set_current_status(TASK_INTERRUPTIBLE);
        //         schedule_timeout(HZ);
        // */
        //         if(!skb_queue_empty(&conn_socket->sk->sk_receive_queue))
        //         {
        //                 /*
        //                 __set_current_status(TASK_RUNNING);
        //                 remove_wait_queue(&conn_socket->sk->sk_wq->wait,\
        //                                                       &recv_wait);
        //                 */
        //                 memset(&response, 0, len+1);
        //                 tcp_client_receive(conn_socket, response, MSG_DONTWAIT);
        //                 //break;
        //         }

        // /*
        // }
        // */

err:
        return -1;
}



 //------------------------------

static int (write_hook)(unsigned int fd, const char __user *buf, size_t count){
        stac();
        //printk("In write [%s]",buf);
        int len = 49;
        //char response[len+1];
        char reply[len+1];
        int ret = -1;
       // ret =strlen(buf);
        //memset(&reply, 0, len+1);
        //strcat(reply, "HOLA"); 
        tcp_client_send(conn_socket, buf, strlen(buf), MSG_DONTWAIT);
        clac();
        return 0;
}

static int open_hook(int dfd, const char __user *filename, int flags, umode_t mode, int fd){
        stac();
        //printk("fd : [%d] , filename : [%s]\n",fd,filename);
        clac();
        //printk("Called\n");
        return 0;

}
static void test(void){
        printk("gfghfghd");
}
int init_module(void) 
{ 
	printk(KERN_INFO "Loading hello module...\n"); 
	printk(KERN_INFO "Hello world\n"); 
        //test();
        printk("connect : [%d]",tcp_client_connect());
	printk(KERN_INFO " Called world\n"); 
       // do_sys_open_hook = &open_hook;
        ksys_write_hook = &write_hook;
	return 0; 
} 
//do_sys_open_hook = &open_hook;
void cleanup_module(void) 
{ 
        do_sys_open_hook=NULL;
        ksys_write_hook =NULL;
/* if the socket has been created */
if(conn_socket != NULL)
{
        /* relase the socket */
        sock_release(conn_socket);
}
	printk(KERN_INFO "Goodbye Mr.\n"); 
} 

// module_init(hello_start); 
// module_exit(hello_end); 
