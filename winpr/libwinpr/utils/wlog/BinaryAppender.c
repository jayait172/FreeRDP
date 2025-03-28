/**
 * WinPR: Windows Portable Runtime
 * WinPR Logger
 *
 * Copyright 2013 Marc-Andre Moreau <marcandre.moreau@gmail.com>
 * Copyright 2015 Thincast Technologies GmbH
 * Copyright 2015 DI (FH) Martin Haimberger <martin.haimberger@thincast.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <winpr/config.h>

#include "BinaryAppender.h"
#include <winpr/crt.h>
#include <winpr/assert.h>
#include <winpr/file.h>
#include <winpr/path.h>
#include <winpr/stream.h>

typedef struct
{
	WLOG_APPENDER_COMMON();

	char* FileName;
	char* FilePath;
	char* FullFileName;
	FILE* FileDescriptor;
} wLogBinaryAppender;

static BOOL WLog_BinaryAppender_Open(wLog* log, wLogAppender* appender)
{
	wLogBinaryAppender* binaryAppender = NULL;
	if (!log || !appender)
		return FALSE;

	binaryAppender = (wLogBinaryAppender*)appender;
	if (!binaryAppender->FileName)
	{
		binaryAppender->FileName = (char*)malloc(MAX_PATH);
		if (!binaryAppender->FileName)
			return FALSE;
		(void)sprintf_s(binaryAppender->FileName, MAX_PATH, "%" PRIu32 ".wlog",
		                GetCurrentProcessId());
	}

	if (!binaryAppender->FilePath)
	{
		binaryAppender->FilePath = GetKnownSubPath(KNOWN_PATH_TEMP, "wlog");
		if (!binaryAppender->FilePath)
			return FALSE;
	}

	if (!binaryAppender->FullFileName)
	{
		binaryAppender->FullFileName =
		    GetCombinedPath(binaryAppender->FilePath, binaryAppender->FileName);
		if (!binaryAppender->FullFileName)
			return FALSE;
	}

	if (!winpr_PathFileExists(binaryAppender->FilePath))
	{
		if (!winpr_PathMakePath(binaryAppender->FilePath, 0))
			return FALSE;
		UnixChangeFileMode(binaryAppender->FilePath, 0xFFFF);
	}

	binaryAppender->FileDescriptor = winpr_fopen(binaryAppender->FullFileName, "a+");

	if (!binaryAppender->FileDescriptor)
		return FALSE;

	return TRUE;
}

static BOOL WLog_BinaryAppender_Close(WINPR_ATTR_UNUSED wLog* log, wLogAppender* appender)
{
	wLogBinaryAppender* binaryAppender = NULL;

	if (!appender)
		return FALSE;

	binaryAppender = (wLogBinaryAppender*)appender;
	if (!binaryAppender->FileDescriptor)
		return TRUE;

	if (binaryAppender->FileDescriptor)
		(void)fclose(binaryAppender->FileDescriptor);

	binaryAppender->FileDescriptor = NULL;

	return TRUE;
}

static BOOL WLog_BinaryAppender_WriteMessage(wLog* log, wLogAppender* appender,
                                             wLogMessage* message)
{
	FILE* fp = NULL;
	wStream* s = NULL;
	size_t MessageLength = 0;
	size_t FileNameLength = 0;
	size_t FunctionNameLength = 0;
	size_t TextStringLength = 0;
	BOOL ret = TRUE;
	wLogBinaryAppender* binaryAppender = NULL;

	if (!log || !appender || !message)
		return FALSE;

	binaryAppender = (wLogBinaryAppender*)appender;

	fp = binaryAppender->FileDescriptor;

	if (!fp)
		return FALSE;

	FileNameLength = strnlen(message->FileName, INT_MAX);
	FunctionNameLength = strnlen(message->FunctionName, INT_MAX);
	TextStringLength = strnlen(message->TextString, INT_MAX);

	MessageLength =
	    16 + (4 + FileNameLength + 1) + (4 + FunctionNameLength + 1) + (4 + TextStringLength + 1);

	if ((MessageLength > UINT32_MAX) || (FileNameLength > UINT32_MAX) ||
	    (FunctionNameLength > UINT32_MAX) || (TextStringLength > UINT32_MAX))
		return FALSE;

	s = Stream_New(NULL, MessageLength);
	if (!s)
		return FALSE;

	Stream_Write_UINT32(s, (UINT32)MessageLength);

	Stream_Write_UINT32(s, message->Type);
	Stream_Write_UINT32(s, message->Level);

	WINPR_ASSERT(message->LineNumber <= UINT32_MAX);
	Stream_Write_UINT32(s, (UINT32)message->LineNumber);

	Stream_Write_UINT32(s, (UINT32)FileNameLength);
	Stream_Write(s, message->FileName, FileNameLength + 1);

	Stream_Write_UINT32(s, (UINT32)FunctionNameLength);
	Stream_Write(s, message->FunctionName, FunctionNameLength + 1);

	Stream_Write_UINT32(s, (UINT32)TextStringLength);
	Stream_Write(s, message->TextString, TextStringLength + 1);

	Stream_SealLength(s);

	if (fwrite(Stream_Buffer(s), MessageLength, 1, fp) != 1)
		ret = FALSE;

	Stream_Free(s, TRUE);

	return ret;
}

static BOOL WLog_BinaryAppender_WriteDataMessage(WINPR_ATTR_UNUSED wLog* log,
                                                 WINPR_ATTR_UNUSED wLogAppender* appender,
                                                 WINPR_ATTR_UNUSED wLogMessage* message)
{
	return TRUE;
}

static BOOL WLog_BinaryAppender_WriteImageMessage(WINPR_ATTR_UNUSED wLog* log,
                                                  WINPR_ATTR_UNUSED wLogAppender* appender,
                                                  WINPR_ATTR_UNUSED wLogMessage* message)
{
	return TRUE;
}

static BOOL WLog_BinaryAppender_Set(wLogAppender* appender, const char* setting, void* value)
{
	wLogBinaryAppender* binaryAppender = (wLogBinaryAppender*)appender;

	/* Just check if the value string is longer than 0 */
	if (!value || (strnlen(value, 2) == 0))
		return FALSE;

	if (!strcmp("outputfilename", setting))
	{
		binaryAppender->FileName = _strdup((const char*)value);
		if (!binaryAppender->FileName)
			return FALSE;
	}
	else if (!strcmp("outputfilepath", setting))
	{
		binaryAppender->FilePath = _strdup((const char*)value);
		if (!binaryAppender->FilePath)
			return FALSE;
	}
	else
		return FALSE;

	return TRUE;
}

static void WLog_BinaryAppender_Free(wLogAppender* appender)
{
	wLogBinaryAppender* binaryAppender = NULL;
	if (appender)
	{
		binaryAppender = (wLogBinaryAppender*)appender;
		free(binaryAppender->FileName);
		free(binaryAppender->FilePath);
		free(binaryAppender->FullFileName);
		free(binaryAppender);
	}
}

wLogAppender* WLog_BinaryAppender_New(WINPR_ATTR_UNUSED wLog* log)
{
	wLogBinaryAppender* BinaryAppender = NULL;

	BinaryAppender = (wLogBinaryAppender*)calloc(1, sizeof(wLogBinaryAppender));
	if (!BinaryAppender)
		return NULL;

	BinaryAppender->Type = WLOG_APPENDER_BINARY;
	BinaryAppender->Open = WLog_BinaryAppender_Open;
	BinaryAppender->Close = WLog_BinaryAppender_Close;
	BinaryAppender->WriteMessage = WLog_BinaryAppender_WriteMessage;
	BinaryAppender->WriteDataMessage = WLog_BinaryAppender_WriteDataMessage;
	BinaryAppender->WriteImageMessage = WLog_BinaryAppender_WriteImageMessage;
	BinaryAppender->Free = WLog_BinaryAppender_Free;
	BinaryAppender->Set = WLog_BinaryAppender_Set;

	return (wLogAppender*)BinaryAppender;
}
