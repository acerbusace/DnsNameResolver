# define WIN32_LEAN_AND_MEAN

#include <functional>
#include <iostream>
#include <Windows.h>
#include <WinDNS.h>
#include <WS2tcpip.h>
#include <string>

#pragma comment(lib, "dnsapi.lib")
#pragma comment(lib, "ws2_32.lib")

// DNS_STATUS = LONG = int in Windows
// DWORD = ULONG = uint in Windows
typedef void (WINAPI* Callback) (DNS_STATUS, PSTR, DWORD);
void WINAPI DnsQueryCompletionRoutine(PVOID, PDNS_QUERY_RESULT);
extern "C" __declspec(dllexport) void WINAPI ResolveDnsName(PCWSTR, WORD, Callback callback);

typedef struct _DNS_QUERY_CONTEXT {
	WORD QueryType;
	DNS_QUERY_RESULT QueryResult;
	Callback Callback;
	DNS_QUERY_CANCEL DnsCancelHandle;
} DNS_QUERY_CONTEXT, *PDNS_QUERY_CONTEXT;

void WINAPI TestCallback(DNS_STATUS dnsStatus, PSTR ipAddress, DWORD ttl) {
	std::cout <<  "TestCallback: dnsStatus: " << dnsStatus << " | ipAddress: " << ipAddress << " | ttl: " << ttl << std::endl;
}

int main() {
	ResolveDnsName(L"www.bing.com", DNS_TYPE_A, TestCallback);

	std::cin.get();
	return 0;
}

/* Resolves given Dns Name.
 * pDnsName: Dns Name.
 * queryType: Type of records to query for. Currently A and AAAA records are supported.
 * callback: Function to callback when request completes.
*/
void WINAPI ResolveDnsName(PCWSTR pDnsName, WORD queryType, Callback callback) {
	PDNS_QUERY_CONTEXT pDnsQueryContext = new DNS_QUERY_CONTEXT();
	pDnsQueryContext->QueryType = queryType;
	pDnsQueryContext->QueryResult.Version = DNS_QUERY_REQUEST_VERSION1;
	pDnsQueryContext->Callback = callback;

	DNS_QUERY_REQUEST DnsQueryRequest;
	DnsQueryRequest.Version = DNS_QUERY_REQUEST_VERSION1;
	DnsQueryRequest.QueryName = pDnsName;
	DnsQueryRequest.QueryType = pDnsQueryContext->QueryType;
	DnsQueryRequest.QueryOptions = DNS_QUERY_BYPASS_CACHE;
	DnsQueryRequest.pDnsServerList = NULL;
	DnsQueryRequest.InterfaceIndex = 0;
	DnsQueryRequest.pQueryCompletionCallback = DnsQueryCompletionRoutine;
	DnsQueryRequest.pQueryContext = pDnsQueryContext;

	DNS_STATUS DnsStatus = DnsQueryEx(&DnsQueryRequest, &pDnsQueryContext->QueryResult, &pDnsQueryContext->DnsCancelHandle);

	if (DnsStatus != DNS_REQUEST_PENDING) {
		pDnsQueryContext->QueryResult.QueryStatus = DnsStatus;
		DnsQueryCompletionRoutine(pDnsQueryContext, &pDnsQueryContext->QueryResult);
	}
}

/* Callback function for DnsQueryEx.
 * pQueryContext: Dns query context. Contains, record type, user callback and cancel handle.
 * pQueryResults: Results of DnsQueryEx. Contains records pointers and dns status code.
*/
void WINAPI DnsQueryCompletionRoutine(PVOID pQueryContext, PDNS_QUERY_RESULT pQueryResults) {
	PDNS_QUERY_CONTEXT pDnsQueryContext = (PDNS_QUERY_CONTEXT)pQueryContext;

	//PCWSTR ret = NULL;
	//WCHAR ipAddress[128];
	PCSTR ret = NULL;
	CHAR ipAddress[128];

	if (pQueryResults->QueryStatus != ERROR_SUCCESS) {
		pDnsQueryContext->Callback(pQueryResults->QueryStatus, ipAddress, 0);
		goto exit;
	}

	for (PDNS_RECORD p = pQueryResults->pQueryRecords; p; p = p->pNext)
	{
		if (p->wType == DNS_TYPE_A) {
			IN_ADDR ipv4;
			ipv4.S_un.S_addr = p->Data.A.IpAddress;

			// InetNtopW is not needed, since ASCHII supports IPV4
			ret = InetNtopA(AF_INET, (PVOID)&ipv4, ipAddress, 128);
			//ret = InetNtopW(AF_INET, (PVOID)&ipv4, ipAddress, 128);
		} else if (p->wType == DNS_TYPE_AAAA) {
			IN6_ADDR ipv6;
			CopyMemory(ipv6.u.Byte, p->Data.AAAA.Ip6Address.IP6Byte, sizeof(ipv6.u.Byte));

			// InetNtopW is not needed, since ASCHII supports IPV6
			ret = InetNtopA(AF_INET6, (PVOID)&ipv6, ipAddress, 128);
			//ret = InetNtopW(AF_INET6, (PVOID)&ipv6, ipAddress, 128);
		}

		if (p->wType == DNS_TYPE_A || p->wType == DNS_TYPE_AAAA) {
			pDnsQueryContext->Callback(
				ret == NULL ? WSAGetLastError() : pQueryResults->QueryStatus,
				ipAddress,
				//"Hello World",
				p->dwTtl
			);

			goto exit;
		}
	}

	pDnsQueryContext->Callback(ERROR_NOT_FOUND, ipAddress, 0);

	exit:
	if (pQueryResults->pQueryRecords) {
		DnsRecordListFree(pQueryResults->pQueryRecords, DnsFreeRecordList);
	}

	delete pDnsQueryContext;
}
