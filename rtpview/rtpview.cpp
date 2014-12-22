// rtpview.cpp : Defines the entry point for the console application.
//
#include "stdafx.h"
#include "tinydav.h"
#include "httpforsdp.h"

#define DEFAULT_CODECS						(tmedia_codec_id_h264_mp | tmedia_codec_id_h264_bp | tmedia_codec_id_vp8 | tmedia_codec_id_pcma | tmedia_codec_id_pcmu) // use "tmedia_codec_id_all" to enable all codecs
#define DEFAULT_VIDEO_SIZE					tmedia_pref_video_size_vga
#define DEFAULT_VIDEO_FPS					30 // up to 120
#define DEFAULT_VIDEO_REMOTE_WINDOW_NAME	L"Remote video window (Decoded RTP)" // Remote window is where the decoded video frames are displayed
#define DEFAULT_VIDEO_LOCAL_WINDOW_NAME		L"Local video window (Preview)" // Local window is where the encoded video frames are displayed before sending (preview, PIP mode).
#define HTTP_FOR_SDP_PORT                   1080

// redefine _ASSERT here to allow executing the expression in release builds (just without the condition check & debug break)
// the predefined _ASSERT/assert would completely omit the expression (which would break how this code uses it for maximum readability - since this is demo code)
#pragma warning (disable : 4005)
#if defined(NDEBUG)
#	define _ASSERT(x) (void)(x)
#else
#	define _ASSERT(x)	assert(x)
#endif

#define GET_ADAPTER_BUFFER_SIZE 16000
#define GET_ADAPTER_MAX_ATTEMPTS 3

#define MALLOC(x) HeapAlloc(GetProcessHeap(), 0, (x))
#define FREE(x) HeapFree(GetProcessHeap(), 0, (x))

// Global variables
tmedia_session_mgr_t* pMgr = nullptr;               // Needs to be global so it's accessible in HTTP transport callback
volatile const char* sHttpReqGetContent  = nullptr; // Ditto (note NO thread-safe guards are placed around these strings accessed by HTTP callbacks - 
volatile const char* sHttpReqPutContent  = nullptr; //        so care must be taken that only a single endpoint triggers access to them at a time - 
volatile const char* sHttpRespContentSdp = nullptr; //        i.e. DON'T use multiple callers or run a caller & web browser to the HTTP_FOR_SDP_PORT simultaneously)

#if TMEDIA_UNDER_WINDOWS

static HWND __hWndLocal = NULL;
static HWND __hWndRemote = NULL;

static HWND GetConsoleHwnd(void)
{
#define MY_BUFSIZE 1024 // Buffer size for console window titles.
    HWND hwndFound;         // This is what is returned to the caller.
    TCHAR pszNewWindowTitle[MY_BUFSIZE]; // Contains fabricated
    // WindowTitle.
    TCHAR pszOldWindowTitle[MY_BUFSIZE]; // Contains original
    // WindowTitle.

    // Fetch current window title.
    GetConsoleTitle(pszOldWindowTitle, MY_BUFSIZE);

    // Format a "unique" NewWindowTitle.
    wsprintf(pszNewWindowTitle,TEXT("%d/%d"),
        GetTickCount(),
        GetCurrentProcessId());

    // Change current window title.
    SetConsoleTitle(pszNewWindowTitle);

    // Ensure window title has been updated.
    Sleep(40);

    // Look for NewWindowTitle.
    hwndFound=FindWindow(NULL, pszNewWindowTitle);

    // Restore original window title.
    SetConsoleTitle(pszOldWindowTitle);

    return(hwndFound);
}

static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_ERASEBKGND:
        return TRUE;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

static HWND GetHwnd(BOOL bRemote)
{
    HWND* pHWND = bRemote ? &__hWndRemote : &__hWndLocal;
    if(!*pHWND)
    {
        WNDCLASS wc = {0};

        wc.lpfnWndProc   = WindowProc;
        wc.hInstance     = GetModuleHandle(NULL);
        wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
        wc.lpszClassName =  L"MFCapture Window Class";
        /*_ASSERT*/(RegisterClass(&wc));

        _ASSERT(*pHWND = ::CreateWindow(
            wc.lpszClassName, 
            bRemote ? DEFAULT_VIDEO_REMOTE_WINDOW_NAME : DEFAULT_VIDEO_LOCAL_WINDOW_NAME, 
            WS_OVERLAPPEDWINDOW, 
            CW_USEDEFAULT, 
            CW_USEDEFAULT, 
            bRemote ? CW_USEDEFAULT : 352, 
            bRemote ? CW_USEDEFAULT : 288, 
            NULL, 
            NULL, 
            GetModuleHandle(NULL),
            NULL));
        ::SetWindowText(*pHWND, bRemote ? DEFAULT_VIDEO_REMOTE_WINDOW_NAME : DEFAULT_VIDEO_LOCAL_WINDOW_NAME);
        ::ShowWindow(*pHWND, SW_SHOWDEFAULT);
        ::UpdateWindow(*pHWND);
    }
    return *pHWND;
}

static HWND GetLocalHwnd(void)
{
    return GetHwnd(FALSE);
}

static HWND GetRemoteHwnd(void)
{
    return GetHwnd(TRUE);
}

#endif /* TMEDIA_UNDER_WINDOWS */

#include "rtpview.h"
#include <algorithm>

/* 
   Gets a local adapter IPV4 address that best matches (is on the same LAN/subnet) 
   as the given (presumed) remote address.
   
   If really given a local address, will return the same address after finding it
   on a physical adapter - or if not actually found - the closest matching address
   of from an active adapter as if a remote address were given.
*/
static std::string GetLocalAddressesIP4(std::string &sRemoteIP4)
{
    std::string sBestLocalIP = "";
    int iBestMatchBitDepth = 0;
    int iLocalMatchBitDepth = 0;
    int iNextOctetMatchDepth = 0;
    int iLocal_net, iLocal_host, iLocal_lh, iLocal_impno;
    int iRemote_net, iRemote_host, iRemote_lh, iRemote_impno;

    PIP_ADAPTER_ADDRESSES pAddresses = NULL;
    ULONG ulRetVal = ERROR_BUFFER_OVERFLOW;
    ULONG outBufLen = GET_ADAPTER_BUFFER_SIZE;
    ULONG getAttempts = 0;

    std::tr1::match_results<std::string::const_iterator> mrIP4;
    std::tr1::regex  rxParseIP4( "([0-9]{1,3})[.]([0-9]{1,3})[.]([0-9]{1,3})[.]([0-9]{1,3})");

    if ( std::tr1::regex_match(sRemoteIP4, mrIP4, rxParseIP4) )
    {
        std::istringstream( mrIP4[1] ) >> iRemote_net;
        std::istringstream( mrIP4[2] ) >> iRemote_host;
        std::istringstream( mrIP4[3] ) >> iRemote_lh;
        std::istringstream( mrIP4[4] ) >> iRemote_impno;
    }

    do {
        pAddresses = (IP_ADAPTER_ADDRESSES *) MALLOC(outBufLen);
        if (pAddresses == NULL) {
            ulRetVal = ERROR_INSUFFICIENT_BUFFER;
            getAttempts = GET_ADAPTER_MAX_ATTEMPTS;
            break;
        }
        
        ulRetVal = GetAdaptersAddresses(AF_INET, GAA_FLAG_INCLUDE_PREFIX, NULL, pAddresses, &outBufLen);
        
        if (ulRetVal == ERROR_BUFFER_OVERFLOW) {
            FREE(pAddresses);
            pAddresses = NULL;
            outBufLen *= 2; // Double buffer size for next attempt
        } else {
            break;
        }
        
        getAttempts++;
        
    } while ((ulRetVal == ERROR_BUFFER_OVERFLOW) && (getAttempts < GET_ADAPTER_MAX_ATTEMPTS));

    if (ulRetVal == NO_ERROR) {
        unsigned int i = 0;
        PIP_ADAPTER_ADDRESSES pCurrAddresses = NULL;
        PIP_ADAPTER_UNICAST_ADDRESS pUnicast = NULL;
        PIP_ADAPTER_ANYCAST_ADDRESS pAnycast = NULL;
        PIP_ADAPTER_MULTICAST_ADDRESS pMulticast = NULL;
        IP_ADAPTER_DNS_SERVER_ADDRESS *pDnServer = NULL;
        IP_ADAPTER_PREFIX *pPrefix = NULL;

        // If successful, output some information from the data we received
        pCurrAddresses = pAddresses;
        while (pCurrAddresses) {
            std::stringstream ssIpAddress;

            ssIpAddress.str("");  // reset to empty string from prior loop

            //printf("\tFriendly name: %wS\n", pCurrAddresses->FriendlyName);
            //printf("\tDescription: %wS\n", pCurrAddresses->Description);

            pUnicast = pCurrAddresses->FirstUnicastAddress;
            if (pUnicast != NULL) {
                for (i = 0; pUnicast != NULL; i++) {
                    if ((pUnicast->DadState == IpDadStatePreferred)          &&
                        (pUnicast->Address.lpSockaddr->sa_family == AF_INET)    ) {
                        bool bSetBestLocalIP = false;
                        PSOCKADDR_IN pSA = (PSOCKADDR_IN) pUnicast->Address.lpSockaddr;
                        iLocal_net   = (int) pSA->sin_addr.s_net;
                        iLocal_host  = (int) pSA->sin_addr.s_host;
                        iLocal_lh    = (int) pSA->sin_addr.s_lh;
                        iLocal_impno = (int) pSA->sin_addr.s_impno;

                        ssIpAddress << iLocal_net << "." << iLocal_host << "." << iLocal_lh << "." << iLocal_impno ;

                        iLocalMatchBitDepth = 1;      // Use lowest non-zero bit depth to use/keep the first "non-internal" IP found if no other IP's are measurably better

                        // For the first octet, simply perform a straight integer compare, and only if they match attempt a finer grained most significant bit (left) based match
                        // of the local IP to the remote IP being contacted (essentially trying to get the IP most likely on the same local subnet as the remote). For
                        // simplicity the subnet mask is being ignored (but ideally that would be accounted for too).
                        if (iLocal_net == iRemote_net) {
                            iLocalMatchBitDepth  = OCTET_EXACT_MATCH_DEPTH;                           // if the first octet matches, start at 8 bits

                            iNextOctetMatchDepth = GetOctetMsbMatchDepth(iLocal_host, iRemote_host);  // then check the next octet and add that match depth too
                            iLocalMatchBitDepth += iNextOctetMatchDepth;

                            // ONLY if the prior (_host) octet matched fully, should the next (less significant _lh) octet be checked
                            if (iNextOctetMatchDepth == OCTET_EXACT_MATCH_DEPTH) {
                                iNextOctetMatchDepth = GetOctetMsbMatchDepth(iLocal_lh, iRemote_lh);
                                iLocalMatchBitDepth += iNextOctetMatchDepth;

                                // ONLY if the prior (_lh) octet matched fully, should the next (less significant _impno) octet be checked
                                if (iNextOctetMatchDepth == OCTET_EXACT_MATCH_DEPTH) {
                                    iNextOctetMatchDepth = GetOctetMsbMatchDepth(iLocal_impno, iRemote_impno);
                                    iLocalMatchBitDepth += iNextOctetMatchDepth;
                                }
                            }
                        } else if ((iLocal_net == 127) || (iLocal_net == 169) || (iLocal_net == 172)) {
                            // If the first octet didn't match and this network typically used for internal (loopback/VM) connections, reset the match depth to 0
                            // to omit the address from the candidates for an external visible connection
                            iLocalMatchBitDepth = 0;
                        }

                        if (iLocalMatchBitDepth > iBestMatchBitDepth) {    // if the current match depth is greater than the last one found...
                            sBestLocalIP = ssIpAddress.str();              // save this IP as the current best match
                            iBestMatchBitDepth = iLocalMatchBitDepth;      // and save the matching bit depth to compare against the next local IP
                        }

                    } else if (pUnicast->Address.lpSockaddr->sa_family == AF_INET6) {
                        PSOCKADDR_IN6 pSA = (PSOCKADDR_IN6) pUnicast->Address.lpSockaddr;
                        ssIpAddress << pSA->sin6_addr.u.Word[0]; // unfinished - need to format all 8 words or 16 bytes as needed for sending ipv6 address as strings
                    }
                    //printf("\tUnicast Addresses: %s\n", ssIpAddress.str().c_str() );
                    pUnicast = pUnicast->Next;
                }
            } else
                fprintf(stderr, "***ERROR: No Unicast Addresses\n");

            pCurrAddresses = pCurrAddresses->Next;
        }
    }

    if (pAddresses) {
        FREE(pAddresses);
    }
    return sBestLocalIP;
}

static void WaitUntilDone()
{
#if TMEDIA_UNDER_WINDOWS
    fprintf(stderr, "\n**** CLOSE THE CONSOLE TO STOP THE APP ****\n");
    MSG msg = {0};
    while( WM_QUIT != msg.message )
    {
        if( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) )
        {
            TranslateMessage( &msg );
            DispatchMessage( &msg );
        }
        else
        {
            Sleep(50); // sleep for 50ms to minimize CPU but still keep window responsive to mouse/keyboard
        }
    }
#else
    fprintf(stderr, "\n**** PRESS ENTER TO STOP THE APP ****\n");
    getchar();
#endif
}


void readSdpIni(std::string &sCallerIP, std::string &sListenerIP, std::stringstream &ssBufferCaller, std::stringstream &ssBufferListener)
{
    bool bGettingCallerSdp = true;
    std::tr1::match_results<std::string::const_iterator> mrConnect;
    std::tr1::regex rxConnectIP4( "^(c=\\s*IN\\s+IP4\\s+)([0-9]{1,3}[.][0-9]{1,3}[.][0-9]{1,3}[.][0-9]{1,3})(.*)$" );

    ssBufferCaller.str("");    // reset to empty strings prior to read loop
    ssBufferListener.str("");

    std::string sReplaceIP4 = sCallerIP;               // default to caller SDP info in INI file if not specified with ": Caller"
    std::stringstream *ssBufferSdp = &ssBufferCaller;
    std::ifstream iniFile(RTPVIEW_INI_FILE);
    std::string sLine;

    while( std::getline(iniFile, sLine) ) 
    {
        trim(sLine);
        if (sLine[0] == ':')
        {
            tolowercase(sLine);
            bGettingCallerSdp = contains(sLine, "caller");
            if (bGettingCallerSdp)
            {
                sReplaceIP4 = sCallerIP;
                ssBufferSdp = &ssBufferCaller;
            }
            else
            {
                sReplaceIP4 = sListenerIP;
                ssBufferSdp = &ssBufferListener;
            }
            continue;
        }

        if (! sLine.empty() )
        {
            if ( (! sReplaceIP4.empty()) && std::tr1::regex_match(sLine, mrConnect, rxConnectIP4) )
            {
                sLine = mrConnect[1].str() + sReplaceIP4;
            }
            *ssBufferSdp << sLine << "\r\n";
        }
    }

}


static int startHttpForSdpTransport(const char* sHttpForSdpIP, tnet_port_t sHttpForSdpPort, const char* pHttpReqGetSdpResponse, thttp_for_sdp_transport_t** ppTransport)
{
    int ret = 0;

    // set string pointer (global) for HTTP content (used by transport callback) so that the SDP string is sent upon HTTP GET requests
    sHttpReqGetContent = pHttpReqGetSdpResponse;   // this could be refactored a bit cleaner (rather than using this tightly coupled global variable)

    // create the network endpoint on the IP address and port given (only support IPV4 for now)
    *ppTransport = thttp_for_sdp_transport_create(sHttpForSdpIP, sHttpForSdpPort, tnet_socket_type_tcp_ipv4);  // caller must use TSK_OBJECT_SAFE_FREE(*ppTransport) when done
    if (!(*ppTransport)) {
        TSK_DEBUG_ERROR("Failed to create HTTP transport on {%s:%d} ", sHttpForSdpIP, sHttpForSdpPort);
        return -1;
    }

    // then start the transport ("tiny" server) running on a background thread to service incoming HTTP requests, as well as outgoing HTTP peer connections
    ret = thttp_for_sdp_transport_start(*ppTransport);
    if (ret) {
        TSK_DEBUG_ERROR("Failed to start HTTP transport on {%s:%d} with error code = %d", sHttpForSdpIP, sHttpForSdpPort, ret);
    }

    return ret;
}

#define HTTP_PEER_CONNECT_TIMEOUT  1000   // 1 second

static int startHttpForSdpPeerConnection(const char* sRemoteIP, tnet_port_t sRemoteHttpPort, thttp_for_sdp_transport_t* pTransport, thttp_for_sdp_peer_t** ppPeer)
{
    int ret = 0;

    // connect to remote HTTP endpoint (if not already connected)
    if (*ppPeer == nullptr)
    {
        *ppPeer = thttp_for_sdp_transport_connect_to(pTransport, sRemoteIP, sRemoteHttpPort);  // caller must use TSK_OBJECT_SAFE_FREE(*ppPeer) when done
        ret = tnet_sockfd_waitUntilWritable((*ppPeer)->fd, HTTP_PEER_CONNECT_TIMEOUT);     // wait up to 1 second
        if (ret) {
            TSK_DEBUG_ERROR("Failed to connect to peer at (%s:%d) with error code = %d", sRemoteIP, sRemoteHttpPort, ret);
        }
    }

    return ret;
}


static void tHttpForSdpTransportProcessMsgFromCallback(thttp_for_sdp_transport_t* self, const thttp_message_t* msg, tnet_fd_t fd)
{
    if (THTTP_MESSAGE_IS_REQUEST(msg)) {
        if (tsk_striequals(THTTP_REQUEST_METHOD(msg), "GET")) {
            char* result = tsk_null;
            const char* content = const_cast<const char*>(sHttpReqGetContent);
            int len = tsk_sprintf(
                (char**)&result, 
                "HTTP/1.1 %u %s\r\n"
                "Server: RtpView.exe HTTP server \r\n"
                "Access-Control-Allow-Origin: *\r\n"
                "Content-Length: %u\r\n"
                "Content-Type: text/plain\r\n"
                "Connection: Close\r\n"
                "\r\n"
                "%s", 200, "OK", tsk_strlen(content), content);
            int sent = (int)thttp_for_sdp_transport_send_data(self, fd, result, len);
            if (sent <= 0) {
                TSK_DEBUG_ERROR("Response to HTTP %s failed with sent bytes = %d (of msg size = %d)", THTTP_REQUEST_METHOD(msg), sent, len);
            }
            TSK_FREE(result);
        }
        else if (tsk_striequals(THTTP_REQUEST_METHOD(msg), "PUT")) {
            // Only receive a single PUT message to avoid threading hazards (until consumer frees the prior message and sets global pointer to nullptr)
            if ( (sHttpReqPutContent == nullptr) && THTTP_MESSAGE_HAS_CONTENT(msg) )
            {
                // first extract content of PUT
                char* msgbody = tsk_null;
                tsk_sprintf( (char**)&msgbody, "%s", THTTP_MESSAGE_CONTENT(msg));
                sHttpReqPutContent = msgbody; // consumer must clean up buffer once done processing PUT content

                // then send "OK" response
                char* result = tsk_null;
                int len = tsk_sprintf(
                    (char**)&result, 
                    "HTTP/1.1 %u %s\r\n"
                    "Server: RtpView.exe HTTP server \r\n"
                    "Access-Control-Allow-Origin: *\r\n"
                    "Content-Length: %u\r\n"
                    "Content-Type: text/plain\r\n"
                    "Connection: Close\r\n"
                    "\r\n"
                    , 200, "OK", 0);
                int sent = (int)thttp_for_sdp_transport_send_data(self, fd, result, len);
                if (sent <= 0) {
                    TSK_DEBUG_ERROR("Response to HTTP %s failed with sent bytes = %d (of msg size = %d)", THTTP_REQUEST_METHOD(msg), sent, len);
                }
                TSK_FREE(result);
            }
        }
    }
    else if (THTTP_MESSAGE_IS_RESPONSE(msg)) {
        // for now just treat any non-zero length content as the response to the GET of the SDP (should change to use an explicit custom header to ensure this is really the case)
        if ( THTTP_RESPONSE_IS_2XX(msg) && THTTP_MESSAGE_HAS_CONTENT(msg) && (THTTP_MESSAGE_CONTENT_LENGTH(msg) > 0) )
        {
            // extract content of response
            char* msgbody = tsk_null;
            tsk_sprintf( (char**)&msgbody, "%s", THTTP_MESSAGE_CONTENT(msg));
            sHttpRespContentSdp = msgbody; // consumer must clean up buffer once done processing GET response
        }
        else
        {
            TSK_DEBUG_INFO("Unhandled HTTP response code (%d) and phrase \"%s\" ", THTTP_RESPONSE_CODE(msg), THTTP_RESPONSE_PHRASE(msg));
        }
    }

}


static int waitForRemoteIp(char** ppRemoteIP)
{
    // simply wait until the HTTP callback receives a PUT request and sets the global ptr sHttpReqPutContent to a buffer with the PUT contents
    TSK_DEBUG_INFO("Start waiting for remote HTTP PUT request with IP");
    while (sHttpReqPutContent == nullptr)
    {
        tsk_thread_sleep(50); // wait 50ms before checking for PUT msg again
    }
    TSK_DEBUG_INFO("Received HTTP PUT request with IP {%s}", sHttpReqPutContent);

    if (ppRemoteIP != nullptr)
    {
        TSK_SAFE_FREE((*ppRemoteIP));                       // free any prior buffer
        tsk_sprintf(ppRemoteIP, "%s", sHttpReqPutContent);  // save off the remote IP string
    }

    TSK_SAFE_FREE(sHttpReqPutContent);                      // free the buffer allocated by the HTTP PUT callback

    return 0;
}


static int getRemoteSdp(char** ppSdpFromRemote, const char* sRemoteIP, tnet_port_t sRemoteHttpPort, thttp_for_sdp_transport_t* pTransport, thttp_for_sdp_peer_t* pPeer)
{
    int nSent = 0;
    int ret = 0;
    char* sHttpGetMsg = nullptr;

    // construct HTTP GET message
    int len = tsk_sprintf(
        (char**)&sHttpGetMsg, 
        "GET /sdp HTTP/1.1\r\n"
        "Host: %s:%u\r\n"
        "User-Agent: Mozilla/5.0 (RtpView.exe; Doubango 2.0) AppleWebKit/537.36 (KHTML, like Gecko)\r\n" \
        "Connection: keep-alive\r\n"
        "Cache-Control: max-age=0\r\n"
        "Accept: text/plain,text/html;q=0.9,*/*;q=0.8\r\n" \
        "Accept-Encoding: gzip,deflate,sdch\r\n" \
        "Accept-Language: en-US,en;q=0.8\r\n" \
        "Content-Type: text/plain\r\n"
        "Content-Length: %u\r\n"
        "\r\n"
        , sRemoteIP, sRemoteHttpPort, 0);

    // send HTTP GET (SDP configuration) request to remote end
    nSent = (int)thttp_for_sdp_transport_send_data(pTransport, pPeer->fd, sHttpGetMsg, (tsk_size_t)tsk_strlen(sHttpGetMsg));
    TSK_DEBUG_INFO("Sent %d bytes", nSent);
    
    // if more than 0 bytes sent, consider it successful
    if (nSent > 0)
    {
        // then wait for the response (should really put timeout & exception handling here and in caller)
        TSK_DEBUG_INFO("Start waiting for remote HTTP GET response with SDP");
        while (sHttpRespContentSdp == nullptr)
        {
            tsk_thread_sleep(50); // wait 50ms before checking for PUT msg again
        }
        TSK_DEBUG_INFO("Received HTTP GET response with SDP");

        TSK_SAFE_FREE((*ppSdpFromRemote));                         // free any prior buffer
        tsk_sprintf( ppSdpFromRemote, "%s", sHttpRespContentSdp);  // copy the SDP content from the response buffer
        TSK_SAFE_FREE(sHttpRespContentSdp);                        // then free the response buffer

        ret = 0; // return success
    }
    else
    {
        TSK_DEBUG_ERROR("Failed to send HTTP GET to remote peer.");
        ret = -1;
    }

    return ret;
}


static int putLocalIpOnRemote(const char* sLocalIP, const char* sRemoteIP, tnet_port_t sRemoteHttpPort, thttp_for_sdp_transport_t* pTransport, thttp_for_sdp_peer_t* pPeer)
{
    int nSent = 0;
    int ret = 0;
    char* sHttpPutMsg = nullptr;

    // construct HTTP PUT message
    int len = tsk_sprintf(
        (char**)&sHttpPutMsg, 
        "PUT /remoteip HTTP/1.1\r\n"
        "Host: %s:%u\r\n"
        "User-Agent: Mozilla/5.0 (RtpView.exe; Doubango 2.0) AppleWebKit/537.36 (KHTML, like Gecko)\r\n" \
        "Connection: keep-alive\r\n"
        "Cache-Control: max-age=0\r\n"
        "Accept: text/plain,text/html;q=0.9,*/*;q=0.8\r\n" \
        "Accept-Encoding: gzip,deflate,sdch\r\n" \
        "Accept-Language: en-US,en;q=0.8\r\n" \
        "Content-Type: text/plain\r\n"
        "Content-Length: %u\r\n"
        "\r\n"
        "%s", sRemoteIP, sRemoteHttpPort, tsk_strlen(sLocalIP), sLocalIP);

    // send HTTP PUT (of the local IP address) to remote end
    nSent = (int)thttp_for_sdp_transport_send_data(pTransport, pPeer->fd, sHttpPutMsg, (tsk_size_t)tsk_strlen(sHttpPutMsg));
    TSK_DEBUG_INFO("Sent %d bytes", nSent);

    // if more than 0 bytes sent, consider it successful
    if (nSent > 0)
    {
        ret = 0; // return success
    }
    else
    {
        TSK_DEBUG_ERROR("Failed to send HTTP PUT to remote peer.");
        ret = -1;
    }

    // release the allocated message buffer
    TSK_FREE(sHttpPutMsg);

    return ret;
}


static void startRtpSession(std::string sCallIP, std::string sListenIP)
{
    thttp_for_sdp_transport_t* pTransport = nullptr;
    thttp_for_sdp_peer_t* pPeer = nullptr;
    const tsdp_message_t* sdp_lo;
    tsdp_message_t* sdp_ro = nullptr;
    char* pStringSdpLO = nullptr;
    char* pStringSdpRO = nullptr;
    int ret = 0;
    tmedia_type_t type = RTPVIEW_MEDIA_TYPE;
    tsk_boolean_t bIsCaller = tsk_false;
    tsk_boolean_t bIsLoopback = tsk_true;
    std::string sLocalIP  = LOOPBACK_IP4;
    char* sRemoteIP = nullptr;

    if (! sListenIP.empty() ) 
    {
        sLocalIP = GetLocalAddressesIP4(sListenIP);
        bIsCaller = tsk_false;
        bIsLoopback = tsk_false;
    } 
    else if (! sCallIP.empty() ) 
    {
        sLocalIP = GetLocalAddressesIP4(sCallIP);
        bIsCaller = tsk_true;
        bIsLoopback = tsk_false;

        // the caller can copy the IP parsed from the command line as the remote IP string
        tsk_sprintf((char**)&sRemoteIP, "%s", sCallIP.c_str());
    }

    // create/init the session
    pMgr = tmedia_session_mgr_create(type, sLocalIP.c_str(), tsk_false, bIsCaller);
    if (pMgr == nullptr)
    {
        TSK_DEBUG_ERROR("Failed to create media session on IP {%s} ", sLocalIP.c_str());
        goto final_cleanup;
    }

    // get SDP of local session (allocates the local ports - needed now to communicate these to remote offerer over http)
    sdp_lo = tmedia_session_mgr_get_lo(pMgr);

    // convert the SDP to a string to send over http (or parse in the case of loop-back testing)
    if((pStringSdpLO = tsdp_message_tostring(sdp_lo))){
        sdp_ro = tsdp_message_parse(pStringSdpLO, (tsk_size_t)tsk_strlen(pStringSdpLO));  // default remote set to local (i.e. loop-back)
        TSK_DEBUG_INFO("Local SDP >>>>>>>>>>>>>>>>>>>> BEGIN <<<<<<<<<<<<<<<<<<<< \r\n%s", pStringSdpLO);
        TSK_DEBUG_INFO("Local SDP >>>>>>>>>>>>>>>>>>>> END <<<<<<<<<<<<<<<<<<<<<< ");
    }

    // Avoid HTTP setup/use during loopback testing
    if (!bIsLoopback)
    {
        // NOTE the following HTTP transactions comprise an arbitrary and simple method for exchanging SDP configuration between the caller and listener, 
        // (along with handshaking before starting the RTP session) as opposed to using a much more robust method such as SIP (Session Initiation Protocol)
        // unlike SIP (which requires intermediate servers) this simple protocol allows for point-to-point connection over LANs

        // start up the http transport ("tiny" server) to announce the SDP of this local session (using an arbitrary/custom "call initiation" protocol)
        ret = startHttpForSdpTransport(sLocalIP.c_str(), HTTP_FOR_SDP_PORT, pStringSdpLO, &pTransport);
        if (ret) { goto final_cleanup; }

        // if this is the listener, need to wait for the HTTP PUT to get the remote (caller) IP
        if (!bIsCaller)
        {
             waitForRemoteIp(&sRemoteIP);
        }
        // otherwise if this is the caller, remote IP was given on command line so NO waiting needed here

        // start the peer connection to remote endpoint
        ret = startHttpForSdpPeerConnection(sRemoteIP, HTTP_FOR_SDP_PORT, pTransport, &pPeer);
        if (ret) { goto final_cleanup; }

        // get the SDP info from it (via HTTP GET)
        ret = getRemoteSdp(&pStringSdpRO, sRemoteIP, HTTP_FOR_SDP_PORT, pTransport, pPeer);
        if (ret) { goto final_cleanup; }

        // free the default remote SDP that was set for loop-back testing earlier, then parse the one retrieved from the remote HTTP endpoint
        TSK_OBJECT_SAFE_FREE(sdp_ro);
        sdp_ro = tsdp_message_parse(pStringSdpRO, (tsk_size_t)tsk_strlen(pStringSdpRO));

        // then send the local IP to the remote end (via HTTP PUT), signaling that this side is ready to begin the media session
        putLocalIpOnRemote(sLocalIP.c_str(), sRemoteIP, HTTP_FOR_SDP_PORT, pTransport, pPeer);
        if (ret) { goto final_cleanup; }

        // if this is the caller, need to wait for the HTTP PUT from the listener to complete the handshake that both sides are ready for the media session to start
        if (bIsCaller)
        {
            waitForRemoteIp(&sRemoteIP);
        }
        // otherwise if this is the listener, both sides are now ready (the caller indicated so when the 'waitForRemoteIp' further up completed)
    }

    // finally set the remote SDP info for the session (negotiated from the HTTP transactions from above for non-loopback testing)
    _ASSERT((ret = tmedia_session_mgr_set_ro(pMgr, sdp_ro, tmedia_ro_type_answer)) == 0);
    TSK_OBJECT_SAFE_FREE(sdp_ro);

    // attach video displays
    _ASSERT((ret = attach_video_displays(pMgr)) == 0);

    // actually start the session
    _ASSERT((ret = tmedia_session_mgr_start(pMgr)) == 0);

    WaitUntilDone();

final_cleanup:
    // free allocations from above
    TSK_OBJECT_SAFE_FREE(pPeer);
    TSK_OBJECT_SAFE_FREE(pTransport);
    TSK_OBJECT_SAFE_FREE(pMgr);
    TSK_SAFE_FREE(pStringSdpLO);
    TSK_SAFE_FREE(pStringSdpRO);
    TSK_SAFE_FREE(sRemoteIP);
}


void parseargs(int argc, _TCHAR* argv[], _TCHAR* envp[], std::string &sCallIP, std::string &sListenIP)
{
    bool bParsedCall, bParseListen;
    int i = 1; // args start at index 1

    while ((i < argc) && (argv[i] != nullptr))
    {
        // use regex to check for and validate IP strings
        std::tr1::match_results<std::string::const_iterator> mrIP4;
        std::tr1::regex rxCheckIP4( "([0-9]{1,3}[.][0-9]{1,3}[.][0-9]{1,3}[.][0-9]{1,3})");
        std::string sArgIP = "";
        std::string sCheckedIP = "";

        // declare string to handle incoming argument (may be a wide string if unicode support enabled)
        TString tsArg;
        tsArg = argv[i];
        i += 1;

        // declare converter to go from incoming arguments to std::string's for remainder of code
        std::wstring_convert<TConvertUtf8, _TCHAR> converter;  // untested for non-UNICODE compilation

        // use regex to match "/l", "-l", "/listenip", "-listen", etc. in a robust/tolerant (though imprecise) way
        TRegex rxListenip( _T("[-/][lL].*") );
        bParseListen = std::tr1::regex_match(tsArg.begin(), tsArg.end(), rxListenip);

        // use regex to match "/c", "-c", "/callip", "-call", etc. in a robust/tolerant (though imprecise) way
        TRegex rxCallip( _T("[-/][cC].*") );
        bParsedCall = std::tr1::regex_match(tsArg.begin(), tsArg.end(), rxCallip);

        if ( bParseListen || bParsedCall )
        {
            sCheckedIP = LOOPBACK_IP4;   // default to loopback IP4 address in case it is not specified, or an invalid string is given

            if (i < argc)
            {
                tsArg = argv[i];
                sArgIP = converter.to_bytes( tsArg );

                if ( std::tr1::regex_match(sArgIP, mrIP4, rxCheckIP4) )
                {
                    i += 1;                        // arg was a valid IP4, so advance arg counter for next loop iteration
                    sCheckedIP = mrIP4[1].str();   // and get matched (cleaned) string as the checked IP
                }
            }

            if (bParseListen)
            {
                sListenIP = sCheckedIP;  // return checked string as the IP to listen
            } 
            else if (bParsedCall)
            {
                sCallIP = sCheckedIP;    // return checked string as the IP to call
            }
        }

    }
}

/* 
   If called with no parameters, simple streaming over the loopback adapter is tested.

   To test from PC1 (caller) to PC2 (receiver/listener), perform the following in sequence:

       1) start receiver listening with : "rtpview.exe /listen <IP_of_PC2>"
       2) then call receiver with       : "rtpview.exe /call   <IP_of_PC2>"

   Where <IP_of_PC2> is replaced with the "public/external" IPV4 address of PC2 (receiver/listener) 
   that will be used for media transport (with "/listen" the given IP is used to disambiguate from
   other adapters/IP's in the system that may not be externally visible to PC1).
  
   NOTE IPV4 addresses are of the form xxx.xxx.xxx.xxx (i.e. 192.168.1.101 and 192.168.1.102). 
*/
int _tmain(int argc, _TCHAR* argv[], _TCHAR* envp[])
{
    int ret = 0;
    bool bGettingClientIni = true;
    std::string sCallIP   = "";
    std::string sListenIP = "";
    //tsk_debug_set_level(DEBUG_LEVEL_INFO); // uncomment if TSK debug messages are needed in release builds

    parseargs(argc, argv, envp, sCallIP, sListenIP);

    _ASSERT((ret = tnet_startup()) == 0);
    _ASSERT((ret = tdav_init()) == 0);

	_ASSERT((ret = tmedia_defaults_set_video_zeroartifacts_enabled(tsk_false)) == 0);
    _ASSERT((ret = tmedia_defaults_set_pref_video_size(DEFAULT_VIDEO_SIZE)) == 0);
    _ASSERT((ret = tmedia_defaults_set_video_fps(DEFAULT_VIDEO_FPS)) == 0);

	_ASSERT((ret = tmedia_defaults_set_agc_enabled(tsk_true)) == 0);
	_ASSERT((ret = tmedia_defaults_set_echo_supp_enabled(tsk_true)) == 0);
    _ASSERT((ret = tmedia_defaults_set_echo_tail(100)) == 0);
    _ASSERT((ret = tmedia_defaults_set_noise_supp_enabled(tsk_true)) == 0);

    _ASSERT((ret = tdav_set_codecs((tdav_codec_id_t)DEFAULT_CODECS)) == 0);
    _ASSERT((ret = tdav_codec_set_priority((tdav_codec_id_t)tmedia_codec_id_h264_mp, 0)) == 0);
    _ASSERT((ret = tdav_codec_set_priority((tdav_codec_id_t)tmedia_codec_id_h264_bp, 1)) == 0);


    // video camera friendly strings for testing
    // 0: "GEIT MViQ Probe Video"
    // 1: "Logitech Webcam C930e"
    // 2: "Integrated Camera - Lenovo ThinkPad"
    // 3: "Rocketfish HD Webcam Pro"
    // 4: "FaceTime HD Camera (Built-in) - iMAC 2003"
    // If none is connected then the default device will be selected
    _ASSERT((ret = tmedia_producer_set_friendly_name(tmedia_video, "Logitech Webcam C930e")) == 0);

    startRtpSession(sCallIP, sListenIP);

    tdav_deinit();
    tnet_cleanup();

    return ret;
}

