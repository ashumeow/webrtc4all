/*
* Copyright (C) 2013 Gotham City. All rights reserved.
*/
/**@file dllmain.cc
 * @brief Implementation for DLL exports and COM class factory for registration. 
 * If 'Regsvr32.exe' works it's maily because of this.
 *
 * @author Batman@GothamCity
 */
#include "stdafx.h"

#include "tinydav.h"

#define RUN_TEST_RTP	1

#if RUN_TEST_RTP
#include "test_rtp.h"
#endif

int _tmain(int argc, _TCHAR* argv[])
{
    tnet_startup();
    tdav_init();

#if RUN_TEST_RTP
    test_rtp();
#endif

    tdav_deinit();
    tnet_cleanup();

    return 0;
}