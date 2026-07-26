/* CFStringTokenizer.c

   Copyright (C) 2025 Free Software Foundation, Inc.

   Written by: NextBSD Contributors
   Date: July, 2025

   This file is part of the GNUstep CoreBase Library.

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

#include "CoreFoundation/CFRuntime.h"
#include "CoreFoundation/CFBase.h"
#include "CoreFoundation/CFString.h"
#include "CoreFoundation/CFLocale.h"
#include "CoreFoundation/CFArray.h"
#include "CoreFoundation/CFStringTokenizer.h"
#include "GSPrivate.h"

#include <stdlib.h>
#include <string.h>

#if defined(HAVE_UNICODE_UBRK_H)
#include <unicode/ubrk.h>
#endif
#if defined(HAVE_UNICODE_USTRING_H)
#include <unicode/ustring.h>
#endif
#if defined(HAVE_ICU_H)
#include <icu.h>
#endif

#define BUFFER_SIZE 256

struct __CFStringTokenizer
{
  CFRuntimeBase        _parent;
  CFStringRef          _string;
  CFRange              _range;
  CFOptionFlags        _options;
  CFLocaleRef          _locale;
  CFRange              _currentTokenRange;
  CFStringTokenizerTokenType _currentTokenType;
#if defined(HAVE_UNICODE_UBRK_H) || defined(HAVE_ICU_H)
  UBreakIterator      *_breakIterator;
  UChar               *_textBuffer;
  CFIndex              _textLength;
#endif
};

static CFTypeID _kCFStringTokenizerTypeID = 0;

static void
CFStringTokenizerFinalize (CFTypeRef cf)
{
  CFStringTokenizerRef tok = (CFStringTokenizerRef)cf;

#if defined(HAVE_UNICODE_UBRK_H) || defined(HAVE_ICU_H)
  if (tok->_breakIterator)
    ubrk_close (tok->_breakIterator);
  if (tok->_textBuffer)
    free (tok->_textBuffer);
#endif
  if (tok->_string)
    CFRelease (tok->_string);
  if (tok->_locale)
    CFRelease (tok->_locale);
}

static const CFRuntimeClass CFStringTokenizerClass =
{
  0,
  "CFStringTokenizer",
  NULL,
  NULL,
  CFStringTokenizerFinalize,
  NULL,
  NULL,
  NULL,
  NULL
};

void CFStringTokenizerInitialize (void)
{
  _kCFStringTokenizerTypeID =
    _CFRuntimeRegisterClass (&CFStringTokenizerClass);
}

CFTypeID
CFStringTokenizerGetTypeID (void)
{
  return _kCFStringTokenizerTypeID;
}

#if defined(HAVE_UNICODE_UBRK_H) || defined(HAVE_ICU_H)
static UBreakIteratorType
CFStringTokenizerOptionsToUBrkType (CFOptionFlags options)
{
  /* Mask out attribute flags to get the unit type */
  CFOptionFlags unit = options & 0xFFFF;
  switch (unit)
    {
      case kCFStringTokenizerUnitWord:
      case kCFStringTokenizerUnitWordBoundary:
        return UBRK_WORD;
      case kCFStringTokenizerUnitSentence:
        return UBRK_SENTENCE;
      case kCFStringTokenizerUnitParagraph:
        /* ICU has no paragraph breaker; use sentence as fallback */
        return UBRK_SENTENCE;
      case kCFStringTokenizerUnitLineBreak:
        return UBRK_LINE;
      default:
        return UBRK_WORD;
    }
}

static void
CFStringTokenizerSetupBreakIterator (CFStringTokenizerRef tok)
{
  UErrorCode err = U_ZERO_ERROR;
  UBreakIteratorType brkType;
  const char *cLocale = NULL;
  char locBuf[ULOC_FULLNAME_CAPACITY];
  CFIndex len;
  CFIndex bufSize;

  /* Clean up previous iterator */
  if (tok->_breakIterator)
    {
      ubrk_close (tok->_breakIterator);
      tok->_breakIterator = NULL;
    }
  if (tok->_textBuffer)
    {
      free (tok->_textBuffer);
      tok->_textBuffer = NULL;
    }

  if (tok->_string == NULL)
    return;

  brkType = CFStringTokenizerOptionsToUBrkType (tok->_options);

  /* Get locale identifier */
  if (tok->_locale)
    {
      CFStringRef locId = CFLocaleGetIdentifier (tok->_locale);
      if (locId && CFStringGetCString (locId, locBuf,
            sizeof(locBuf), kCFStringEncodingASCII))
        cLocale = locBuf;
    }

  /* Extract UTF-16 text for the specified range */
  len = tok->_range.length;
  bufSize = len + 1;
  tok->_textBuffer = (UChar *)malloc (bufSize * sizeof(UChar));
  if (tok->_textBuffer == NULL)
    return;

  CFStringGetCharacters (tok->_string, tok->_range, tok->_textBuffer);
  tok->_textBuffer[len] = 0;
  tok->_textLength = len;

  /* Create break iterator */
  tok->_breakIterator = ubrk_open (brkType, cLocale,
    tok->_textBuffer, (int32_t)len, &err);

  if (U_FAILURE(err))
    {
      if (tok->_breakIterator)
        {
          ubrk_close (tok->_breakIterator);
          tok->_breakIterator = NULL;
        }
      free (tok->_textBuffer);
      tok->_textBuffer = NULL;
      tok->_textLength = 0;
    }

  /* Reset current token */
  tok->_currentTokenRange = CFRangeMake (0, 0);
  tok->_currentTokenType = kCFStringTokenizerTokenNone;
}
#endif /* HAVE_UNICODE_UBRK_H || HAVE_ICU_H */

CFStringTokenizerRef
CFStringTokenizerCreate (CFAllocatorRef alloc, CFStringRef string,
  CFRange range, CFOptionFlags options, CFLocaleRef locale)
{
  struct __CFStringTokenizer *new;

  if (string == NULL)
    return NULL;

  new = (struct __CFStringTokenizer *)_CFRuntimeCreateInstance (alloc,
    _kCFStringTokenizerTypeID,
    sizeof(struct __CFStringTokenizer) - sizeof(CFRuntimeBase),
    0);

  if (new)
    {
      new->_string = CFRetain (string);
      new->_range = range;
      new->_options = options;
      new->_currentTokenRange = CFRangeMake (0, 0);
      new->_currentTokenType = kCFStringTokenizerTokenNone;

      if (locale)
        new->_locale = (CFLocaleRef)CFRetain (locale);
      else
        new->_locale = NULL;

#if defined(HAVE_UNICODE_UBRK_H) || defined(HAVE_ICU_H)
      new->_breakIterator = NULL;
      new->_textBuffer = NULL;
      new->_textLength = 0;
      CFStringTokenizerSetupBreakIterator (new);
#endif
    }

  return (CFStringTokenizerRef)new;
}

void
CFStringTokenizerSetString (CFStringTokenizerRef tokenizer,
  CFStringRef string, CFRange range)
{
  if (tokenizer == NULL)
    return;

  if (tokenizer->_string)
    CFRelease (tokenizer->_string);

  if (string)
    tokenizer->_string = CFRetain (string);
  else
    tokenizer->_string = NULL;

  tokenizer->_range = range;
  tokenizer->_currentTokenRange = CFRangeMake (0, 0);
  tokenizer->_currentTokenType = kCFStringTokenizerTokenNone;

#if defined(HAVE_UNICODE_UBRK_H) || defined(HAVE_ICU_H)
  CFStringTokenizerSetupBreakIterator (tokenizer);
#endif
}

CFStringTokenizerTokenType
CFStringTokenizerAdvanceToNextToken (CFStringTokenizerRef tokenizer)
{
#if defined(HAVE_UNICODE_UBRK_H) || defined(HAVE_ICU_H)
  int32_t start;
  int32_t end;

  if (tokenizer == NULL || tokenizer->_breakIterator == NULL)
    return kCFStringTokenizerTokenNone;

  /* Determine the starting position for the next search */
  if (tokenizer->_currentTokenType == kCFStringTokenizerTokenNone)
    {
      /* First call: position at the beginning */
      start = ubrk_first (tokenizer->_breakIterator);
    }
  else
    {
      /* Continue from the end of the current token (relative to range) */
      start = (int32_t)(tokenizer->_currentTokenRange.location
                - tokenizer->_range.location
                + tokenizer->_currentTokenRange.length);
    }

  /* For word break iterators, skip non-word tokens */
  for (;;)
    {
      end = ubrk_following (tokenizer->_breakIterator, start);
      if (end == UBRK_DONE)
        {
          tokenizer->_currentTokenRange = CFRangeMake (0, 0);
          tokenizer->_currentTokenType = kCFStringTokenizerTokenNone;
          return kCFStringTokenizerTokenNone;
        }

      /* For word/word-boundary mode, skip whitespace/punctuation boundaries */
      if ((tokenizer->_options & 0xFFFF) == kCFStringTokenizerUnitWord
          || (tokenizer->_options & 0xFFFF) == kCFStringTokenizerUnitWordBoundary)
        {
          int32_t ruleStatus = ubrk_getRuleStatus (tokenizer->_breakIterator);
          if (ruleStatus == UBRK_WORD_NONE)
            {
              start = end;
              continue;
            }
        }

      break;
    }

  /* Store current token range (offset relative to the original string) */
  tokenizer->_currentTokenRange =
    CFRangeMake (tokenizer->_range.location + start,
                 end - start);
  tokenizer->_currentTokenType = kCFStringTokenizerTokenNormal;

  return kCFStringTokenizerTokenNormal;
#else
  return kCFStringTokenizerTokenNone;
#endif
}

CFStringTokenizerTokenType
CFStringTokenizerGoToTokenAtIndex (CFStringTokenizerRef tokenizer,
  CFIndex index)
{
#if defined(HAVE_UNICODE_UBRK_H) || defined(HAVE_ICU_H)
  int32_t relIdx;
  int32_t start;
  int32_t end;

  if (tokenizer == NULL || tokenizer->_breakIterator == NULL)
    return kCFStringTokenizerTokenNone;

  /* Convert absolute index to relative index within the text buffer */
  relIdx = (int32_t)(index - tokenizer->_range.location);
  if (relIdx < 0 || relIdx >= (int32_t)tokenizer->_textLength)
    {
      tokenizer->_currentTokenRange = CFRangeMake (0, 0);
      tokenizer->_currentTokenType = kCFStringTokenizerTokenNone;
      return kCFStringTokenizerTokenNone;
    }

  /* Find the boundaries around the given index */
  if (ubrk_isBoundary (tokenizer->_breakIterator, relIdx))
    start = relIdx;
  else
    start = ubrk_preceding (tokenizer->_breakIterator, relIdx);

  if (start == UBRK_DONE)
    start = 0;

  end = ubrk_following (tokenizer->_breakIterator, relIdx);
  if (end == UBRK_DONE)
    end = (int32_t)tokenizer->_textLength;

  /* For word mode, skip non-word tokens - search forward if needed */
  if ((tokenizer->_options & 0xFFFF) == kCFStringTokenizerUnitWord
      || (tokenizer->_options & 0xFFFF) == kCFStringTokenizerUnitWordBoundary)
    {
      int32_t ruleStatus = ubrk_getRuleStatus (tokenizer->_breakIterator);
      if (ruleStatus == UBRK_WORD_NONE)
        {
          /* The index falls in a non-word region; still return the range */
          tokenizer->_currentTokenRange =
            CFRangeMake (tokenizer->_range.location + start, end - start);
          tokenizer->_currentTokenType = kCFStringTokenizerTokenNone;
          return kCFStringTokenizerTokenNone;
        }
    }

  tokenizer->_currentTokenRange =
    CFRangeMake (tokenizer->_range.location + start, end - start);
  tokenizer->_currentTokenType = kCFStringTokenizerTokenNormal;

  return kCFStringTokenizerTokenNormal;
#else
  return kCFStringTokenizerTokenNone;
#endif
}

CFRange
CFStringTokenizerGetCurrentTokenRange (CFStringTokenizerRef tokenizer)
{
  if (tokenizer == NULL
      || tokenizer->_currentTokenType == kCFStringTokenizerTokenNone)
    return CFRangeMake (kCFNotFound, 0);

  return tokenizer->_currentTokenRange;
}

CFTypeRef
CFStringTokenizerCopyCurrentTokenAttribute (CFStringTokenizerRef tokenizer,
  CFOptionFlags attribute)
{
  CFStringRef sub;

  if (tokenizer == NULL || tokenizer->_string == NULL
      || tokenizer->_currentTokenType == kCFStringTokenizerTokenNone)
    return NULL;

  if (tokenizer->_currentTokenRange.length <= 0)
    return NULL;

  /* For kCFStringTokenizerAttributeLatinTranscription, return the substring
     itself as a basic implementation.  A full implementation would use
     ICU transliteration. */
  sub = CFStringCreateWithSubstring (kCFAllocatorDefault,
    tokenizer->_string, tokenizer->_currentTokenRange);

  return (CFTypeRef)sub;
}

CFIndex
CFStringTokenizerGetCurrentSubTokens (CFStringTokenizerRef tokenizer,
  CFRange *ranges, CFIndex maxRangeLength, CFMutableArrayRef derivedSubTokens)
{
  /* Stub implementation - not critical for WebKit */
  (void)tokenizer;
  (void)ranges;
  (void)maxRangeLength;
  (void)derivedSubTokens;
  return 0;
}
