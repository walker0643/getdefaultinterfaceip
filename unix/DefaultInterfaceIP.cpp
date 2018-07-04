#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/wireless.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "SocketResource.h"
#include "IfAddrsResource.h"

namespace Debauchee
{

static bool is_wireless(const char * ifname)
{
    if (ifname) {
        SocketResource fd(AF_INET, SOCK_STREAM, 0);
        if (fd.is_valid()) {
            struct iwreq req { 0 };
            ::strncpy(req.ifr_name, ifname, IFNAMSIZ - 1);
            return ioctl(fd, SIOCGIWMODE, req) >= 0;
        }
    }
    return false;
}

std::string default_interface_ip()
{
    std::string wirelessAddress;
    IfAddrsResource ifa;
    if (ifa.is_valid()) {
        for (struct ifaddrs * next = ifa; next; next = next->ifa_next) {
            auto sain = (struct sockaddr_in *)next->ifa_addr;
            if (!sain || sain->sin_family != AF_INET ||
                !(next->ifa_flags & IFF_RUNNING) ||
                (next->ifa_flags & IFF_LOOPBACK)
               ) {
                continue;
            }
            std::string address = inet_ntoa(sain->sin_addr);
            // take first wired address right away
            if (!is_wireless(next->ifa_name)) {
                return address;
            }
            // save first wireless address to be used if we don't find a wired one
            if (wirelessAddress.empty()) {
                wirelessAddress = address;
            }
        }
    }
    return wirelessAddress;
}

}
