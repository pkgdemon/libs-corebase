/* CFStringTokenizer.h

   Copyright (C) 2025 Free Software Foundation, Inc.

   Written by: NextBSD Contributors
   Date: July, 2025

   This file is part of GNUstep CoreBase Library.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; see the file COPYING.LIB.
   If not, see <http://www.gnu.org/licenses/> or write to the
   Free Software Foundation, 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef __COREFOUNDATION_CFSTRINGTOKENIZER_H__
#define __COREFOUNDATION_CFSTRINGTOKENIZER_H__

#include <CoreFoundation/CFBase.h>
#include <CoreFoundation/CFLocale.h>
#include <CoreFoundation/CFString.h>
#include <CoreFoundation/CFArray.h>

CF_EXTERN_C_BEGIN

/** \ingroup CFStringTokenizerRef */
typedef struct __CFStringTokenizer * CFStringTokenizerRef;

/** \defgroup CFStringTokenizerRef CFStringTokenizer Reference
    \{
 */

/** Option flags for CFStringTokenizerCreate */
enum
{
  kCFStringTokenizerUnitWord                          = 0,
  kCFStringTokenizerUnitSentence                      = 1,
  kCFStringTokenizerUnitParagraph                     = 2,
  kCFStringTokenizerUnitLineBreak                     = 3,
  kCFStringTokenizerUnitWordBoundary                  = 4,
  kCFStringTokenizerAttributeLatinTranscription       = (1UL << 16),
  kCFStringTokenizerAttributeLanguage                 = (1UL << 17)
};

/** Token type flags returned by advance/goto functions */
typedef CFOptionFlags CFStringTokenizerTokenType;

enum
{
  kCFStringTokenizerTokenNone                         = 0,
  kCFStringTokenizerTokenNormal                       = (1UL << 0),
  kCFStringTokenizerTokenHasSubTokensMask             = (1UL << 1),
  kCFStringTokenizerTokenHasDerivedSubTokensMask      = (1UL << 2),
  kCFStringTokenizerTokenHasHasNumbersMask            = (1UL << 3),
  kCFStringTokenizerTokenHasNonLettersMask            = (1UL << 4),
  kCFStringTokenizerTokenIsCJWordMask                 = (1UL << 5)
};

/** \name Getting the CFStringTokenizer Type ID
    \{
 */
CF_EXPORT CFTypeID
CFStringTokenizerGetTypeID (void);
/** \} */

/** \name Creating a Tokenizer
    \{
 */
CF_EXPORT CFStringTokenizerRef
CFStringTokenizerCreate (CFAllocatorRef alloc, CFStringRef string,
  CFRange range, CFOptionFlags options, CFLocaleRef locale);
/** \} */

/** \name Setting the String
    \{
 */
CF_EXPORT void
CFStringTokenizerSetString (CFStringTokenizerRef tokenizer,
  CFStringRef string, CFRange range);
/** \} */

/** \name Navigating Tokens
    \{
 */
CF_EXPORT CFStringTokenizerTokenType
CFStringTokenizerAdvanceToNextToken (CFStringTokenizerRef tokenizer);

CF_EXPORT CFStringTokenizerTokenType
CFStringTokenizerGoToTokenAtIndex (CFStringTokenizerRef tokenizer,
  CFIndex index);
/** \} */

/** \name Getting Current Token Info
    \{
 */
CF_EXPORT CFRange
CFStringTokenizerGetCurrentTokenRange (CFStringTokenizerRef tokenizer);

CF_EXPORT CFTypeRef
CFStringTokenizerCopyCurrentTokenAttribute (CFStringTokenizerRef tokenizer,
  CFOptionFlags attribute);

CF_EXPORT CFIndex
CFStringTokenizerGetCurrentSubTokens (CFStringTokenizerRef tokenizer,
  CFRange *ranges, CFIndex maxRangeLength, CFMutableArrayRef derivedSubTokens);
/** \} */

/** \} */

CF_EXTERN_C_END

#endif /* __COREFOUNDATION_CFSTRINGTOKENIZER_H__ */
