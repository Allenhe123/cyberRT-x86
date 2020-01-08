// Copyright 2016 Proyectos y Sistemas de Mantenimiento SL (eProsima).
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/**
 * @file IPFinder.cpp
 *
 */

#include <fastrtps/utils/IPFinder.h>

#if defined(_WIN32)
#include <stdio.h>
#include <winsock2.h>
#include <iphlpapi.h>
#include <ws2tcpip.h>
#include <assert.h>
#else
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#endif


namespace eprosima {
namespace fastrtps{

IPFinder::IPFinder() {


}

IPFinder::~IPFinder() {

}

#if defined(_WIN32)

#define DEFAULT_ADAPTER_ADDRESSES_SIZE 15360

bool IPFinder::getIPs(std::vector<info_IP>* vec_name, bool return_loopback)
{
    DWORD rv, size = DEFAULT_ADAPTER_ADDRESSES_SIZE;
    PIP_ADAPTER_ADDRESSES adapter_addresses, aa;
    PIP_ADAPTER_UNICAST_ADDRESS ua;

    adapter_addresses = (PIP_ADAPTER_ADDRESSES)malloc(DEFAULT_ADAPTER_ADDRESSES_SIZE);

    rv = GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_PREFIX, NULL, adapter_addresses, &size);

    if (rv != ERROR_SUCCESS)
    {
        adapter_addresses = (PIP_ADAPTER_ADDRESSES)realloc(adapter_addresses, size);

        rv = GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_PREFIX, NULL, adapter_addresses, &size);
    }

    if (rv != ERROR_SUCCESS)
    {
        fprintf(stderr, "GetAdaptersAddresses() failed...");
        free(adapter_addresses);
        return false;
    }

    for (aa = adapter_addresses; aa != NULL; aa = aa->Next) {
        if (aa->OperStatus == 1) //is ENABLED
        {
            for (ua = aa->FirstUnicastAddress; ua != NULL; ua = ua->Next) {
                char buf[BUFSIZ];

                int family = ua->Address.lpSockaddr->sa_family;

                if (family == AF_INET || family == AF_INET6) //IP4
                {
                    //printf("\t%s ",  family == AF_INET ? "IPv4":"IPv6");
                    memset(buf, 0, BUFSIZ);
                    getnameinfo(ua->Address.lpSockaddr, ua->Address.iSockaddrLength, buf, sizeof(buf), NULL, 0, NI_NUMERICHOST);
                    info_IP info;
                    info.type = family == AF_INET ? IP4 : IP6;
                    info.name = std::string(buf);

                    // Currently not supported interfaces that not support multicast.
                    if(aa->Flags & 0x0010)
                        continue;

                    if (info.type == IP4)
                        parseIP4(info);
                    else if (info.type == IP6)
                        parseIP6(info);
                    if (info.type == IP6 || info.type == IP6_LOCAL)
                    {
                        sockaddr_in6* so = (sockaddr_in6*)ua->Address.lpSockaddr;
                        info.scope_id = so->sin6_scope_id;
                    }

                    if(return_loopback || (info.type != IP6_LOCAL && info.type != IP4_LOCAL))
                        vec_name->push_back(info);
                    //printf("Buffer: %s\n", buf);
                }
            }
        }
    }

    free(adapter_addresses);
    return true;
}

#else

bool IPFinder::getIPs(std::vector<info_IP>* vec_name, bool return_loopback)
{
    struct ifaddrs *ifaddr, *ifa;
    int family, s;
    char host[NI_MAXHOST];

    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        exit(EXIT_FAILURE);
    }

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
    {
        if (ifa->ifa_addr == NULL)
            continue;

        family = ifa->ifa_addr->sa_family;

        if (family == AF_INET)
        {
            s = getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in),
                    host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
            if (s != 0) {
                printf("getnameinfo() failed: %s\n", gai_strerror(s));
                freeifaddrs(ifaddr);
                exit(EXIT_FAILURE);
            }
            info_IP info;
            info.type = IP4;
            info.name = std::string(host);
            parseIP4(info);

            if (return_loopback || info.type != IP4_LOCAL)
                vec_name->push_back(info);
        }
        else if(family == AF_INET6)
        {
            s = getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in6),
                    host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
            if (s != 0) {
                printf("getnameinfo() failed: %s\n", gai_strerror(s));
                freeifaddrs(ifaddr);
                exit(EXIT_FAILURE);
            }
            struct sockaddr_in6 * so = (struct sockaddr_in6 *)ifa->ifa_addr;
            info_IP info;
            info.type = IP6;
            info.name = std::string(host);
            if(parseIP6(info))
            {
                info.scope_id = so->sin6_scope_id;

                if (return_loopback || info.type != IP6_LOCAL)
                    vec_name->push_back(info);
            }
            //printf("<Interface>: %s \t <Address> %s\n", ifa->ifa_name, host);
        }
    }

    freeifaddrs(ifaddr);
    return true;
}
#endif

bool IPFinder::getIP4Address(LocatorList_t* locators)
{
    std::vector<info_IP> ip_names;
    if(IPFinder::getIPs(&ip_names))
    {

        locators->clear();
        for(auto it=ip_names.begin();
                it!=ip_names.end();++it)
        {
            if (it->type == IP4)
            {
                locators->push_back(it->locator);
            }
        }
        return true;
    }
    return false;
}

bool IPFinder::getAllIPAddress(LocatorList_t* locators)
{
    std::vector<info_IP> ip_names;
    if (IPFinder::getIPs(&ip_names))
    {
        locators->clear();
        for (auto it = ip_names.begin();
                it != ip_names.end(); ++it)
        {
            if (it->type == IP6)
            {
                locators->push_back(it->locator);
            }
            else if (it->type == IP4)
            {
                locators->push_back(it->locator);
            }
        }
        return true;
    }
    return false;
}

bool IPFinder::getIP6Address(LocatorList_t* locators)
{
    std::vector<info_IP> ip_names;
    if (IPFinder::getIPs(&ip_names))
    {

        locators->clear();
        for (auto it = ip_names.begin();
                it != ip_names.end(); ++it)
        {
            if (it->type == IP6)
            {
                locators->push_back(it->locator);
            }
        }
        return true;
    }
    return false;
}

RTPS_DllAPI bool IPFinder::parseIP4(info_IP& info)
{
    std::stringstream ss(info.name);
    int a, b, c, d;
    char ch;
    ss >> a >> ch >> b >> ch >> c >> ch >> d;
    //TODO Property to activate or deactivate the loopback interface.
    if (a == 127 && b == 0 && c == 0 && d == 1)
        info.type = IP4_LOCAL;
    //		if(a==169 && b==254)
    //			continue;
    info.locator.kind = 1;
    info.locator.port = 0;
    for (int8_t i = 0; i < 12; ++i)
        info.locator.address[i] = 0;
    info.locator.address[12] = (octet)a;
    info.locator.address[13] = (octet)b;
    info.locator.address[14] = (octet)c;
    info.locator.address[15] = (octet)d;
    return true;
}
RTPS_DllAPI bool IPFinder::parseIP6(info_IP& info)
{
    std::vector<std::string> hexdigits;

    size_t start = 0, end = 0;
    std::string auxstr;

    while(end != std::string::npos)
    {
        end = info.name.find(':',start);
        if (end - start > 1)
        {
            hexdigits.push_back(info.name.substr(start, end - start));
        }
        else
            hexdigits.push_back(std::string("EMPTY"));
        start = end + 1;
    }

    if ((hexdigits.end() - 1)->find('.') != std::string::npos) //FOUND a . in the last element (MAP TO IP4 address)
        return false;

    if(*hexdigits.begin() == std::string("EMPTY") && *(hexdigits.begin() + 1) == std::string("EMPTY"))
        info.type = IP6_LOCAL;

    for (int8_t i = 0; i < 2; ++i)
        info.locator.address[i] = 0;
    info.locator.kind = LOCATOR_KIND_UDPv6;
    info.locator.port = 0;
    *(hexdigits.end() - 1) = (hexdigits.end() - 1)->substr(0, (hexdigits.end() - 1)->find('%'));

    int auxnumber = 0;
    uint8_t index= 15;
    for (auto it = hexdigits.rbegin(); it != hexdigits.rend(); ++it)
    {
        if (*it != std::string("EMPTY"))
        {
            if (it->length() <= 2)
            {
                info.locator.address[index - 1] = 0;
                std::stringstream ss;
                ss << std::hex << (*it);
                ss >> auxnumber;
                info.locator.address[index] = (octet)auxnumber;
            }
            else
            {
                std::stringstream ss;
                ss << std::hex << it->substr(it->length()-2);
                ss >> auxnumber;
                info.locator.address[index] = (octet)auxnumber;
                ss.str("");
                ss.clear();
                ss << std::hex << it->substr(0, it->length() - 2);
                ss >> auxnumber;
                info.locator.address[index - 1] = (octet)auxnumber;
            }
            index -= 2;
        }
        else
            break;
    }
    index = 0;
    for (auto it = hexdigits.begin(); it != hexdigits.end(); ++it)
    {
        if (*it != std::string("EMPTY"))
        {
            if (it->length() <= 2)
            {
                info.locator.address[index] = 0;
                std::stringstream ss;
                ss << std::hex << (*it);
                ss >> auxnumber;
                info.locator.address[index + 1]=(octet)auxnumber;
            }
            else
            {
                std::stringstream ss;
                ss << std::hex << it->substr(it->length() - 2);
                ss >> auxnumber;
                info.locator.address[index + 1] = (octet)auxnumber;
                ss.str("");
                ss.clear();
                ss << std::hex << it->substr(0, it->length() - 2);
                ss >> auxnumber;
                info.locator.address[index] =  (octet)auxnumber;
            }
            index += 2;
        }
        else
            break;
    }
    /*
       cout << "IPSTRING: ";
       for (auto it : hexdigits)
       cout << it << " ";
       cout << endl;
       cout << "LOCATOR: " << *loc << endl;
       */
    return true;
}

}
} /* namespace eprosima */
