/* CFPreferences.m

   Copyright (C) 2025 Free Software Foundation, Inc.

   Written by: Jamie Plummer
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

#import <Foundation/NSUserDefaults.h>
#import <Foundation/NSString.h>
#import <Foundation/NSArray.h>
#import <Foundation/NSDictionary.h>
#import <Foundation/NSValue.h>

#include "CoreFoundation/CFBase.h"
#include "CoreFoundation/CFPreferences.h"
#include "CoreFoundation/CFString.h"
#include "CoreFoundation/CFNumber.h"

#include "GSPrivate.h"

CONST_STRING_DECL(kCFPreferencesAnyApplication,
  "kCFPreferencesAnyApplication");
CONST_STRING_DECL(kCFPreferencesCurrentApplication,
  "kCFPreferencesCurrentApplication");
CONST_STRING_DECL(kCFPreferencesAnyHost,
  "kCFPreferencesAnyHost");
CONST_STRING_DECL(kCFPreferencesCurrentHost,
  "kCFPreferencesCurrentHost");
CONST_STRING_DECL(kCFPreferencesAnyUser,
  "kCFPreferencesAnyUser");
CONST_STRING_DECL(kCFPreferencesCurrentUser,
  "kCFPreferencesCurrentUser");

static NSUserDefaults *
_CFPreferencesDefaultsForAppID (CFStringRef applicationID)
{
  if (applicationID == kCFPreferencesCurrentApplication)
    return [NSUserDefaults standardUserDefaults];

  if (applicationID == kCFPreferencesAnyApplication)
    return [NSUserDefaults standardUserDefaults];

  /* For a specific application ID, use initWithSuiteName: */
  return [[[NSUserDefaults alloc]
    initWithSuiteName: (NSString *)applicationID] autorelease];
}

CFPropertyListRef
CFPreferencesCopyAppValue (CFStringRef key, CFStringRef applicationID)
{
  NSUserDefaults *defaults;
  id value;

  if (key == NULL || applicationID == NULL)
    return NULL;

  defaults = _CFPreferencesDefaultsForAppID (applicationID);
  value = [defaults objectForKey: (NSString *)key];

  if (value == nil)
    return NULL;

  return (CFPropertyListRef)CFRetain ((CFTypeRef)value);
}

Boolean
CFPreferencesGetAppBooleanValue (CFStringRef key, CFStringRef applicationID,
  Boolean *keyExistsAndHasValidFormat)
{
  CFPropertyListRef value;

  if (key == NULL || applicationID == NULL)
    {
      if (keyExistsAndHasValidFormat)
        *keyExistsAndHasValidFormat = false;
      return false;
    }

  value = CFPreferencesCopyAppValue (key, applicationID);
  if (value == NULL)
    {
      if (keyExistsAndHasValidFormat)
        *keyExistsAndHasValidFormat = false;
      return false;
    }

  /* Check if the value is a CFBoolean or CFNumber */
  if (CFGetTypeID (value) == CFBooleanGetTypeID ())
    {
      Boolean result = CFBooleanGetValue ((CFBooleanRef)value);
      CFRelease (value);
      if (keyExistsAndHasValidFormat)
        *keyExistsAndHasValidFormat = true;
      return result;
    }

  if (CFGetTypeID (value) == CFNumberGetTypeID ())
    {
      int intVal = 0;
      CFNumberGetValue ((CFNumberRef)value, kCFNumberIntType, &intVal);
      CFRelease (value);
      if (keyExistsAndHasValidFormat)
        *keyExistsAndHasValidFormat = true;
      return intVal != 0;
    }

  /* Check if the value is a string that represents a boolean */
  if (CFGetTypeID (value) == CFStringGetTypeID ())
    {
      Boolean result = false;
      Boolean valid = false;
      CFStringRef str = (CFStringRef)value;

      if (CFStringCompare (str, CFSTR("true"), kCFCompareCaseInsensitive)
            == kCFCompareEqualTo
          || CFStringCompare (str, CFSTR("yes"), kCFCompareCaseInsensitive)
            == kCFCompareEqualTo
          || CFStringCompare (str, CFSTR("1"), 0)
            == kCFCompareEqualTo)
        {
          result = true;
          valid = true;
        }
      else if (CFStringCompare (str, CFSTR("false"), kCFCompareCaseInsensitive)
                 == kCFCompareEqualTo
               || CFStringCompare (str, CFSTR("no"), kCFCompareCaseInsensitive)
                 == kCFCompareEqualTo
               || CFStringCompare (str, CFSTR("0"), 0)
                 == kCFCompareEqualTo)
        {
          result = false;
          valid = true;
        }

      CFRelease (value);
      if (keyExistsAndHasValidFormat)
        *keyExistsAndHasValidFormat = valid;
      return result;
    }

  CFRelease (value);
  if (keyExistsAndHasValidFormat)
    *keyExistsAndHasValidFormat = false;
  return false;
}

CFIndex
CFPreferencesGetAppIntegerValue (CFStringRef key, CFStringRef applicationID,
  Boolean *keyExistsAndHasValidFormat)
{
  CFPropertyListRef value;

  if (key == NULL || applicationID == NULL)
    {
      if (keyExistsAndHasValidFormat)
        *keyExistsAndHasValidFormat = false;
      return 0;
    }

  value = CFPreferencesCopyAppValue (key, applicationID);
  if (value == NULL)
    {
      if (keyExistsAndHasValidFormat)
        *keyExistsAndHasValidFormat = false;
      return 0;
    }

  if (CFGetTypeID (value) == CFNumberGetTypeID ())
    {
      CFIndex result = 0;
      CFNumberGetValue ((CFNumberRef)value, kCFNumberCFIndexType, &result);
      CFRelease (value);
      if (keyExistsAndHasValidFormat)
        *keyExistsAndHasValidFormat = true;
      return result;
    }

  if (CFGetTypeID (value) == CFStringGetTypeID ())
    {
      SInt32 intVal = CFStringGetIntValue ((CFStringRef)value);
      CFRelease (value);
      if (keyExistsAndHasValidFormat)
        *keyExistsAndHasValidFormat = true;
      return (CFIndex)intVal;
    }

  CFRelease (value);
  if (keyExistsAndHasValidFormat)
    *keyExistsAndHasValidFormat = false;
  return 0;
}

void
CFPreferencesSetAppValue (CFStringRef key, CFPropertyListRef value,
  CFStringRef applicationID)
{
  NSUserDefaults *defaults;

  if (key == NULL || applicationID == NULL)
    return;

  defaults = _CFPreferencesDefaultsForAppID (applicationID);

  if (value == NULL)
    [defaults removeObjectForKey: (NSString *)key];
  else
    [defaults setObject: (id)value forKey: (NSString *)key];
}

Boolean
CFPreferencesAppSynchronize (CFStringRef applicationID)
{
  NSUserDefaults *defaults;

  if (applicationID == NULL)
    return false;

  defaults = _CFPreferencesDefaultsForAppID (applicationID);
  return [defaults synchronize] ? true : false;
}

void
CFPreferencesAddSuitePreferencesToApp (CFStringRef applicationID,
  CFStringRef suiteID)
{
  NSUserDefaults *defaults;

  if (applicationID == NULL || suiteID == NULL)
    return;

  defaults = _CFPreferencesDefaultsForAppID (applicationID);
  [defaults addSuiteNamed: (NSString *)suiteID];
}

void
CFPreferencesRemoveSuitePreferencesFromApp (CFStringRef applicationID,
  CFStringRef suiteID)
{
  NSUserDefaults *defaults;

  if (applicationID == NULL || suiteID == NULL)
    return;

  defaults = _CFPreferencesDefaultsForAppID (applicationID);
  [defaults removeSuiteNamed: (NSString *)suiteID];
}

CFPropertyListRef
CFPreferencesCopyValue (CFStringRef key, CFStringRef applicationID,
  CFStringRef userName, CFStringRef hostName)
{
  /* Delegate to the App version; user/host parameters are not
     currently used with NSUserDefaults. */
  return CFPreferencesCopyAppValue (key, applicationID);
}

void
CFPreferencesSetValue (CFStringRef key, CFPropertyListRef value,
  CFStringRef applicationID, CFStringRef userName, CFStringRef hostName)
{
  CFPreferencesSetAppValue (key, value, applicationID);
}

Boolean
CFPreferencesSynchronize (CFStringRef applicationID, CFStringRef userName,
  CFStringRef hostName)
{
  return CFPreferencesAppSynchronize (applicationID);
}

CFArrayRef
CFPreferencesCopyKeyList (CFStringRef applicationID, CFStringRef userName,
  CFStringRef hostName)
{
  NSUserDefaults *defaults;
  NSDictionary *dict;
  NSArray *keys;

  if (applicationID == NULL)
    return NULL;

  defaults = _CFPreferencesDefaultsForAppID (applicationID);
  dict = [defaults dictionaryRepresentation];
  keys = [dict allKeys];

  if (keys == nil || [keys count] == 0)
    return NULL;

  return (CFArrayRef)CFRetain ((CFTypeRef)keys);
}

CFDictionaryRef
CFPreferencesCopyMultiple (CFArrayRef keysToFetch, CFStringRef applicationID,
  CFStringRef userName, CFStringRef hostName)
{
  NSUserDefaults *defaults;
  NSMutableDictionary *result;
  CFIndex i, count;

  if (applicationID == NULL)
    return NULL;

  defaults = _CFPreferencesDefaultsForAppID (applicationID);

  if (keysToFetch == NULL)
    {
      /* Return all keys */
      NSDictionary *dict = [defaults dictionaryRepresentation];
      if (dict == nil)
        return NULL;
      return (CFDictionaryRef)CFRetain ((CFTypeRef)dict);
    }

  count = CFArrayGetCount (keysToFetch);
  result = [NSMutableDictionary dictionaryWithCapacity: count];

  for (i = 0; i < count; i++)
    {
      NSString *key = (NSString *)CFArrayGetValueAtIndex (keysToFetch, i);
      id value = [defaults objectForKey: key];
      if (value != nil)
        [result setObject: value forKey: key];
    }

  if ([result count] == 0)
    return NULL;

  return (CFDictionaryRef)CFRetain ((CFTypeRef)result);
}

void
CFPreferencesSetMultiple (CFDictionaryRef keysToSet, CFArrayRef keysToRemove,
  CFStringRef applicationID, CFStringRef userName, CFStringRef hostName)
{
  NSUserDefaults *defaults;

  if (applicationID == NULL)
    return;

  defaults = _CFPreferencesDefaultsForAppID (applicationID);

  if (keysToRemove != NULL)
    {
      CFIndex i, count = CFArrayGetCount (keysToRemove);
      for (i = 0; i < count; i++)
        {
          NSString *key =
            (NSString *)CFArrayGetValueAtIndex (keysToRemove, i);
          [defaults removeObjectForKey: key];
        }
    }

  if (keysToSet != NULL)
    {
      NSEnumerator *enumerator =
        [(NSDictionary *)keysToSet keyEnumerator];
      NSString *key;
      while ((key = [enumerator nextObject]) != nil)
        {
          id value = [(NSDictionary *)keysToSet objectForKey: key];
          [defaults setObject: value forKey: key];
        }
    }
}
