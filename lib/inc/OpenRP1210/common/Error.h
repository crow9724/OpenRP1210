//------------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at http://mozilla.org/MPL/2.0/.
//------------------------------------------------------------------------------
#ifndef OPENRP1210_ERROR_H__
#define OPENRP1210_ERROR_H__

#include "OpenRP1210/OpenRP1210.h"
#include <stdarg.h>

ORP_ERR rp_ClearLastError(void);

ORP_ERR rp_SetLastError(ORP_ERR error, const char *msg, ...);
ORP_ERR rp_SetLastErrorVA(ORP_ERR error, const char *msg, va_list *args);

void rp_AppendLastError(const char *msg, ...);

#endif
