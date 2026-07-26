/* CFNotificationCenter.h

   Copyright (C) 2025 Free Software Foundation, Inc.

   Written by: John Googler
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

#ifndef __COREFOUNDATION_CFNOTIFICATIONCENTER_H__
#define __COREFOUNDATION_CFNOTIFICATIONCENTER_H__

#include <CoreFoundation/CFBase.h>
#include <CoreFoundation/CFDictionary.h>

CF_EXTERN_C_BEGIN

typedef CFStringRef CFNotificationName;

typedef struct __CFNotificationCenter * CFNotificationCenterRef;

typedef void (*CFNotificationCallback)(CFNotificationCenterRef center,
  void *observer, CFNotificationName name, const void *object,
  CFDictionaryRef userInfo);

typedef CF_ENUM(CFIndex, CFNotificationSuspensionBehavior) {
  CFNotificationSuspensionBehaviorDrop = 1,
  CFNotificationSuspensionBehaviorCoalesce = 2,
  CFNotificationSuspensionBehaviorHold = 3,
  CFNotificationSuspensionBehaviorDeliverImmediately = 4
};

CF_ENUM(CFOptionFlags) {
  kCFNotificationDeliverImmediately = (1UL << 0),
  kCFNotificationPostToAllSessions = (1UL << 1)
};

/** \defgroup CFNotificationCenterRef CFNotificationCenter Reference
    \{
 */

/** \name Getting the CFNotificationCenter Type ID
    \{
 */
CF_EXPORT CFTypeID
CFNotificationCenterGetTypeID (void);
/** \} */

/** \name Getting Notification Centers
    \{
 */
CF_EXPORT CFNotificationCenterRef
CFNotificationCenterGetLocalCenter (void);

CF_EXPORT CFNotificationCenterRef
CFNotificationCenterGetDistributedCenter (void);
/** \} */

/** \name Adding and Removing Observers
    \{
 */
CF_EXPORT void
CFNotificationCenterAddObserver (CFNotificationCenterRef center,
  const void *observer, CFNotificationCallback callBack, CFStringRef name,
  const void *object, CFNotificationSuspensionBehavior suspensionBehavior);

CF_EXPORT void
CFNotificationCenterRemoveObserver (CFNotificationCenterRef center,
  const void *observer, CFNotificationName name, const void *object);

CF_EXPORT void
CFNotificationCenterRemoveEveryObserver (CFNotificationCenterRef center,
  const void *observer);
/** \} */

/** \name Posting Notifications
    \{
 */
CF_EXPORT void
CFNotificationCenterPostNotification (CFNotificationCenterRef center,
  CFNotificationName name, const void *object, CFDictionaryRef userInfo,
  Boolean deliverImmediately);

CF_EXPORT void
CFNotificationCenterPostNotificationWithOptions (CFNotificationCenterRef center,
  CFNotificationName name, const void *object, CFDictionaryRef userInfo,
  CFOptionFlags options);
/** \} */
/** \} */

CF_EXTERN_C_END

#endif /* __COREFOUNDATION_CFNOTIFICATIONCENTER_H__ */
