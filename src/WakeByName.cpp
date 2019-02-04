// WakeByName.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
//#define IPv6
//#define WSASendTo

#pragma region Constants
const auto USAGE = _T("%s <name>\r\n");
const auto DNS_NOTFOUND = _T("Could not resolve: %s\r\n");
const USHORT PORT = 7U;
const DWORD DNS_OPTIONS = DNS_QUERY_STANDARD;
#ifdef IPv6
const WORD DNS_TYPES = DNS_TYPE_A | DNS_TYPE_AAAA;
#else
const WORD DNS_TYPES = DNS_TYPE_A;
#endif
const DWORD VERSION_WSA = MAKEWORD(2UL, 2UL);
const INT PAYLOAD_LEN = 102;
//const ULONG BROADCAST_FULL = -1L;
#ifndef WSASendTo
const ULONG MAC_LEN = 6UL;
#endif
#pragma endregion

typedef struct _NameEntry {
    SLIST_ENTRY Entry;
    _TCHAR* Value;
} NameEntry, *PNameEntry;

INT _tmain(INT argc, _TCHAR* argv[]) {
#pragma region Parameters
    auto err = EXIT_SUCCESS;
    PSLIST_HEADER pListTargets = NULL;
    PIP4_ARRAY pSrvList = NULL;
    auto port = PORT;
#ifdef _DEBUG
    SIZE_T bufferSize;
    _TCHAR* repr = NULL;
#endif
#pragma endregion
    if ((pListTargets = (PSLIST_HEADER)_aligned_malloc(sizeof(SLIST_HEADER), MEMORY_ALLOCATION_ALIGNMENT)) == NULL) {
        _ftprintf(stderr, _T("Memory allocation failed\r\n"));
        err = EXIT_FAILURE;
        goto END;
    }
    InitializeSListHead(pListTargets);
    while (optind < argc) {
        switch (getopt(argc, argv, _T("hn:p:"))) {
            case _T('n'):
                if (!pSrvList) {
                    if (!(pSrvList = (PIP4_ARRAY)LocalAlloc(LPTR, sizeof(IP4_ARRAY)))) {
                        _ftprintf(stderr, _T("Memory allocation failed\r\n"));
                        err = EXIT_FAILURE;
                        goto END;
                    }
                    pSrvList->AddrCount = 0UL;
                }
                pSrvList->AddrArray[pSrvList->AddrCount++] = _tstol(optarg);
                break;
            case _T('p'):
                port = _tstoi(optarg);
                break;
            case -1:
                PNameEntry item;
                if (!(item = (PNameEntry)_aligned_malloc(sizeof(NameEntry), MEMORY_ALLOCATION_ALIGNMENT))) {
                    _ftprintf(stderr, _T("Memory allocation failed\r\n"));
                    err = EXIT_FAILURE;
                    goto END;
                }
                item->Value = argv[optind++];
                InterlockedPushEntrySList(pListTargets, &item->Entry);
                break;
            case _T(':'):
                _ftprintf(stderr, _T("Option -%c requires an operand\r\n"), optopt);
                err = EXIT_FAILURE;
                goto END;
            case _T('?'):
                _ftprintf(stderr, _T("Unrecognized option: -%c\r\n"), optopt);
                err = EXIT_FAILURE;
                goto END;
            case _T('h'):
            default:
                auto name = new _TCHAR[_MAX_FNAME];
                _tsplitpath_s(argv[0], NULL, NULL, NULL, NULL, name, _MAX_FNAME, NULL, NULL);
                _tprintf(USAGE, name);
                err = EXIT_FAILURE;
                goto END;
        }
    }
    if (!QueryDepthSList(pListTargets)) {
        _tprintf(_T("Please provide a domain name\r\n"));
        err = EXIT_FAILURE;
        goto END;
    }
    //PMIB_IPADDRTABLE nicInfos = NULL;
    //DWORD infosSize;
    //if (GetIpAddrTable(nicInfos, &infosSize, FALSE) == ERROR_INSUFFICIENT_BUFFER) {
    //    free(nicInfos);
    //    nicInfos = (PMIB_IPADDRTABLE)malloc(infosSize);
    //}
    //if (err = GetIpAddrTable(nicInfos, &infosSize, FALSE)) {
    //    goto FREETABLE;
    //}
    WSADATA wsaData;
    if (err = WSAStartup(VERSION_WSA, &wsaData) != NO_ERROR)
        goto CLEANUP;
    SOCKET sock = INVALID_SOCKET;
    if ((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO::IPPROTO_UDP)) == INVALID_SOCKET)
        goto CLEANUP;
#ifndef WSASendTo
    IN_ADDR dest;
    SOCKADDR_IN src;
    src.sin_family = AF_INET;
    src.sin_port = port;
#endif
#ifdef IPv6
    SOCKET sock6 = INVALID_SOCKET;
    if ((sock6 = socket(AF_INET6, SOCK_DGRAM, IPPROTO::IPPROTO_UDP)) == INVALID_SOCKET)
        goto CLEANUP;
#ifndef WSASendTo
    IN6_ADDR dest6;
    SOCKADDR_IN6 src6;
    src6.sin6_family = AF_INET6;
    src6.sin6_port = port;
#else
    MIB_IPNET_ROW2 dest;
    SOCKADDR_INET src;
    src.si_family = AF_INET;
    src.Ipv4.sin_port = port;
    src.Ipv6.sin6_port = port;
#endif
#endif
#ifndef WSASendTo
    auto payload = new CHAR[PAYLOAD_LEN] { '\xFF', '\xFF', '\xFF', '\xFF', '\xFF', '\xFF' };
#else
    WSABUF payload;
    payload.len = PAYLOAD_LEN;
    payload.buf = new CHAR[PAYLOAD_LEN] { '\xFF', '\xFF', '\xFF', '\xFF', '\xFF', '\xFF' };
#endif
    for (auto item = &pListTargets->Next; item->Next; item = item->Next) {
        auto entry = (PNameEntry)item->Next;
        PDNS_RECORD results = NULL;
        if ((err = DnsQuery(entry->Value, DNS_TYPES, DNS_OPTIONS, pSrvList, &results, NULL)) || !results) {
            _tprintf(DNS_NOTFOUND, entry->Value);
            goto FREELIST;
        }
        for (auto item = results; item; item = item->pNext) {
            ADDRESS_FAMILY family;
            switch (item->wType) {
                case DNS_TYPE_A:
#ifndef WSASendTo
                    dest.s_addr = item->Data.A.IpAddress;
                    family = AF_INET;
#else
                    dest.Address.Ipv4.sin_addr.s_addr = item->Data.A.IpAddress;
                    dest.Address.si_family = AF_INET;
#endif
#ifdef _DEBUG
                    bufferSize = INET_ADDRSTRLEN;
#endif
                    break;
#ifdef IPv6
                case DNS_TYPE_AAAA:
#ifndef WSASendTo
                    *dest6.u.Byte = *item->Data.AAAA.Ip6Address.IP6Byte;
                    family = AF_INET6;
#else
                    *dest.Address.Ipv6.sin6_addr.u.Byte = *item->Data.AAAA.Ip6Address.IP6Byte;
                    dest.Address.si_family = AF_INET6;
#endif
#ifdef _DEBUG
                    bufferSize = INET6_ADDRSTRLEN;
#endif
                    break;
#endif
                default:
                    continue;
            }
#ifdef _DEBUG
            repr = new _TCHAR[bufferSize];
#ifndef WSASendTo
            if (InetNtop(family, &dest, repr, bufferSize))
#else
            if (InetNtop(dest.Address.si_family, &dest, repr, bufferSize))
#endif
                _tprintf(_T("Got IP Address: %s\r\n"), repr);
#endif
            //        DWORD nicIndex;
            //        if (GetBestInterfaceEx((SOCKADDR*)&dest.Address.Ipv4, &nicIndex) != NO_ERROR)
            //            continue;
            //#ifdef _DEBUG
            //        else
            //            _tprintf(_T("Will send on interface index: %i\r\n"), nicIndex);
            //#endif
            //        PMIB_IPADDRROW nic;
            //        for (auto entry = 0U; entry < nicInfos->dwNumEntries; ++entry) {
            //            if ((nic = &nicInfos->table[entry])->dwIndex == nicIndex) {
            //                break;
            //            }
            //        }
            //        src.Ipv4.sin_addr.s_addr = nic->dwMask ^ BROADCAST_FULL | nic->dwAddr;
            //#ifdef IPv6
            //        IN6_ADDR src6;
            //#endif
            //#ifdef _DEBUG
            //        if (InetNtop(dest.Address.si_family, &src.Ipv4.sin_addr, repr, bufferSize))
            //            _tprintf(_T("With Broadcast: %s\r\n"), repr);
            //#endif
#ifndef WSASendTo
            DWORD mac[2U];
            auto macLen = MAC_LEN;
            //if (SendARP(dest.s_addr, src.sin_addr.s_addr, &mac, &macLen) != NO_ERROR)
            if (SendARP(dest.s_addr, INADDR_ANY, &mac, &macLen) != NO_ERROR)
                continue;
#else
            if (ResolveIpNetEntry2(&dest, NULL) != NO_ERROR)
                continue;
#endif
#ifdef _DEBUG
            else {
#ifndef WSASendTo
                auto macArr = (PBYTE)&mac;
                _tprintf(_T("Targeting MAC: "));
                for (auto index = 0UL; index < macLen; ++index)
                    _tprintf(_T("%.2x-"), macArr[index]);
#else
                _tprintf(_T("Targeting MAC: "));
                for (auto index = 0UL; index < dest.PhysicalAddressLength; ++index)
                    _tprintf(_T("%.2x-"), dest.PhysicalAddress[index]);
                _tprintf(_T("\r\n"));
#endif
            }
#endif
#ifndef WSASendTo
            auto macArr = (PBYTE)&mac;
            for (auto index = macLen; index < PAYLOAD_LEN; ++index)
                payload[index] = macArr[index % macLen];
#else
            for (auto index = dest.PhysicalAddressLength; index < PAYLOAD_LEN; ++index)
                payload[index] = dest.PhysicalAddress[index % dest.PhysicalAddressLength];
#endif
            sendto(sock, payload, PAYLOAD_LEN, 0, (SOCKADDR*)&src, sizeof(src));
            //WSASendTo(sock, &payload, 1UL, NULL, NULL, (SOCKADDR*)&src.Ipv4, sizeof(src), NULL, NULL);
#ifdef IPv6
            sendto(sock6, payload, PAYLOAD_LEN, 0, (SOCKADDR*)&src, sizeof(src));
            //WSASendTo(sock, &payload, 1UL, NULL, NULL, (SOCKADDR*)&src.Ipv6, sizeof(src), NULL, NULL);
#endif
        }
    FREELIST:
        DnsRecordListFree(results, DNS_FREE_TYPE::DnsFreeRecordList);
    }
CLEANUP:
    if (sock != INVALID_SOCKET)
        closesocket(sock);
#ifdef IPv6
    if (sock6 != INVALID_SOCKET)
        closesocket(sock6);
#endif
    WSACleanup();
    //FREETABLE:
    //    free(nicInfos);
END:
    if (pSrvList)
        LocalFree(pSrvList);
    if (pListTargets) {
        InterlockedFlushSList(pListTargets);
        _aligned_free(pListTargets);
    }
    return err;
}