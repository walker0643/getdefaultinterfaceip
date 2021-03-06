// REQUIRES: Ws2_32.lib, Iphlpapi.lib
// REQUIRES: WSAStartup(..)

// see comments regarding WSAAddressToStringA in address_string(..)
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <WinSock2.h>
#include <iphlpapi.h>
#include <iostream>
#include <string>
#include "HeapResource.h"

namespace Debauchee
{

static void err(const std::string& msg)
{
    std::cerr << msg << std::endl;
}

static ULONG get_addresses(IP_ADAPTER_ADDRESSES * addresses, PULONG sz)
{
    return GetAdaptersAddresses(AF_INET, GAA_FLAG_SKIP_ANYCAST |GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER, NULL, addresses, sz);
}

static std::string address_string(const SOCKET_ADDRESS& address)
{
    // planning to use this with Qt and QString does not have a c'tor
    // for wide strings so we need the ANSI API here
    DWORD szStrAddress = 1023;
    char strAddress[1024];
    if (WSAAddressToStringA(address.lpSockaddr, address.iSockaddrLength, NULL, strAddress, &szStrAddress) != 0) {
        err("WSAAddressToStringA() failed");
        return "";
    }
    return strAddress;
}

static std::string default_ip_address(const IP_ADAPTER_ADDRESSES * next)
{
    std::string defaultAddress;
    std::string wirelessAddress;
    int idx = 1;
    while (next) {
        std::string firstAddress = "";
        std::cout << "Adapter " << idx << ":" << std::endl;
        std::cout << "  Name:" << next->AdapterName << std::endl;
        std::wcout << "  Description: " << next->Description << std::endl;
        auto unicast = next->FirstUnicastAddress;
        while (unicast) {
            auto address = address_string(unicast->Address);
            if (!address.empty()) {
                std::cout << "  Unicast Address: " << address << std::endl;
                if (firstAddress.empty()) {
                    firstAddress = address;
                }
            }
            unicast = unicast->Next;
        }

        if (firstAddress.empty()) {
            std::cout << "  No IP" << std::endl;
        }

        if (next->OperStatus == IfOperStatusUp) {
            std::cout << "  Up" << std::endl;
        } else {
            std::cout << "  Not Up (" << next->OperStatus << ")" << std::endl;
        }

        if (next->IfType == IF_TYPE_ETHERNET_CSMACD) {
            std::cout << "  Wired" << std::endl;
            if (!firstAddress.empty() && next->OperStatus == IfOperStatusUp) {
                // would return here, but keep going for testing
                if (defaultAddress.empty()) {
                    std::cout << "  Selecting as default" << std::endl;
                    defaultAddress = firstAddress;
                }
            }
        } else if (next->IfType == IF_TYPE_IEEE80211) {
            std::cout << "  Wireless" << std::endl;
            if (!firstAddress.empty() && wirelessAddress.empty() && next->OperStatus == IfOperStatusUp) {
                std::cout << "  Selecting as wireless" << std::endl;
                wirelessAddress = firstAddress;
            }
        } else if (next->IfType == IF_TYPE_SOFTWARE_LOOPBACK) {
            std::cout << "  Loopback" << std::endl;
        } else {
            std::cout << "  Unknown Interface Type" << std::endl;
        }

        ++idx;
        next = next->Next;
    }
    return defaultAddress.empty() ? wirelessAddress : defaultAddress;
}

std::string default_ip_address_verbose()
{
    std::string defaultAddress = "";
    const ULONG DefaultSzAddresses = 15 * 1024; // 15k recommended by msdn
    ULONG szAddresses = DefaultSzAddresses;
    HeapResource<IP_ADAPTER_ADDRESSES> addresses(GetProcessHeap(), 0, DefaultSzAddresses);
    if (!addresses.is_valid()) {
        err("HeapAlloc() failed");
    } else {
        ULONG result;
        while (ERROR_BUFFER_OVERFLOW == (result = get_addresses(addresses, &szAddresses))) {
            // add more space in case more adapters have shown up
            szAddresses += DefaultSzAddresses;
            swap(addresses, HeapResource<IP_ADAPTER_ADDRESSES>(GetProcessHeap(), 0, szAddresses));
            if (!addresses.is_valid()) {
                err("HeapAlloc() failed");
                break;
            }
        }
        if (result == ERROR_BUFFER_OVERFLOW) {
            // HeapAlloc() failed - already reported
        } if (result != ERROR_SUCCESS) {
            err("GetAdaptersAddresses() failed");
        } else {
            defaultAddress = default_ip_address(addresses);
        }
    }
    return defaultAddress;
}

}
