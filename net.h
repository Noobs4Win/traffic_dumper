#pragma once
#include <sys/socket.h>
#include <linux/if.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <linux/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <sys/ioctl.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>


//
struct eth_addr
{
    unsigned char addr[6];
};


//
static int get_interface_index_mac( const char* interface, int* index, eth_addr* mac, uint32_t* ip, uint32_t* mask )
{
    int sock = socket( AF_PACKET, SOCK_RAW, htons( ETH_P_ALL ) );
    if( sock < 0 )
        return -1;

    struct ifreq ifr;
    strncpy( ifr.ifr_name, interface, sizeof( ifr.ifr_name )-1 );
    ifr.ifr_name[ sizeof( ifr.ifr_name ) - 1 ] = '\0';
    if( ioctl( sock, SIOCGIFINDEX, &ifr ) == -1 )
    {
        close( sock );
        return -1;
    }
    *index = ifr.ifr_ifindex;

    if( ioctl( sock, SIOCGIFADDR, &ifr ) == -1 )
    {
        close( sock );
        return -1;
    }
    *ip = ( (struct sockaddr_in* )( &ifr.ifr_addr ) )->sin_addr.s_addr;

    if( ioctl( sock, SIOCGIFNETMASK, &ifr ) == -1 )
    {
        close( sock );
        return -1;
    }
    *mask = ( (struct sockaddr_in* )( &ifr.ifr_addr ) )->sin_addr.s_addr;


    if( ioctl( sock, SIOCGIFHWADDR, &ifr ) == -1 )
    {
        close( sock );
        return -1;
    }
    memcpy( ( void* )mac, ( void* )&ifr.ifr_hwaddr.sa_data, 6 );

    return 0;
}


//
static void macToStr( eth_addr* mac, char* mac_str)
{
    sprintf( mac_str, "%02X:%02X:%02X:%02X:%02X:%02X", mac->addr[0], mac->addr[1], mac->addr[2], mac->addr[3], mac->addr[4], mac->addr[5] );
}
