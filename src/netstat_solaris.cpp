#include <iomanip>
#include <iostream>
#include <stdint.h>
#include <cstdlib>
#include <string>
#include <sstream>
#include <vector>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <unistd.h>
#include <stropts.h>

#include <cstring>

#include <sys/tihdr.h>
#include <inet/mib2.h>

class buffer_t
{
	public:
		char buffer[512];
		strbuf buf;

		buffer_t()
		{
			memset(buffer,0,512);
			buf.buf=(char*)buffer;
			buf.len=512;
		}
};

struct request_t
{
	T_optmgmt_req req_header;
	opthdr opt_header;
};

struct reply_t
{
	T_optmgmt_ack ack_header;
	opthdr opt_header;
};

std::string uint32_t_to_ipv4(const uint32_t address)
{
        std::ostringstream ostr;
        ostr<<(uint32_t)((uint8_t*)&address)[0]<<"."<<
                (uint32_t)((uint8_t*)&address)[1]<<"."<<
                (uint32_t)((uint8_t*)&address)[2]<<"."<<
                (uint32_t)((uint8_t*)&address)[3];
        return ostr.str();
}

std::string uint8_t_16_to_ipv6(const uint8_t address[16])
{
        std::ostringstream ostr;
        for(int ii=0;ii<16;ii+=2)
        {
                ostr<<std::hex<<std::setw(2)<<std::setfill('0')<<(unsigned int)(unsigned char)address[ii+0];
                ostr<<std::hex<<std::setw(2)<<std::setfill('0')<<(unsigned int)(unsigned char)address[ii+1];

                if(ii<14)
                        ostr<<":";
        }

        return ostr.str();
}

std::string dword_to_port(const uint32_t port)
{
        std::ostringstream ostr;
        ostr<<((((uint32_t)((uint8_t*)&port)[0])<<8)+((uint8_t*)&port)[1]);
        return ostr.str();
}

std::string to_string(const uint32_t val)
{
        std::ostringstream ostr;
        ostr<<val;
        return ostr.str();
}

std::string state_int_to_string(const uint32_t state)
{
	if(state==MIB2_TCP_established)
		return "ESTABLISHED";
	if(state==MIB2_TCP_synSent)
		return "SYN_SENT";
	if(state==MIB2_TCP_synReceived)
		return "SYN_RECV";
	if(state==MIB2_TCP_finWait1)
		return "FIN_WAIT1";
	if(state==MIB2_TCP_finWait2)
		return "FIN_WAIT2";
	if(state==MIB2_TCP_timeWait)
		return "TIME_WAIT";
	if(state==MIB2_TCP_closed)
		return "CLOSE";
	if(state==MIB2_TCP_closeWait)
		return "CLOSE_WAIT";
	if(state==MIB2_TCP_lastAck)
		return "LAST_ACK";
	if(state==MIB2_TCP_listen)
		return "LISTEN";
	if(state==MIB2_TCP_closing||state==MIB2_TCP_deleteTCB)
		return "CLOSING";
	
	return "UNKNOWN";
}

struct netstat_t
{
        std::string proto;
        std::string local_address;
        std::string foreign_address;
        std::string local_port;
        std::string foreign_port;
        std::string state;
        std::string inode;
        std::string pid;
};

typedef std::vector<netstat_t> netstat_list_t;

void netstat_print(const netstat_t& netstat)
{
        std::cout<<
                std::setw(4)<<netstat.proto<<" "<<
                std::setw(64)<<netstat.local_address+":"+netstat.local_port<<" "<<
                std::setw(64)<<netstat.foreign_address+":"+netstat.foreign_port<<" "<<
                std::setw(16)<<netstat.state<<" "<<
                std::setw(8)<<netstat.pid<<" "<<
                std::endl;
}

void netstat_list_print(const netstat_list_t& netstats)
{
        std::cout<<
                std::setw(4)<<"proto "<<
                std::setw(64)<<"local_address "<<
                std::setw(64)<<"foreign_address "<<
                std::setw(16)<<"state "<<
                std::setw(8)<<"pid "<<
                std::endl;

        for(size_t ii=0;ii<netstats.size();++ii)
                netstat_print(netstats[ii]);
}

int main()
{
	int fd=open("/dev/arp",O_RDWR);

	if(fd==-1)
		return 1;

	if(ioctl(fd,I_PUSH,"tcp")==-1)
		return 1;

	if(ioctl(fd,I_PUSH,"udp")==-1)
		return 1;

	request_t request;
	request.req_header.PRIM_type=T_SVR4_OPTMGMT_REQ;
	request.req_header.OPT_length=sizeof(request.opt_header);
	request.req_header.OPT_offset=(long)offsetof(request_t,opt_header);
	request.req_header.MGMT_flags=T_CURRENT;
	request.opt_header.level=MIB2_IP;
	request.opt_header.name=0;
	request.opt_header.len=0;

	strbuf buf;
	buf.len=sizeof(request);
	buf.buf=(char*)&request;

	if(putmsg(fd,&buf,NULL,0)<0)
		return 1;

	netstat_list_t tcp4;
	netstat_list_t tcp6;
	netstat_list_t udp4;
	netstat_list_t udp6;

	while(true)
	{
		strbuf buf2;
		void* data=NULL;
		int flags=0;
		reply_t reply;
		buf2.maxlen=sizeof(reply);
		buf2.buf=(char*)&reply;
		int ret=getmsg(fd,&buf2,NULL,&flags);

		if(ret<0)
		{
			std::cout<<"ret<0"<<std::endl;
			return 1;
		}
		if(ret!=MOREDATA)
		{
			std::cout<<"moredata"<<std::endl;
			break;
		}
		if(reply.ack_header.PRIM_type!=T_OPTMGMT_ACK)
		{
			std::cout<<"primtype"<<std::endl;
			return 1;
		}
		if((unsigned int)buf2.len<sizeof(reply.ack_header))
		{
			std::cout<<"buf2len"<<std::endl;
			return 1;
		}
		if((unsigned int)reply.ack_header.OPT_length<sizeof(reply.opt_header))
		{
			std::cout<<"optlength"<<std::endl;
			return 1;
		}

		data=malloc(reply.opt_header.len);

		if(data==NULL)
			return 1;

		buf2.maxlen=reply.opt_header.len;
		buf2.buf=(char*)data;
		flags=0;

		if(getmsg(fd,NULL,&buf2,&flags)>=0)
		{
			if(reply.opt_header.level==MIB2_TCP&&reply.opt_header.name==MIB2_TCP_CONN)
			{
				for(mib2_tcpConnEntry_t* entry=(mib2_tcpConnEntry_t*)data;(char*)(entry+1)<=(char*)data+buf2.len;++entry)
				{
					netstat_t netstat;
					netstat.proto="tcp4";
					netstat.local_address=uint32_t_to_ipv4(entry->tcpConnLocalAddress);
					netstat.foreign_address=uint32_t_to_ipv4(entry->tcpConnRemAddress);
					netstat.local_port=dword_to_port(htons(entry->tcpConnLocalPort));
					netstat.foreign_port=dword_to_port(htons(entry->tcpConnRemPort));
					netstat.state=state_int_to_string(entry->tcpConnState);
					netstat.pid="-";;
					tcp4.push_back(netstat);
				}
			}

			if(reply.opt_header.level==MIB2_TCP6&&reply.opt_header.name==MIB2_TCP6_CONN)
			{
				for(mib2_tcp6ConnEntry_t* entry=(mib2_tcp6ConnEntry_t*)data;(char*)(entry+1)<=(char*)data+buf2.len;++entry)
				{
					netstat_t netstat;
					netstat.proto="tcp6";
					netstat.local_address=uint8_t_16_to_ipv6(entry->tcp6ConnLocalAddress.s6_addr);
					netstat.foreign_address=uint8_t_16_to_ipv6(entry->tcp6ConnRemAddress.s6_addr);
					netstat.local_port=dword_to_port(htons(entry->tcp6ConnLocalPort));
					netstat.foreign_port=dword_to_port(htons(entry->tcp6ConnRemPort));
					netstat.state=state_int_to_string(entry->tcp6ConnState);
					netstat.pid="-";
					tcp6.push_back(netstat);
				}
			}

			if(reply.opt_header.level==MIB2_UDP&&reply.opt_header.name==MIB2_UDP_ENTRY)
			{
				for(mib2_udpEntry_t* entry=(mib2_udpEntry_t*)data;(char*)(entry+1)<=(char*)data+buf2.len;++entry)
				{
					netstat_t netstat;
					netstat.proto="udp4";
					netstat.local_address=uint32_t_to_ipv4(entry->udpLocalAddress);
					netstat.foreign_address="0.0.0.0";
					netstat.local_port=dword_to_port(htons(entry->udpLocalPort));
					netstat.foreign_port="0";
					netstat.state="-";
					netstat.pid="-";
					udp4.push_back(netstat);
				}
			}

			if(reply.opt_header.level==MIB2_UDP6&&reply.opt_header.name==MIB2_UDP6_ENTRY)
			{
				for(mib2_udp6Entry_t* entry=(mib2_udp6Entry_t*)data;(char*)(entry+1)<=(char*)data+buf2.len;++entry)
				{
					netstat_t netstat;
					netstat.proto="udp6";
					netstat.local_address=uint8_t_16_to_ipv6(entry->udp6LocalAddress.s6_addr);
					netstat.foreign_address="0000:0000:0000:0000:0000:0000:0000:0000";
					netstat.local_port=dword_to_port(htons(entry->udp6LocalPort));
					netstat.foreign_port="0";
					netstat.state="-";
					netstat.pid="-";
					udp6.push_back(netstat);
				}
			}
		}

		free(data);
	}

	netstat_list_t netstats;

        for(size_t ii=0;ii<tcp4.size();++ii)
                netstats.push_back(tcp4[ii]);
        for(size_t ii=0;ii<tcp6.size();++ii)
                netstats.push_back(tcp6[ii]);
        for(size_t ii=0;ii<udp4.size();++ii)
                netstats.push_back(udp4[ii]);
        for(size_t ii=0;ii<udp6.size();++ii)
                netstats.push_back(udp6[ii]);

	netstat_list_print(netstats);

	return 0;
}