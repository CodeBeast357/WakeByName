// WakeByName.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
//#define IPv6
//#define WSASendTo

#pragma region Constants
const auto USAGE = _T("%s <name>\r\n");
const auto DNS_NOTFOUND = _T("Could not resolve: %s\r\n");
const auto MALLOC_FAILED = _T("Memory allocation failed\r\n");
const auto OP_MISSING = _T("Option -%c requires an operand\r\n");
const auto OP_UNRECOGNIZED = _T("Unrecognized option: -%c\r\n");
const auto DOMAIN_NAME_MISSING = _T("Please provide a domain name\r\n");
const auto SOCKET_ERR = _T("Could not open socket\r\n");
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
const ULONG MAC_LEN = 6UL;
#pragma endregion

typedef struct _NameEntry {
    SLIST_ENTRY Entry;
    _TCHAR* Value;
} NameEntry, *PNameEntry;

void printError(DWORD err)
{
    CString errorText;
    LPTSTR errorBuffer;
    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        err,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR)&errorBuffer,
        0,
        NULL);
    errorText = errorBuffer;
    LocalFree(errorBuffer);
    _ftprintf(stderr, errorText);
}

INT _tmain(INT argc, _TCHAR* argv[]) {
#pragma region Parameters
    auto err = EXIT_SUCCESS;
    PSLIST_HEADER pListTargets = NULL;
    PIP4_ARRAY pSrvList = NULL;
    auto port = PORT;
    auto subnet = false;
#ifdef _DEBUG
    SIZE_T bufferSize;
    _TCHAR* repr = NULL;
#endif
#pragma endregion
    if ((pListTargets = (PSLIST_HEADER)_aligned_malloc(sizeof(SLIST_HEADER), MEMORY_ALLOCATION_ALIGNMENT)) == NULL) {
        _ftprintf(stderr, MALLOC_FAILED);
        err = EXIT_FAILURE;
        goto END;
    }
    InitializeSListHead(pListTargets);
    while (optind < argc) {
        switch (getopt(argc, argv, _T("hn:p:s"))) {
            case _T('n'):
                if (!pSrvList) {
                    if (!(pSrvList = (PIP4_ARRAY)LocalAlloc(LPTR, sizeof(IP4_ARRAY)))) {
                        _ftprintf(stderr, MALLOC_FAILED);
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
                    _ftprintf(stderr, MALLOC_FAILED);
                    err = EXIT_FAILURE;
                    goto END;
                }
                item->Value = argv[optind++];
                InterlockedPushEntrySList(pListTargets, &item->Entry);
                break;
            case _T('s'):
                subnet = true;
                break;
            case _T(':'):
                _ftprintf(stderr, OP_MISSING, optopt);
                err = EXIT_FAILURE;
                goto END;
            case _T('?'):
                _ftprintf(stderr, OP_UNRECOGNIZED, optopt);
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
        _tprintf(DOMAIN_NAME_MISSING);
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
    if (err = WSAStartup(VERSION_WSA, &wsaData) != NO_ERROR) {
        printError(err);
        goto CLEANUP;
    }
    SOCKET sock = INVALID_SOCKET;
    if ((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO::IPPROTO_UDP)) == INVALID_SOCKET) {
        _tprintf(SOCKET_ERR);
        err = EXIT_FAILURE;
        goto CLEANUP;
    }
#ifndef _DEBUG
    IPAddr dest;
#else
    IN_ADDR dest;
#endif
    SOCKADDR_IN src;
    src.sin_family = AF_INET;
    src.sin_port = port;
#ifdef IPv6
    SOCKET sock6 = INVALID_SOCKET;
    if ((sock6 = socket(AF_INET6, SOCK_DGRAM, IPPROTO::IPPROTO_UDP)) == INVALID_SOCKET)
    {
        _tprintf(SOCKET_ERR);
        err = EXIT_FAILURE;
        goto CLEANUP;
    }
    IN6_ADDR dest6;
    SOCKADDR_IN6 src6;
    src6.sin6_family = AF_INET6;
    src6.sin6_port = port;
#endif
    for (auto targetItem = InterlockedPopEntrySList(pListTargets); targetItem; targetItem = InterlockedPopEntrySList(pListTargets)) {
        auto entry = (PNameEntry)targetItem;
        PDNS_RECORD results = NULL;
        DNS_STATUS errDns;
        if (errDns = DnsQuery(entry->Value, DNS_TYPES, DNS_OPTIONS, pSrvList, &results, NULL)) {
            printError(errDns);
            goto FREELIST;
        }
        if (!results) {
            _tprintf(DNS_NOTFOUND, entry->Value);
            goto FREELIST;
        }
        for (auto resultItem = results; resultItem; resultItem = resultItem->pNext) {
            ADDRESS_FAMILY family;
            switch (resultItem->wType) {
                case DNS_TYPE_A:
#ifndef _DEBUG
                    dest = resultItem->Data.A.IpAddress;
#else
                    dest.s_addr = resultItem->Data.A.IpAddress;
#endif
                    family = AF_INET;
#ifdef _DEBUG
                    bufferSize = INET_ADDRSTRLEN;
#endif
                    break;
#ifdef IPv6
                case DNS_TYPE_AAAA:
                    *dest6.u.Byte = *resultItem->Data.AAAA.Ip6Address.IP6Byte;
                    family = AF_INET6;
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
            if (InetNtop(family, &dest, repr, bufferSize))
                _tprintf(_T("Got IP Address: %s\r\n"), repr);
#endif
            src.sin_addr.s_addr = INADDR_BROADCAST;
            DWORD errIphlp;
            if (subnet)
            {
                DWORD nicIndex;
                MIB_IPFORWARDROW bestRoute;
#ifndef _DEBUG
                if ((errIphlp = GetBestInterface(dest, &nicIndex)) == NO_ERROR && (errIphlp = GetBestRoute(dest, NULL, &bestRoute)) == NO_ERROR)
#else
                if ((errIphlp = GetBestInterface(dest.s_addr, &nicIndex)) == NO_ERROR && (errIphlp = GetBestRoute(dest.s_addr, NULL, &bestRoute)) == NO_ERROR)
#endif
                    src.sin_addr.s_addr = bestRoute.dwForwardMask;
                else
                    printError(errIphlp);
            }
            DWORD mac[2U];
            auto macLen = MAC_LEN;
#ifndef _DEBUG
            if ((errIphlp = SendARP(dest, src.sin_addr.s_addr, &mac, &macLen)) != NO_ERROR)
#else
            if ((errIphlp = SendARP(dest.s_addr, src.sin_addr.s_addr, &mac, &macLen)) != NO_ERROR)
#endif
            {
                printError(errIphlp);
                continue;
            }
#ifdef _DEBUG
            else {
                auto macArr = (PBYTE)&mac;
                _tprintf(_T("Targeting MAC: "));
                for (auto index = 0UL; index < macLen; ++index)
                    _tprintf(_T("%.2x-"), macArr[index]);
                _tprintf(_T("\r\n"));
            }
#endif
#ifndef WSASendTo
            auto payload = new CHAR[PAYLOAD_LEN]{ '\xFF', '\xFF', '\xFF', '\xFF', '\xFF', '\xFF' };
#else
            WSABUF payload;
            payload.len = PAYLOAD_LEN;
            payload.buf = new CHAR[PAYLOAD_LEN]{ '\xFF', '\xFF', '\xFF', '\xFF', '\xFF', '\xFF' };
#endif
            auto macArr = (PBYTE)&mac;
            for (auto index = macLen; index < PAYLOAD_LEN; ++index)
#ifndef WSASendTo
                payload[index] = macArr[index % macLen];
#else
                payload.buf[index] = macArr[index % macLen];
#endif
#ifndef WSASendTo
            sendto(sock, payload, PAYLOAD_LEN, 0, (SOCKADDR*)&src, sizeof(src));
#ifdef IPv6
            sendto(sock6, payload, PAYLOAD_LEN, 0, (SOCKADDR*)&src, sizeof(src));
#endif
#else
            if (err = WSASendTo(sock, &payload, 1UL, NULL, NULL, (SOCKADDR*)&src, sizeof(src), NULL, NULL) != NO_ERROR)
            {
                _tprintf(_T("WSASendTo: error %d"), err);
            }
#ifdef IPv6
            WSASendTo(sock6, &payload, 1UL, NULL, NULL, (SOCKADDR*)&src, sizeof(src), NULL, NULL);
#endif
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
