#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/stat.h>

#include "net.h"

const int MAX_PORTS_NUM = 1024;
const char* DUMP_DIR = "dump";


struct Port
{
    uint16_t    port;
    bool        is_tcp;
    FILE*       file;

    Port()
    {
        file = NULL;
    }
    ~Port()
    {
        if( file )
            fclose( file );
    }
};


//
int ReadConfigFile( const char* cfgFile, Port* ports )
{
    FILE* file;
    char buf[ 1024 ];

    file = fopen( cfgFile, "r" );
    if( !file )
        return -1;

    int portsNum = 0;
    while( fgets( buf, 1024, file ) != NULL )
    {
        ports[ portsNum ].is_tcp = strncmp( buf, "tcp", 3 ) == 0;
        ports[ portsNum ].port = atoi( buf + 3 );

        char filename[ 256 ];
        sprintf( filename, "%s/port%u.dmp", DUMP_DIR, ports[ portsNum ].port );
        ports[ portsNum ].file = fopen( filename, "a" );

        portsNum++;
        if( portsNum == MAX_PORTS_NUM )
            break;
    }

    fclose( file );

    return portsNum;
}


//
void SortPorts( Port* ports, int portsNum )
{
    // do nothing
}


//
Port* FindPort( Port* ports, int portsNum, uint16_t port, bool is_tcp )
{
    for( int i = 0; i < portsNum; i++ )
        if( ports[ i ].port == port && ports[ i ].is_tcp == is_tcp )
            return &ports[ i ];

    return NULL;
}


//
int main( int argc, char* argv[] )
{
    if( argc != 3 )
    {
        printf( "./traffic_dumper <iface> <cfgFile>\n" );
        return 0;
    }

    const char* iface = argv[ 1 ];
    const char* cfgFile = argv[ 2 ];
    int iface_index;
    struct eth_addr iface_mac;
    uint32_t iface_ip;
    uint32_t iface_mask;
    if( get_interface_index_mac( iface, &iface_index, &iface_mac, &iface_ip, &iface_mask ) != 0 )
    {
        printf( "Maybe root?\n" );
        return -1;
    }

    Port ports[ MAX_PORTS_NUM ];
    mkdir( DUMP_DIR, 0x777 );
    int portsNum = ReadConfigFile( cfgFile, ports );
    SortPorts( ports, portsNum );

    int sock = socket( AF_PACKET, SOCK_RAW, htons( ETH_P_ALL ) );
    if( sock == -1 )
    {
        perror("socket: ");
        return -1;
    }
    struct sockaddr_ll saddr;
    bzero( &saddr, sizeof( sockaddr_ll ) );
    saddr.sll_family   = AF_PACKET;
    saddr.sll_protocol = htons( ETH_P_ALL );
    saddr.sll_ifindex  = iface_index;

    if( bind( sock, ( sockaddr* )( &saddr ), sizeof( saddr ) ) < 0 )
    {
        perror( "bind error" );
        return -1;
    }

    while( 1 )
    {
        char buf[ ETH_FRAME_LEN ];
        char* ptr = buf;
        int received = recv( sock, buf, ETH_FRAME_LEN, 0 );
        if( received < sizeof( ethhdr ) )
            continue;

        ethhdr* eth = ( ethhdr* )ptr;   ptr += sizeof( ethhdr );
        if( eth->h_proto != htons( ETH_P_IP ) )
            continue;

        iphdr* ip = ( iphdr* )ptr;      ptr += sizeof( iphdr );

        uint32_t saddr = ip->saddr;
        uint32_t daddr = ip->daddr;
        bool is_tcp = ip->protocol == IPPROTO_TCP;
        uint16_t sport = 0;
        uint16_t dport = 0;
        tcphdr* tcp = NULL;
        udphdr* udp = NULL;

        if( ip->protocol == IPPROTO_TCP )
        {
            tcp = ( tcphdr* )ptr;
            ptr += tcp->th_off * 4;
            sport = ntohs( tcp->source );
            dport = ntohs( tcp->dest );
        }
        else if( ip->protocol == IPPROTO_UDP )
        {
            udp = ( udphdr* )ptr;       ptr += sizeof( udphdr );
            sport = ntohs( udp->source );
            dport = ntohs( udp->dest );
        }
        else
        {
            continue;
        }

        Port* p = NULL;
        if( saddr == iface_ip )
            p = FindPort( ports, portsNum, sport, is_tcp );
        if( daddr == iface_ip )
            p = FindPort( ports, portsNum, dport, is_tcp );
        if( !p )
            continue;

        int size = received -( ptr - buf );
        if( size > 0 )
        {
            fwrite( ptr, size, 1, p->file );
            fflush( p->file );
        }
    }

    return 0;
}

