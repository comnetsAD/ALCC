/*****************************************************
 * This code was compiled and tested on Ubuntu 18.04.1
 * with kernel version 4.15.0
 *****************************************************/

/*
sudo insmod alcc_kernel.ko
dmesg
sudo rmmod alcc_kernel
*/

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/udp.h>
#include <net/netlink.h>
#include <net/net_namespace.h>

MODULE_LICENSE("GPL");

/* Protocol family, consistent in both kernel prog and user prog. */
#define MYPROTO NETLINK_USERSOCK
/* Multicast group, consistent in both kernel prog and user prog. */
#define MYGRP 21
#define CC_PORT 60001

#define NETLINK_TEST  17

static struct sock *nl_sk = NULL;
static struct nf_hook_ops *nfho = NULL;
static struct nf_hook_ops *nfho2 = NULL;

static void send_to_alcc(u16 sport, u16 dport, u32 seq, u32 ack_seq)
{
	struct sk_buff *skb;
	struct nlmsghdr *nlh;

	char msg[256];
	sprintf(msg, "%u %u %lu %lu", (unsigned) sport, (unsigned) dport, (unsigned long) seq, (unsigned long) ack_seq);

	int msg_size = 257; 

	// printk(KERN_INFO "MSG %s %d", msg, msg_size);

	int res;

	// printk(KERN_INFO "Netfilter TCP: %u; %u; %lu; %lu \n", sport, dport, (unsigned long)seq, (unsigned long)ack_seq); 

	// pr_info("Creating skb.\n");
	skb = nlmsg_new(NLMSG_ALIGN(msg_size + 1), GFP_KERNEL);
	if (!skb) {
		pr_err("Allocation failure.\n");
		return;
	}

	nlh = nlmsg_put(skb, 0, 1, NLMSG_DONE, msg_size + 1, 0);

	strcpy(nlmsg_data(nlh), msg);

	// pr_info("Sending skb.\n");
	res = nlmsg_multicast(nl_sk, skb, 0, NETLINK_TEST, GFP_KERNEL);
	if (res < 0)
		pr_info("nlmsg_multicast() error: %d\n", res);
	// else
	//	pr_info("Success.\n");
}

static unsigned int hfunc(void *priv, struct sk_buff *skb, const struct nf_hook_state *state)
{
	struct iphdr *iph;
	struct tcphdr *tcp_header;
	u16 sport, dport;           /* Source and destination ports */
	u32 seq, ack_seq;           /* Source and destination ports */
	
	if (!skb)
		return NF_ACCEPT;

	iph = ip_hdr(skb);

	if (iph->protocol == IPPROTO_TCP) {
		//tcp_header = (struct tcphdr *)(skb_transport_header(skb)+20); //Note: +20 is only for incoming packets
		//tcp_header= (struct tcphdr *)((__u32 *)iph+ iph->ihl);
		tcp_header = tcp_hdr(skb);

		sport = ntohs(tcp_header->source);
    	dport = ntohs(tcp_header->dest);
    	seq = ntohl(tcp_header->seq);
    	ack_seq = ntohl(tcp_header->ack_seq);

		// printk(KERN_INFO "Netfilter ALCC: %d; %d; %u; %u \n", sport, dport, seq, ack_seq); //, tcp_header->seq, tcp_header->ack_seq); 

		if (dport == CC_PORT || sport == CC_PORT) {
			send_to_alcc(sport, dport, seq, ack_seq);
		}

		return NF_ACCEPT;
	}

	return NF_ACCEPT;
}

static unsigned int hfunc2(void *priv, struct sk_buff *skb, const struct nf_hook_state *state)
{
	struct iphdr *iph;
	struct tcphdr *tcp_header;
	u16 sport, dport;           /* Source and destination ports */
	u32 seq, ack_seq;           /* Source and destination ports */
	
	if (!skb)
		return NF_ACCEPT;

	iph = ip_hdr(skb);

	if (iph->protocol == IPPROTO_TCP) {
		//tcp_header = (struct tcphdr *)(skb_transport_header(skb)+20); //Note: +20 is only for incoming packets
		//tcp_header= (struct tcphdr *)((__u32 *)iph+ iph->ihl);
		tcp_header = tcp_hdr(skb);

		sport = ntohs(tcp_header->source);
    	dport = ntohs(tcp_header->dest);
    	seq = ntohl(tcp_header->seq);
    	ack_seq = ntohl(tcp_header->ack_seq);

		// printk(KERN_INFO "Netfilter ALCC: %d; %d; %u; %u \n", sport, dport, seq, ack_seq); //, tcp_header->seq, tcp_header->ack_seq); 

    	if (dport == CC_PORT || sport == CC_PORT) {
			send_to_alcc(sport, dport, seq, ack_seq);
		}

		return NF_ACCEPT;
	}

	return NF_ACCEPT;
}

static int __init LKM_init(void)
{

	nl_sk = netlink_kernel_create(&init_net, MYPROTO, NULL);
	if (!nl_sk) {
		pr_err("Error creating socket.\n");
		return -10;
	}

	nfho = (struct nf_hook_ops*)kcalloc(1, sizeof(struct nf_hook_ops), GFP_KERNEL);

	/* Initialize netfilter hook */
	nfho->hook 	= (nf_hookfn*)hfunc;		/* hook function */
	nfho->hooknum 	= NF_INET_PRE_ROUTING;		/* received packets */
	nfho->pf 	= PF_INET;			/* IPv4 */
	nfho->priority 	= NF_IP_PRI_FIRST;		/* max hook priority */

	nf_register_net_hook(&init_net, nfho);

	nfho2 = (struct nf_hook_ops*)kcalloc(1, sizeof(struct nf_hook_ops), GFP_KERNEL);

	/* Initialize netfilter hook */
	nfho2->hook 	= (nf_hookfn*)hfunc2;		/* hook function */
	nfho2->hooknum 	= NF_INET_POST_ROUTING;		/* received packets */
	nfho2->pf 	= PF_INET;			/* IPv4 */
	nfho2->priority 	= NF_IP_PRI_FIRST;		/* max hook priority */

	nf_register_net_hook(&init_net, nfho2);

	return 0;
}

static void __exit LKM_exit(void)
{
	netlink_kernel_release(nl_sk);

	nf_unregister_net_hook(&init_net, nfho);
	kfree(nfho);
	nf_unregister_net_hook(&init_net, nfho2);
	kfree(nfho2);
}

module_init(LKM_init);
module_exit(LKM_exit);
