/*
 * Copyright (c) 2018-2022 Arm Limited. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#ifndef _PRINT_LOG_H_
#define _PRINT_LOG_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "cmsis_os2.h" 
#include "RTOS_config.h"

#define ERR_LEVEL		3
#define WARN_LEVEL		2
#define INFO_LEVEL		1
#define DBG_LEVEL		0

#ifdef DEBUG
#define LOG_LEVEL		DBG_LEVEL
#endif

#ifndef LOG_LEVEL
#define LOG_LEVEL		INFO_LEVEL
#endif

/**
 * @brief Initialize the mutex of UART.
 */
void vUARTLockInit( void ) __attribute__( ( section( "privileged_functions" ) ) );

void print_log( const char *format, ... );

#define SOH				"\001"
#define ERR_HEAD		SOH "Error: "
#define WARN_HEAD		SOH "Warning: "
#define INFO_HEAD		SOH "Info: "
#define DBG_HEAD		SOH "Debug: "

#if LOG_LEVEL <= DBG_LEVEL
#define DBG_LOG( fmt, ... )		print_log( DBG_HEAD fmt, ##__VA_ARGS__ )
#else
#define DBG_LOG( x )
#endif

#if LOG_LEVEL <= INFO_LEVEL
#define INFO_LOG( fmt, ... )	print_log( INFO_HEAD fmt, ##__VA_ARGS__ )
#else
#define INFO_LOG( x )
#endif

#if LOG_LEVEL <= WARN_LEVEL
#define WARN_LOG( fmt, ... )	print_log( WARN_HEAD fmt, ##__VA_ARGS__ )
#else
#define WARN_LOG( x )
#endif

#if LOG_LEVEL <= ERR_LEVEL
#define ERR_LOG( fmt, ... )		print_log( ERR_HEAD fmt, ##__VA_ARGS__ )
#else
#define ERR_LOG( x )
#endif

#ifdef __cplusplus
}
#endif

#endif /* _PRINT_LOG_H_ */
