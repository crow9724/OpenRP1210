//------------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at http://mozilla.org/MPL/2.0/.
//------------------------------------------------------------------------------
#include "OpenRP1210/util/Ini.h"
#include "OpenRP1210/Common.h"
#include "OpenRP1210/platform/Platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <limits.h>
#include <assert.h>

#if defined _WIN32 || defined _WIN64
	#define STRICMP _stricmp
#else
	#include <strings.h>
	#define STRICMP strcasecmp
#endif

#define CARRIGE_RETURN 0x0D
#define LINE_FEED 0x0A

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
typedef struct S_Offset_t
{
	int64_t Start;
	int64_t End;
	int64_t Count;
	int64_t Distance;
}S_Offset;

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
typedef enum E_IniSymbolType_t
{
	SYM_Comment,
	SYM_Key,
	SYM_KeyName,
	SYM_SectionName,
}E_IniSymbolType;

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
typedef struct S_IniSymbol_t
{
	E_IniSymbolType Type;
	struct S_IniSymbol_t *NextSymbol;
	struct S_IniSymbol_t *PrevSymbol;
	struct S_IniSymbol_t *Children;
	char *Value;
}S_IniSymbol;

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
typedef struct S_EnumKey_t
{
	S_IniSymbol *Key;
	struct S_EnumKey_t *NextKey;
}S_EnumKey;

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
typedef struct S_Ini_t
{
	S_IniConfig Config;
	S_IniSymbol *FirstSection;
	S_IniSymbol *LastSection;
}S_Ini;

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
typedef struct S_ParseContext_t
{
	unsigned char *Data;
	int64_t DataLength;

	int64_t CurrentOffset;
	unsigned int CurrentLine;

	unsigned int SymbolCount;
	S_IniConfig Config;
}S_ParseContext;

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
int rp_StrCmp(const char *s1, const char *s2, bool caseSensitive)
{
	return caseSensitive ? strcmp(s1, s2) : STRICMP(s1, s2);
}

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
ORP_ERR rp_SetParseError(S_ParseContext *context, ORP_ERR error, bool errorParsing, const char *msg, ...)
{
	va_list args;
	va_start(args, msg);

	rp_SetLastErrorVA(error, msg, &args);

	if(errorParsing)
		rp_AppendLastError(" Line: %d.", context->CurrentLine + 1);

	va_end(args);

	return error;
}

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
void rp_SetOffset(S_Offset *offset, int64_t start, int64_t end)
{
	offset->Start = start;
	offset->End = end;
	offset->Distance = end - start;
	offset->Count = offset->Distance + 1;
}

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
S_Offset rp_MakeOffset(int64_t start, int64_t end)
{
	S_Offset offset;
	rp_SetOffset(&offset, start, end);

	return offset;
}

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
S_IniConfig rp_CreateDefaultConfig(void)
{
	S_IniConfig config = { 0 };
	config.DuplicateMode = IniDuplicateMode_Ignore;
	config.TrimKeyValues = true;
	config.KeysCaseInsensitive = true;

	return config;
}

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
void rp_InitializeConfig(S_IniConfig *configIn, S_IniConfig *configOut)
{
	if(configIn)
		memcpy(configOut, configIn, sizeof(S_IniConfig));
	else
		*configOut = rp_CreateDefaultConfig();
}

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
S_Ini *rp_CreateIni(S_IniConfig *config, S_IniSymbol *firstSection, S_IniSymbol *lastSection, ORP_ERR error, const char *errorText)
{
	S_Ini *ini = NULL;

	if(error == ORP_ERR_NO_ERROR)
	{
		ini = rp_mallocZ(sizeof(S_Ini));
		if(ini)
		{
			ini->FirstSection = firstSection;
			ini->LastSection = lastSection;

			rp_InitializeConfig(config, &ini->Config);
		}
	}
	else if(errorText)
		rp_SetLastError(error, errorText);

	return ini;
}

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
void rp_InitializeContext(S_ParseContext *context, S_IniConfig *config, unsigned char *data, int64_t dataLength)
{
	memset(context, 0, sizeof(S_ParseContext));
	context->Data = data;
	context->DataLength = dataLength;

	rp_InitializeConfig(config, &context->Config);
}

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
bool rp_IsPrintable(char c)
{
	return c >= 32 && c <= 126;
}

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
int64_t rp_FindChar(S_ParseContext *context, char c, int64_t startOffset, int64_t endOffset, bool breakOnNonPrintable)
{
	if(endOffset < 0)
		endOffset = context->DataLength;

	while(startOffset < endOffset)
	{
		if(breakOnNonPrintable && !rp_IsPrintable(context->Data[startOffset]))
			break;
		else
		{
			if(context->Data[startOffset] != c && rp_IsPrintable(context->Data[startOffset]))
				startOffset++;
			else
				break;
		}
	}

	return startOffset >= context->DataLength ? -1 : (context->Data[startOffset] == c ? startOffset : -1);
}

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
void rp_Trim(S_ParseContext *context, S_Offset *offset)
{
	int64_t so, eo;

	for(so = offset->Start; so < offset->End; so++)
	{
		if(context->Data[so] != ' ')
			break;
	}

	for(eo = offset->End; eo > so; eo--)
	{
		if(context->Data[eo] != ' ')
			break;
	}

	rp_SetOffset(offset, so, eo);
}

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
const char rp_PeekChar(S_ParseContext *context)
{
	return context->Data[context->CurrentOffset];
}

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
bool rp_IsNewLine(S_ParseContext *context, int64_t offset, unsigned char *newLineLength)
{
	unsigned int count = 0;

	if(offset < context->DataLength && context->Data[offset] == CARRIGE_RETURN)
	{
		count++;
		offset++;
	}
	if(offset < context->DataLength && context->Data[offset] == LINE_FEED)
		count++;

	if(count > 0 && newLineLength)
		*newLineLength = count;

	return count > 0;
}

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
bool rp_FindNewLine(S_ParseContext *context, int64_t startOffset, S_Offset *newLineOffset)
{
	bool found = false;
	unsigned char lineCharCount = 0;

	while(!found && startOffset < context->DataLength)
	{
		if(rp_IsNewLine(context, startOffset, &lineCharCount))
			found = true;
		else
			startOffset++;
	}

	if(found && newLineOffset)
		rp_SetOffset(newLineOffset, startOffset, startOffset + lineCharCount - 1);

	return found;
}

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
void rp_NextLine(S_ParseContext *context)
{
	S_Offset offset;
	if(rp_FindNewLine(context, context->CurrentOffset, &offset))
	{
		context->CurrentOffset = offset.End + 1;
		context->CurrentLine++;
	}
	else
		context->CurrentOffset = context->DataLength;
}

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
void rp_SkipWhiteSpace(S_ParseContext *context)
{
	unsigned char lineCharCount = 0;

	while(context->CurrentOffset < context->DataLength)
	{
		if(rp_IsNewLine(context, context->CurrentOffset, &lineCharCount))
		{
			context->CurrentOffset += lineCharCount;
			context->CurrentLine++;
		}
		else
		{
			if(!rp_IsPrintable(rp_PeekChar(context)) || rp_PeekChar(context) == ' ')
				context->CurrentOffset++;
			else
				break;
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
S_IniSymbol *rp_LastSymbol(S_IniSymbol *symbol)
{
	S_IniSymbol *s = symbol;

	while(s->NextSymbol)
		s = s->NextSymbol;

	return s;
}

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
S_IniSymbol *rp_CreateSymbol(E_IniSymbolType symbolType, char *iniData, int64_t startOffset, int64_t endOffset)
{
	S_IniSymbol *symbol = rp_malloc(sizeof(S_IniSymbol));
	if(symbol)
	{
		assert(endOffset - startOffset + 1 <= SIZE_MAX);
		size_t len = iniData == NULL ? 0 : (size_t)(endOffset - startOffset + 1);

		memset(symbol, 0, sizeof(S_IniSymbol));
		symbol->Type = symbolType;
		symbol->Value = rp_malloc(len + 1);

		if(symbol->Value)
		{
			if(iniData)
				strncpy(symbol->Value, iniData + startOffset, len);

			symbol->Value[len] = 0;
		}
		else
		{
			rp_free(symbol);  // couldn't fully construct the symbol
			symbol = NULL;
		}
	}

	return symbol;
}

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
void rp_DestroySymbol(S_IniSymbol *symbol)
{
	if(symbol->Children)
		rp_DestroySymbol(symbol->Children);

	if(symbol->NextSymbol)
		rp_DestroySymbol(symbol->NextSymbol);

	rp_free(symbol->Value);
	rp_free(symbol);
}

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
S_IniSymbol *rp_CreateEmptySymbol(E_IniSymbolType symbolType)
{
	return rp_CreateSymbol(symbolType, NULL, 0, 0);
}

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
S_IniSymbol *rp_AddSymbol(S_ParseContext *context, S_IniSymbol **currentSymbol, S_IniSymbol *symbol)
{
	if(symbol)
	{
		if(*currentSymbol != NULL)
		{
			S_IniSymbol *lastSym = rp_LastSymbol(*currentSymbol);
			lastSym->NextSymbol = symbol;
			symbol->PrevSymbol = lastSym;
		}
		else
			*currentSymbol = symbol;

		context->SymbolCount++;
	}

	return symbol;
}

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
S_IniSymbol *rp_CreateAddSymbol(S_ParseContext *context, S_IniSymbol **currentSymbol, E_IniSymbolType symbolType, int64_t startOffset, int64_t endOffset)
{
	S_IniSymbol *symbol = rp_CreateSymbol(symbolType, (char *)context->Data, startOffset, endOffset);
	return rp_AddSymbol(context, currentSymbol, symbol);
}

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
bool rp_ContainsKey(S_EnumKey *enumKey, const char *key, bool caseSensitive)
{
	S_EnumKey *e = enumKey;
	
	if(e->Key)
	{
		while(e)
		{
			if(rp_StrCmp(e->Key->Value, key, caseSensitive) == 0)
				break;
			else
				e = e->NextKey;
		}

		return e ? true : false;
	}
	else
		return false;
}

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
ORP_ERR rp_FindSection(S_Ini *ini, const char *section, S_IniSymbol **symbolOut)
{
	ORP_ERR r = ORP_ERR_NO_ERROR;
	S_IniSymbol *sectionSym = ini->Config.DuplicateMode == IniDuplicateMode_Ignore ? ini->FirstSection : ini->LastSection;

	while(sectionSym)
	{
		if(rp_StrCmp(sectionSym->Value, section, !ini->Config.KeysCaseInsensitive) == 0)
			break;
		else
			sectionSym = ini->Config.DuplicateMode == IniDuplicateMode_Ignore ? sectionSym->NextSymbol : sectionSym->PrevSymbol;
	}

	if(sectionSym && sectionSym->Type == SYM_SectionName)
		*symbolOut = sectionSym;
	else
		r = rp_SetLastError(ORP_ERR_INI_SECTION_NOT_FOUND, " Section name = %s.", section);

	return r;
}

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
ORP_ERR rp_FindKey(S_Ini *ini, const char *section, const char *key, S_IniSymbol **symbolOut)
{
	S_IniSymbol *sectionSym;
	ORP_ERR r = rp_FindSection(ini, section, &sectionSym);

	if(r == ORP_ERR_NO_ERROR)
	{
		S_IniSymbol *keySym = ini->Config.DuplicateMode == IniDuplicateMode_Ignore ? sectionSym->Children : rp_LastSymbol(sectionSym->Children);

		while(keySym)
		{
			if(rp_StrCmp(keySym->Value, key, !ini->Config.KeysCaseInsensitive) == 0)
				break;
			else
				keySym = ini->Config.DuplicateMode == IniDuplicateMode_Ignore ? keySym->NextSymbol : keySym->PrevSymbol;
		}

		if(keySym && keySym->Type == SYM_KeyName)
		{
			S_IniSymbol *valueSym = keySym->Children;

			if(valueSym && valueSym->Type == SYM_Key)
				*symbolOut = valueSym;
			else
				r = rp_SetLastError(ORP_ERR_INI_MISSING_KEYVALUE, " Section name = %s, Key name = %s.", section, key);
		}
		else
			r = rp_SetLastError(ORP_ERR_INI_KEYNOTFOUND, " Section name = %s, Key name = %s.", section, key);
	}

	return r;
}

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
S_IniSymbol *rp_ReadSection(S_ParseContext *context, S_IniSymbol *currentSymbol)
{
	S_IniSymbol *symbol = NULL;
	int64_t bracketOffset = rp_FindChar(context, ']', context->CurrentOffset, -1, true);

	if(bracketOffset < 0)
		rp_SetParseError(context, ORP_ERR_INI_INVALID_SECTION, true, "");
	else
	{
		symbol = rp_CreateAddSymbol(context, &currentSymbol, SYM_SectionName, context->CurrentOffset + 1, bracketOffset - 1);

		if(symbol)
			context->CurrentOffset = bracketOffset + 1;
		else
			rp_SetParseError(context, ORP_ERR_MEM_ALLOC, false, "");
	}

	return symbol;
}

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
void rp_ReadKey(S_ParseContext *context, S_IniSymbol *parentSymbol)
{
	// TODO: Doesn't handle case of comment in key name (ie, "Version;=4")

	int64_t eqOffset = rp_FindChar(context, '=', context->CurrentOffset, -1, true);

	if(eqOffset < 0)
		rp_SetParseError(context, ORP_ERR_INI_MISSING_KEYVALUE, true, ""); // missing "=" symbol
	else if(eqOffset - context->CurrentOffset == 0)
		rp_SetParseError(context, ORP_ERR_INI_INVALID_KEYNAME, true, "");  // "=" present, but no key name precedes it
	else
	{
		S_Offset keyOffset = rp_MakeOffset(context->CurrentOffset, eqOffset - 1);
		rp_Trim(context, &keyOffset);

		S_IniSymbol *symbol = rp_CreateAddSymbol(context, &parentSymbol->Children, SYM_KeyName, keyOffset.Start, keyOffset.End);

		if(symbol)
		{
			S_Offset valueOffset;
			rp_SetOffset(&valueOffset, eqOffset + 1, rp_FindNewLine(context, eqOffset + 1, &valueOffset) ? valueOffset.Start - 1 : context->DataLength - 1);

			if(context->Config.TrimKeyValues)
				rp_Trim(context, &valueOffset);

			if(valueOffset.Count > 0 && rp_IsPrintable(context->Data[valueOffset.Start]) && context->Data[valueOffset.Start] != ' ')
				symbol = rp_CreateAddSymbol(context, &symbol->Children, SYM_Key, valueOffset.Start, valueOffset.End);		 
			else
				symbol = rp_AddSymbol(context, &symbol->Children, rp_CreateEmptySymbol(SYM_Key)); // blank key i.e. KEY=

			if(symbol)
				context->CurrentOffset = valueOffset.End + 1;
			else
				rp_SetParseError(context, ORP_ERR_MEM_ALLOC, false, "");
		}
		else
			rp_SetParseError(context, ORP_ERR_MEM_ALLOC, false, "");
	}
}

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
S_Ini *rp_ParseIni(S_IniConfig *config, unsigned char *data, int64_t dataLength)
{
	S_IniSymbol *lastSection = NULL, *firstSection = NULL;
	S_ParseContext context;

	rp_InitializeContext(&context, config, data, dataLength);
	rp_SkipWhiteSpace(&context);

	while(context.CurrentOffset < dataLength && rpGetLastError() == ORP_ERR_NO_ERROR)
	{
		char c = rp_PeekChar(&context);

		if(c == ';')
			rp_NextLine(&context);
		else
		{
			if(c == '[')
				lastSection = rp_ReadSection(&context, lastSection);
			else if(lastSection && rp_IsPrintable(c))
				rp_ReadKey(&context, lastSection);
			else
				rp_SetParseError(&context, ORP_ERR_INI_INVALID_SYMBOL, true, "");

			if(lastSection != NULL && firstSection == NULL)
				firstSection = lastSection;
		}

		rp_SkipWhiteSpace(&context);
	}

	return rp_CreateIni(&context.Config, firstSection, lastSection, rpGetLastError(), NULL);
}

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
void rp_IniClose(ORP_HANDLE hIni)
{
	assert(hIni != NULL);
	S_Ini *ini = rp_HandleToTarget(hIni);

	rp_DestroySymbol(ini->FirstSection);
	rp_free(ini);
}

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
ORP_HANDLE rp_IniOpen(const char *iniPath, S_IniConfig *config)
{
	assert(iniPath != NULL);
	rp_ClearLastError();

	S_Ini *ini = NULL;
	enum { ERRLEN = 512 };
	char errBuf[ERRLEN];

	memset(errBuf, 0, ERRLEN);
	uint64_t fileSizeFull = rp_GetFileSize(iniPath);

	if(rpGetLastError() == ORP_ERR_NO_ERROR)
	{
		if(fileSizeFull >= 0 && fileSizeFull <= SIZE_MAX)
		{
			size_t fileSize = (size_t)fileSizeFull;
			unsigned char *data = rp_malloc(fileSize);

			if(data)
			{
				FILE *fp = fopen(iniPath, "rb");

				if(fp)
				{
					if(fread(data, 1, fileSize, fp) != fileSize)
						ini = rp_CreateIni(config, NULL, NULL, ORP_ERR_FILE_IO, "Failed to read from file.");
					else
						ini = rp_ParseIni(config, data, fileSize);

					fclose(fp);
				}
				else
					ini = rp_CreateIni(config, NULL, NULL, ORP_ERR_FILE_NOT_FOUND, rp_Concat(errBuf, ERRLEN, "File: %s", iniPath));

				rp_free(data);
			}
			else
				ini = rp_CreateIni(config, NULL, NULL, ORP_ERR_MEM_ALLOC, NULL);
		}
		else
			ini = rp_CreateIni(config, NULL, NULL, ORP_ERR_FILE_NOT_FOUND, rp_Concat(errBuf, ERRLEN, "File: %s", iniPath));
	}

	return ini ? rp_CreateHandle(ini, rp_IniClose) : NULL;
}

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
bool rp_IniHasKey(ORP_HANDLE hIni, const char *section, const char *key)
{
	assert(hIni != NULL && section != NULL && key != NULL);
	rp_ClearLastError();

	S_Ini *ini = rp_HandleToTarget(hIni);
	S_IniSymbol *keySym;

	return rp_FindKey(ini, section, key, &keySym) == ORP_ERR_NO_ERROR ? true : false;
}

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
bool rp_IniHasSection(ORP_HANDLE hIni, const char *section)
{
	assert(hIni != NULL && section != NULL);
	rp_ClearLastError();

	S_Ini *ini = rp_HandleToTarget(hIni);
	S_IniSymbol *sectionSym;

	return rp_FindSection(ini, section, &sectionSym) == ORP_ERR_NO_ERROR ? true : false;
}

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
ORP_ERR rp_IniReadKey(ORP_HANDLE hIni, const char *section, const char *key, char *dest, size_t length)
{
	assert(hIni != NULL && section != NULL && key != NULL);
	ORP_ERR r = rp_ClearLastError();

	if(length > 0)
	{
		S_Ini *ini = rp_HandleToTarget(hIni);
		S_IniSymbol *keySym;

		r = rp_FindKey(ini, section, key, &keySym);
		if(r == ORP_ERR_NO_ERROR)
		{
			strncpy(dest, keySym->Value, length);
			if(length <= strlen(keySym->Value))
				dest[length - 1] = 0;
		}
	}

	return r;
}

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
ORP_ERR rp_IniReadKeyLength(ORP_HANDLE hIni, const char *section, const char *key, size_t *length)
{
	assert(hIni != NULL && section != NULL && key != NULL);
	rp_ClearLastError();

	S_Ini *ini = rp_HandleToTarget(hIni);
	S_IniSymbol *keySym;

	ORP_ERR r = rp_FindKey(ini, section, key, &keySym);
	if(r == ORP_ERR_NO_ERROR && length != NULL)
		*length = strlen(keySym->Value);

	return r;
}

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
ORP_ERR rp_IniReadInt(ORP_HANDLE hIni, const char *section, const char *key, int *value)
{
	assert(hIni != NULL && section != NULL && key != NULL);
	rp_ClearLastError();

	S_Ini *ini = rp_HandleToTarget(hIni);
	S_IniSymbol *keySym;

	ORP_ERR r = rp_FindKey(ini, section, key, &keySym);
	if(r == ORP_ERR_NO_ERROR && value != NULL)
	{
		char *e = NULL;
		long v = strtol(keySym->Value, &e, 0);

		if(e - keySym->Value != strlen(keySym->Value))
			r = rp_SetLastError(ORP_ERR_BAD_ARG, " Not an integer: %s", keySym->Value);
		else if(errno == ERANGE)
			r = rp_SetLastError(ORP_ERR_BAD_RANGE, " Value = %s", keySym->Value);
		else if(v > INT_MAX)
			r = rp_SetLastError(ORP_ERR_BAD_RANGE, " Value = %s", keySym->Value);
		else
			*value = v;
	}

	return r;
}

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
ORP_ERR rp_IniEnumerateKeys(ORP_HANDLE hIni, const char *section, ENUM_KEYS_CALLBACK callback, void *userPtr)
{
	// this is a bit hacky
	// problem is, duplicate sections/keys are not removed during parsing
	// this is not a problem in Ini_Read* functions b/c they alter foward/reverse key traversing and stop on the first match
	// Enumerate can't stop on the first match and needs a way to filter out duplicates
	// work-around is to build a list using S_EnumKey that filters out duplicates based on S_IniConfig::DuplicateMode

	assert(hIni != NULL && section != NULL);
	rp_ClearLastError();

	S_Ini *ini = rp_HandleToTarget(hIni);
	S_IniSymbol *sectionSym;
	ORP_ERR r = rp_FindSection(hIni, section, &sectionSym);

	if(r == ORP_ERR_NO_ERROR)
	{
		S_IniSymbol *keySym = ini->Config.DuplicateMode == IniDuplicateMode_Ignore ? sectionSym->Children : rp_LastSymbol(sectionSym->Children);
		S_EnumKey enumKeyStart = { 0 };
		S_EnumKey *ek = &enumKeyStart;

		while(keySym && r == ORP_ERR_NO_ERROR)
		{
			// traverse the list of keys and keep track of non-duplicate keys using S_EnumKey
			if(keySym->Type == SYM_KeyName)
			{
				if(!rp_ContainsKey(&enumKeyStart, keySym->Value, !ini->Config.KeysCaseInsensitive)) // No Duplicates
				{
					if(ek->Key)
					{
						ek->NextKey = rp_malloc(sizeof(S_EnumKey));
						if(!ek->NextKey)
							r = rp_SetLastError(ORP_ERR_MEM_ALLOC, "");
						else
						{
							ek = ek->NextKey;
							ek->Key = keySym;
							ek->NextKey = NULL;
						}
					}
					else
						ek->Key = keySym;
				}
			}

			keySym = ini->Config.DuplicateMode == IniDuplicateMode_Ignore ? keySym->NextSymbol : keySym->PrevSymbol;
		}

		// all keys, with duplicates stripped, can now be referenced from enumKeyStart
		// walk the list and invoke the user callback function for each
		ek = &enumKeyStart;
		bool keepGoing = true;

		if(ek->Key)
		{
			while(r == ORP_ERR_NO_ERROR && ek && keepGoing)
			{
				if(ek->Key->Children && ek->Key->Children->Type == SYM_Key)
					keepGoing = callback(ek->Key->Value, ek->Key->Children->Value, userPtr);
				else
					r = rp_SetLastError(ORP_ERR_INI_MISSING_KEYVALUE, " Section name = %s, Key name = %s.", section, ek->Key);

				ek = ek->NextKey;
			}

			ek = enumKeyStart.NextKey;
			while(ek)
			{
				S_EnumKey *tempEk = ek->NextKey;
				rp_free(ek);
				ek = tempEk;
			}
		}
	}

	return r;
}