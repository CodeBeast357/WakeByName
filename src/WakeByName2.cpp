// WakeByName.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

const _TCHAR* USAGE = _T("%s <name>\r\n");
const _TCHAR* DNS_NOTFOUND = _T("Could not resolve: %s\r\n");
const USHORT PORT = 7U;
const DWORD DNS_OPTIONS = DNS_QUERY_STANDARD;
const WORD DNS_TYPES = DNS_TYPE_A;
const DWORD VERSION_WSA = MAKEWORD(2UL, 2UL);
const ULONG BROADCAST_FULL = -1L;
const INT PAYLOAD_LEN = 102;
const ULONG MAC_LEN = 6UL;

INT _tmain(INT argc, _TCHAR* argv[]) {
    INT err = EXIT_SUCCESS;
    USHORT port = PORT;
    while (optind < argc) {
        INT arg;
        if ((arg = getopt(argc, argv, _T("a:hn:p:"))) != -1) {
            switch (arg) {
                case _T('a'):
                    _tprintf(_T("a: %s\r\n"), optarg);
                    break;
                case _T('n'):
                    _tprintf(_T("n: %s\r\n"), optarg);
                    break;
                case _T('p'):
                    port = _tstoi(optarg);
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
                    _TCHAR* name = new _TCHAR[_MAX_FNAME];
                    _tsplitpath_s(argv[0], NULL, NULL, NULL, NULL, name, _MAX_FNAME, NULL, NULL);
                    _tprintf(USAGE, name);
                    err = EXIT_FAILURE;
                    goto END;
            }
        } else {
            _tprintf(_T("%s\r\n"), argv[optind++]);
        }
    }
    exit(EXIT_SUCCESS);
    PIP4_ARRAY pSrvList = (PIP4_ARRAY)LocalAlloc(LPTR, sizeof(IP4_ARRAY));
    pSrvList->AddrCount = 1;
    pSrvList->AddrArray[0] = 0x0100A8C0; // 192.168.0.1
    PDNS_RECORD results = NULL;
    if ((err = DnsQuery(argv[1], DNS_TYPES, DNS_OPTIONS, pSrvList, &results, NULL)) || !results) {
        _tprintf(DNS_NOTFOUND, argv[1]);
        goto FREELIST;
    }
    PMIB_IPADDRTABLE nicInfos = NULL;
    DWORD infosSize;
    if (GetIpAddrTable(nicInfos, &infosSize, FALSE) == ERROR_INSUFFICIENT_BUFFER) {
        free(nicInfos);
        nicInfos = (PMIB_IPADDRTABLE)malloc(infosSize);
    }
    if (err = GetIpAddrTable(nicInfos, &infosSize, FALSE)) {
        goto FREETABLE;
    }
    WSADATA wsaData;
    SOCKET sock;
    if (err = WSAStartup(VERSION_WSA, &wsaData) != NO_ERROR
        && (sock = socket(AF_INET, SOCK_DGRAM, IPPROTO::IPPROTO_UDP)) == INVALID_SOCKET) {
        goto CLEANUP;
    }
    IN_ADDR dest;
    SOCKADDR_IN src;
    src.sin_family = AF_INET;
    src.sin_port = port;
    CHAR* payload = new CHAR[PAYLOAD_LEN]; // { '\xFF', '\xFF', '\xFF', '\xFF', '\xFF', '\xFF' };
    for (PDNS_RECORD item = results; item != NULL; item = item->pNext) {
        switch (item->wType) {
            case DNS_TYPE_A:
                dest.s_addr = item->Data.A.IpAddress;
                break;
            default:
                continue;
        }
        DWORD nicIndex;
        if (GetBestInterface(dest.s_addr, &nicIndex) != NO_ERROR)
            continue;
        PMIB_IPADDRROW nic;
        for (UINT entry = 0U; entry < nicInfos->dwNumEntries; ++entry) {
            if ((nic = &nicInfos->table[entry])->dwIndex == nicIndex) {
                break;
            }
        }
        src.sin_addr.s_addr = nic->dwMask ^ BROADCAST_FULL | nic->dwAddr;
        DWORD mac[2U];
        ULONG macLen = MAC_LEN;
        if (SendARP(dest.s_addr, src.sin_addr.s_addr, &mac, &macLen) != NO_ERROR)
            continue;
        PBYTE macArr = (PBYTE)&mac;
        for (ULONG index = macLen; index < PAYLOAD_LEN; ++index)
            payload[index] = macArr[index % macLen];
        sendto(sock, payload, PAYLOAD_LEN, 0, (SOCKADDR*)&src, sizeof(src));
    }
    closesocket(sock);
CLEANUP:
    WSACleanup();
FREETABLE:
    free(nicInfos);
FREELIST:
    DnsRecordListFree(results, DNS_FREE_TYPE::DnsFreeRecordList);
END:
    return err;
}
